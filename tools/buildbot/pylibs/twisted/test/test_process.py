# Copyright (c) 2001-2008 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Test running processes.
"""

import gzip
import os
import popen2
import sys
import signal
import StringIO
import errno
import gc
import warnings
import socket
try:
    import fcntl
except ImportError:
    fcntl = None

from zope.interface.verify import verifyObject

from twisted.internet import reactor, protocol, error, interfaces, defer
from twisted.internet import selectreactor
from twisted.trial import unittest
from twisted.python import util, runtime, procutils

try:
    from twisted.internet import process
except ImportError:
    process = None



class StubProcessProtocol(protocol.ProcessProtocol):
    """
    ProcessProtocol counter-implementation: all methods on this class raise an
    exception, so instances of this may be used to verify that only certain
    methods are called.
    """
    def outReceived(self, data):
        raise NotImplementedError()

    def errReceived(self, data):
        raise NotImplementedError()

    def inConnectionLost(self):
        raise NotImplementedError()

    def outConnectionLost(self):
        raise NotImplementedError()

    def errConnectionLost(self):
        raise NotImplementedError()



class ProcessProtocolTests(unittest.TestCase):
    """
    Tests for behavior provided by the process protocol base class,
    L{protocol.ProcessProtocol}.
    """
    def test_interface(self):
        """
        L{ProcessProtocol} implements L{IProcessProtocol}.
        """
        verifyObject(interfaces.IProcessProtocol, protocol.ProcessProtocol())


    def test_outReceived(self):
        """
        Verify that when stdout is delivered to
        L{ProcessProtocol.childDataReceived}, it is forwarded to
        L{ProcessProtocol.outReceived}.
        """
        received = []
        class OutProtocol(StubProcessProtocol):
            def outReceived(self, data):
                received.append(data)

        bytes = "bytes"
        p = OutProtocol()
        p.childDataReceived(1, bytes)
        self.assertEqual(received, [bytes])


    def test_errReceived(self):
        """
        Similar to L{test_outReceived}, but for stderr.
        """
        received = []
        class ErrProtocol(StubProcessProtocol):
            def errReceived(self, data):
                received.append(data)

        bytes = "bytes"
        p = ErrProtocol()
        p.childDataReceived(2, bytes)
        self.assertEqual(received, [bytes])


    def test_inConnectionLost(self):
        """
        Verify that when stdin close notification is delivered to
        L{ProcessProtocol.childConnectionLost}, it is forwarded to
        L{ProcessProtocol.inConnectionLost}.
        """
        lost = []
        class InLostProtocol(StubProcessProtocol):
            def inConnectionLost(self):
                lost.append(None)

        p = InLostProtocol()
        p.childConnectionLost(0)
        self.assertEqual(lost, [None])


    def test_outConnectionLost(self):
        """
        Similar to L{test_inConnectionLost}, but for stdout.
        """
        lost = []
        class OutLostProtocol(StubProcessProtocol):
            def outConnectionLost(self):
                lost.append(None)

        p = OutLostProtocol()
        p.childConnectionLost(1)
        self.assertEqual(lost, [None])


    def test_errConnectionLost(self):
        """
        Similar to L{test_inConnectionLost}, but for stderr.
        """
        lost = []
        class ErrLostProtocol(StubProcessProtocol):
            def errConnectionLost(self):
                lost.append(None)

        p = ErrLostProtocol()
        p.childConnectionLost(2)
        self.assertEqual(lost, [None])



class TrivialProcessProtocol(protocol.ProcessProtocol):
    """
    Simple process protocol for tests purpose.

    @ivar outData: data received from stdin
    @ivar errData: data received from stderr
    """

    def __init__(self, d):
        """
        Create the deferred that will be fired at the end, and initialize
        data structures.
        """
        self.deferred = d
        self.outData = []
        self.errData = []

    def processEnded(self, reason):
        self.reason = reason
        self.deferred.callback(None)

    def outReceived(self, data):
        self.outData.append(data)

    def errReceived(self, data):
        self.errData.append(data)


class TestProcessProtocol(protocol.ProcessProtocol):

    def connectionMade(self):
        self.stages = [1]
        self.data = ''
        self.err = ''
        self.transport.write("abcd")

    def childDataReceived(self, childFD, data):
        """
        Override and disable the dispatch provided by the base class to ensure
        that it is really this method which is being called, and the transport
        is not going directly to L{outReceived} or L{errReceived}.
        """
        if childFD == 1:
            self.data += data
        elif childFD == 2:
            self.err += data


    def childConnectionLost(self, childFD):
        """
        Similarly to L{childDataReceived}, disable the automatic dispatch
        provided by the base implementation to verify that the transport is
        calling this method directly.
        """
        if childFD == 1:
            self.stages.append(2)
            if self.data != "abcd":
                raise RuntimeError
            self.transport.write("1234")
        elif childFD == 2:
            self.stages.append(3)
            if self.err != "1234":
                print 'err != 1234: ' + repr(self.err)
                raise RuntimeError()
            self.transport.write("abcd")
            self.stages.append(4)
        elif childFD == 0:
            self.stages.append(5)

    def processEnded(self, reason):
        self.reason = reason
        self.deferred.callback(None)


class EchoProtocol(protocol.ProcessProtocol):

    s = "1234567" * 1001
    n = 10
    finished = 0

    failure = None

    def __init__(self, onEnded):
        self.onEnded = onEnded
        self.count = 0

    def connectionMade(self):
        assert self.n > 2
        for i in range(self.n - 2):
            self.transport.write(self.s)
        # test writeSequence
        self.transport.writeSequence([self.s, self.s])
        self.buffer = self.s * self.n

    def outReceived(self, data):
        if buffer(self.buffer, self.count, len(data)) != buffer(data):
            self.failure = ("wrong bytes received", data, self.count)
            self.transport.closeStdin()
        else:
            self.count += len(data)
            if self.count == len(self.buffer):
                self.transport.closeStdin()

    def processEnded(self, reason):
        self.finished = 1
        if not reason.check(error.ProcessDone):
            self.failure = "process didn't terminate normally: " + str(reason)
        self.onEnded.callback(self)



class SignalProtocol(protocol.ProcessProtocol):
    """
    A process protocol that sends a signal when data is first received.

    @ivar deferred: deferred firing on C{processEnded}.
    @type deferred: L{defer.Deferred}

    @ivar signal: the signal to send to the process.
    @type signal: C{str}
    """

    def __init__(self, deferred, sig):
        self.deferred = deferred
        self.signal = sig


    def outReceived(self, data):
        self.transport.signalProcess(self.signal)


    def processEnded(self, reason):
        """
        Callback C{self.deferred} with C{None} if C{reason} is a
        L{error.ProcessTerminated} failure with C{exitCode} set to C{None},
        C{signal} set to C{self.signal}, and C{status} holding the status code
        of the exited process. Otherwise, errback with a C{ValueError}
        describing the problem.
        """
        if not reason.check(error.ProcessTerminated):
            return self.deferred.errback(
                ValueError("wrong termination: %s" % (reason,)))
        v = reason.value
        signalValue = getattr(signal, 'SIG' + self.signal)
        if v.exitCode is not None:
            return self.deferred.errback(
                ValueError("SIG%s: exitCode is %s, not None" %
                           (self.signal, v.exitCode)))
        if v.signal != signalValue:
            return self.deferred.errback(
                ValueError("SIG%s: .signal was %s, wanted %s" %
                           (self.signal, v.signal, signalValue)))
        if os.WTERMSIG(v.status) != signalValue:
            return self.deferred.errback(
                ValueError('SIG%s: %s' % (self.signal, os.WTERMSIG(v.status))))
        self.deferred.callback(None)



class TestManyProcessProtocol(TestProcessProtocol):
    def __init__(self):
        self.deferred = defer.Deferred()

    def processEnded(self, reason):
        self.reason = reason
        if reason.check(error.ProcessDone):
            self.deferred.callback(None)
        else:
            self.deferred.errback(reason)



class UtilityProcessProtocol(protocol.ProcessProtocol):
    """
    Helper class for launching a Python process and getting a result from it.

    @ivar program: A string giving a Python program for the child process to
    run.
    """
    program = None

    def run(cls, reactor, argv, env):
        """
        Run a Python process connected to a new instance of this protocol
        class.  Return the protocol instance.

        The Python process is given C{self.program} on the command line to
        execute, in addition to anything specified by C{argv}.  C{env} is
        the complete environment.
        """
        exe = sys.executable
        self = cls()
        reactor.spawnProcess(
            self, exe, [exe, "-c", self.program] + argv, env=env)
        return self
    run = classmethod(run)


    def __init__(self):
        self.bytes = []
        self.requests = []


    def parseChunks(self, bytes):
        """
        Called with all bytes received on stdout when the process exits.
        """
        raise NotImplementedError()


    def getResult(self):
        """
        Return a Deferred which will fire with the result of L{parseChunks}
        when the child process exits.
        """
        d = defer.Deferred()
        self.requests.append(d)
        return d


    def _fireResultDeferreds(self, result):
        """
        Callback all Deferreds returned up until now by L{getResult}
        with the given result object.
        """
        requests = self.requests
        self.requests = None
        for d in requests:
            d.callback(result)


    def outReceived(self, bytes):
        """
        Accumulate output from the child process in a list.
        """
        self.bytes.append(bytes)


    def processEnded(self, reason):
        """
        Handle process termination by parsing all received output and firing
        any waiting Deferreds.
        """
        self._fireResultDeferreds(self.parseChunks(self.bytes))




class GetArgumentVector(UtilityProcessProtocol):
    """
    Protocol which will read a serialized argv from a process and
    expose it to interested parties.
    """
    program = (
        "from sys import stdout, argv\n"
        "stdout.write(chr(0).join(argv))\n"
        "stdout.flush()\n")

    def parseChunks(self, chunks):
        """
        Parse the output from the process to which this protocol was
        connected, which is a single unterminated line of \\0-separated
        strings giving the argv of that process.  Return this as a list of
        str objects.
        """
        return ''.join(chunks).split('\0')



class GetEnvironmentDictionary(UtilityProcessProtocol):
    """
    Protocol which will read a serialized environment dict from a process
    and expose it to interested parties.
    """
    program = (
        "from sys import stdout\n"
        "from os import environ\n"
        "items = environ.iteritems()\n"
        "stdout.write(chr(0).join([k + chr(0) + v for k, v in items]))\n"
        "stdout.flush()\n")

    def parseChunks(self, chunks):
        """
        Parse the output from the process to which this protocol was
        connected, which is a single unterminated line of \\0-separated
        strings giving key value pairs of the environment from that process.
        Return this as a dictionary.
        """
        environString = ''.join(chunks)
        if not environString:
            return {}
        environ = iter(environString.split('\0'))
        d = {}
        while 1:
            try:
                k = environ.next()
            except StopIteration:
                break
            else:
                v = environ.next()
                d[k] = v
        return d



class ProcessTestCase(unittest.TestCase):
    """Test running a process."""

    usePTY = False

    def testStdio(self):
        """twisted.internet.stdio test."""
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_twisted.py")
        p = Accumulator()
        d = p.endedDeferred = defer.Deferred()
        env = {"PYTHONPATH": os.pathsep.join(sys.path)}
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=env,
                             path=None, usePTY=self.usePTY)
        p.transport.write("hello, world")
        p.transport.write("abc")
        p.transport.write("123")
        p.transport.closeStdin()

        def processEnded(ign):
            self.assertEquals(p.outF.getvalue(), "hello, worldabc123",
                              "Output follows:\n"
                              "%s\n"
                              "Error message from process_twisted follows:\n"
                              "%s\n" % (p.outF.getvalue(), p.errF.getvalue()))
        return d.addCallback(processEnded)


    def test_unsetPid(self):
        """
        Test if pid is None/non-None before/after process termination.  This
        reuses process_echoer.py to get a process that blocks on stdin.
        """
        finished = defer.Deferred()
        p = TrivialProcessProtocol(finished)
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_echoer.py")
        procTrans = reactor.spawnProcess(p, exe,
                                    [exe, "-u", scriptPath], env=None)
        self.failUnless(procTrans.pid)

        def afterProcessEnd(ignored):
            self.assertEqual(procTrans.pid, None)

        p.transport.closeStdin()
        return finished.addCallback(afterProcessEnd)


    def test_process(self):
        """
        Test running a process: check its output, it exitCode, some property of
        signalProcess.
        """
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_tester.py")
        d = defer.Deferred()
        p = TestProcessProtocol()
        p.deferred = d
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=None)
        def check(ignored):
            self.assertEquals(p.stages, [1, 2, 3, 4, 5])
            f = p.reason
            f.trap(error.ProcessTerminated)
            self.assertEquals(f.value.exitCode, 23)
            # would .signal be available on non-posix?
            # self.assertEquals(f.value.signal, None)
            self.assertRaises(
                error.ProcessExitedAlready, p.transport.signalProcess, 'INT')
            try:
                import process_tester, glob
                for f in glob.glob(process_tester.test_file_match):
                    os.remove(f)
            except:
                pass
        d.addCallback(check)
        return d

    def testManyProcesses(self):

        def _check(results, protocols):
            for p in protocols:
                self.assertEquals(p.stages, [1, 2, 3, 4, 5], "[%d] stages = %s" % (id(p.transport), str(p.stages)))
                # test status code
                f = p.reason
                f.trap(error.ProcessTerminated)
                self.assertEquals(f.value.exitCode, 23)

        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_tester.py")
        args = [exe, "-u", scriptPath]
        protocols = []
        deferreds = []

        for i in xrange(50):
            p = TestManyProcessProtocol()
            protocols.append(p)
            reactor.spawnProcess(p, exe, args, env=None)
            deferreds.append(p.deferred)

        deferredList = defer.DeferredList(deferreds, consumeErrors=True)
        deferredList.addCallback(_check, protocols)
        return deferredList

    def testEcho(self):
        finished = defer.Deferred()
        p = EchoProtocol(finished)

        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_echoer.py")
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=None)

        def asserts(ignored):
            self.failIf(p.failure, p.failure)
            self.failUnless(hasattr(p, 'buffer'))
            self.assertEquals(len(''.join(p.buffer)), len(p.s * p.n))

        def takedownProcess(err):
            p.transport.closeStdin()
            return err

        return finished.addCallback(asserts).addErrback(takedownProcess)
    testEcho.timeout = 60 # XXX This should not be.  There is already a
                          # global timeout value.  Why do you think this
                          # test can complete more quickly?


    def testCommandLine(self):
        args = [r'a\"b ', r'a\b ', r' a\\"b', r' a\\b', r'"foo bar" "', '\tab', '"\\', 'a"b', "a'b"]
        pyExe = sys.executable
        scriptPath = util.sibpath(__file__, "process_cmdline.py")
        p = Accumulator()
        d = p.endedDeferred = defer.Deferred()
        reactor.spawnProcess(p, pyExe, [pyExe, "-u", scriptPath]+args, env=None,
                             path=None)

        def processEnded(ign):
            self.assertEquals(p.errF.getvalue(), "")
            recvdArgs = p.outF.getvalue().splitlines()
            self.assertEquals(recvdArgs, args)
        return d.addCallback(processEnded)


    def test_wrongArguments(self):
        """
        Test invalid arguments to spawnProcess: arguments and environment
        must only contains string or unicode, and not null bytes.
        """
        exe = sys.executable
        p = protocol.ProcessProtocol()

        badEnvs = [
            {"foo": 2},
            {"foo": "egg\0a"},
            {3: "bar"},
            {"bar\0foo": "bar"}]

        badArgs = [
            [exe, 2],
            "spam",
            [exe, "foo\0bar"]]

        # Sanity check - this will fail for people who have mucked with
        # their site configuration in a stupid way, but there's nothing we
        # can do about that.
        badUnicode = u'\N{SNOWMAN}'
        try:
            badUnicode.encode(sys.getdefaultencoding())
        except UnicodeEncodeError:
            # Okay, that unicode doesn't encode, put it in as a bad environment
            # key.
            badEnvs.append({badUnicode: 'value for bad unicode key'})
            badEnvs.append({'key for bad unicode value': badUnicode})
            badArgs.append([exe, badUnicode])
        else:
            # It _did_ encode.  Most likely, Gtk2 is being used and the
            # default system encoding is UTF-8, which can encode anything.
            # In any case, if implicit unicode -> str conversion works for
            # that string, we can't test that TypeError gets raised instead,
            # so just leave it off.
            pass

        for env in badEnvs:
            self.assertRaises(
                TypeError,
                reactor.spawnProcess, p, exe, [exe, "-c", ""], env=env)

        for args in badArgs:
            self.assertRaises(
                TypeError,
                reactor.spawnProcess, p, exe, args, env=None)


    # Use upper-case so that the environment key test uses an upper case
    # name: some versions of Windows only support upper case environment
    # variable names, and I think Python (as of 2.5) doesn't use the right
    # syscall for lowercase or mixed case names to work anyway.
    okayUnicode = u"UNICODE"
    encodedValue = "UNICODE"

    def _deprecatedUnicodeSupportTest(self, processProtocolClass, argv=[], env={}):
        """
        Check that a deprecation warning is emitted when passing unicode to
        spawnProcess for an argv value or an environment key or value.
        Check that the warning is of the right type, has the right message,
        and refers to the correct file.  Unfortunately, don't check that the
        line number is correct, because that is too hard for me to figure
        out.

        @param processProtocolClass: A L{UtilityProcessProtocol} subclass
        which will be instantiated to communicate with the child process.

        @param argv: The argv argument to spawnProcess.

        @param env: The env argument to spawnProcess.

        @return: A Deferred which fires when the test is complete.
        """
        # Sanity to check to make sure we can actually encode this unicode
        # with the default system encoding.  This may be excessively
        # paranoid. -exarkun
        self.assertEqual(
            self.okayUnicode.encode(sys.getdefaultencoding()),
            self.encodedValue)

        p = self.assertWarns(DeprecationWarning,
            "Argument strings and environment keys/values passed to "
            "reactor.spawnProcess should be str, not unicode.", __file__,
            processProtocolClass.run, reactor, argv, env)
        return p.getResult()


    def test_deprecatedUnicodeArgvSupport(self):
        """
        Test that a unicode string passed for an argument value is allowed
        if it can be encoded with the default system encoding, but that a
        deprecation warning is emitted.
        """
        d = self._deprecatedUnicodeSupportTest(GetArgumentVector, argv=[self.okayUnicode])
        def gotArgVector(argv):
            self.assertEqual(argv, ['-c', self.encodedValue])
        d.addCallback(gotArgVector)
        return d


    def test_deprecatedUnicodeEnvKeySupport(self):
        """
        Test that a unicode string passed for the key of the environment
        dictionary is allowed if it can be encoded with the default system
        encoding, but that a deprecation warning is emitted.
        """
        d = self._deprecatedUnicodeSupportTest(
            GetEnvironmentDictionary, env={self.okayUnicode: self.encodedValue})
        def gotEnvironment(environ):
            self.assertEqual(environ[self.encodedValue], self.encodedValue)
        d.addCallback(gotEnvironment)
        return d


    def test_deprecatedUnicodeEnvValueSupport(self):
        """
        Test that a unicode string passed for the value of the environment
        dictionary is allowed if it can be encoded with the default system
        encoding, but that a deprecation warning is emitted.
        """
        d = self._deprecatedUnicodeSupportTest(
            GetEnvironmentDictionary, env={self.encodedValue: self.okayUnicode})
        def gotEnvironment(environ):
            # On Windows, the environment contains more things than we
            # specified, so only make sure that at least the key we wanted
            # is there, rather than testing the dictionary for exact
            # equality.
            self.assertEqual(environ[self.encodedValue], self.encodedValue)
        d.addCallback(gotEnvironment)
        return d



class TwoProcessProtocol(protocol.ProcessProtocol):
    num = -1
    finished = 0
    def __init__(self):
        self.deferred = defer.Deferred()
    def outReceived(self, data):
        pass
    def processEnded(self, reason):
        self.finished = 1
        self.deferred.callback(None)

class TestTwoProcessesBase:
    def setUp(self):
        self.processes = [None, None]
        self.pp = [None, None]
        self.done = 0
        self.verbose = 0

    def createProcesses(self, usePTY=0):
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_reader.py")
        for num in (0,1):
            self.pp[num] = TwoProcessProtocol()
            self.pp[num].num = num
            p = reactor.spawnProcess(self.pp[num],
                                     exe, [exe, "-u", scriptPath], env=None,
                                     usePTY=usePTY)
            self.processes[num] = p

    def close(self, num):
        if self.verbose: print "closing stdin [%d]" % num
        p = self.processes[num]
        pp = self.pp[num]
        self.failIf(pp.finished, "Process finished too early")
        p.loseConnection()
        if self.verbose: print self.pp[0].finished, self.pp[1].finished

    def _onClose(self):
        return defer.gatherResults([ p.deferred for p in self.pp ])

    def testClose(self):
        if self.verbose: print "starting processes"
        self.createProcesses()
        reactor.callLater(1, self.close, 0)
        reactor.callLater(2, self.close, 1)
        return self._onClose()

class TestTwoProcessesNonPosix(TestTwoProcessesBase, unittest.TestCase):
    pass

class TestTwoProcessesPosix(TestTwoProcessesBase, unittest.TestCase):
    def tearDown(self):
        for pp, pr in zip(self.pp, self.processes):
            if not pp.finished:
                try:
                    os.kill(pr.pid, signal.SIGTERM)
                except OSError:
                    # If the test failed the process may already be dead
                    # The error here is only noise
                    pass
        return self._onClose()

    def kill(self, num):
        if self.verbose: print "kill [%d] with SIGTERM" % num
        p = self.processes[num]
        pp = self.pp[num]
        self.failIf(pp.finished, "Process finished too early")
        os.kill(p.pid, signal.SIGTERM)
        if self.verbose: print self.pp[0].finished, self.pp[1].finished

    def testKill(self):
        if self.verbose: print "starting processes"
        self.createProcesses(usePTY=0)
        reactor.callLater(1, self.kill, 0)
        reactor.callLater(2, self.kill, 1)
        return self._onClose()

    def testClosePty(self):
        if self.verbose: print "starting processes"
        self.createProcesses(usePTY=1)
        reactor.callLater(1, self.close, 0)
        reactor.callLater(2, self.close, 1)
        return self._onClose()

    def testKillPty(self):
        if self.verbose: print "starting processes"
        self.createProcesses(usePTY=1)
        reactor.callLater(1, self.kill, 0)
        reactor.callLater(2, self.kill, 1)
        return self._onClose()

class FDChecker(protocol.ProcessProtocol):
    state = 0
    data = ""
    failed = None

    def __init__(self, d):
        self.deferred = d

    def fail(self, why):
        self.failed = why
        self.deferred.callback(None)

    def connectionMade(self):
        self.transport.writeToChild(0, "abcd")
        self.state = 1

    def childDataReceived(self, childFD, data):
        if self.state == 1:
            if childFD != 1:
                self.fail("read '%s' on fd %d (not 1) during state 1" \
                          % (childFD, data))
                return
            self.data += data
            #print "len", len(self.data)
            if len(self.data) == 6:
                if self.data != "righto":
                    self.fail("got '%s' on fd1, expected 'righto'" \
                              % self.data)
                    return
                self.data = ""
                self.state = 2
                #print "state2", self.state
                self.transport.writeToChild(3, "efgh")
                return
        if self.state == 2:
            self.fail("read '%s' on fd %s during state 2" % (childFD, data))
            return
        if self.state == 3:
            if childFD != 1:
                self.fail("read '%s' on fd %s (not 1) during state 3" \
                          % (childFD, data))
                return
            self.data += data
            if len(self.data) == 6:
                if self.data != "closed":
                    self.fail("got '%s' on fd1, expected 'closed'" \
                              % self.data)
                    return
                self.state = 4
            return
        if self.state == 4:
            self.fail("read '%s' on fd %s during state 4" % (childFD, data))
            return

    def childConnectionLost(self, childFD):
        if self.state == 1:
            self.fail("got connectionLost(%d) during state 1" % childFD)
            return
        if self.state == 2:
            if childFD != 4:
                self.fail("got connectionLost(%d) (not 4) during state 2" \
                          % childFD)
                return
            self.state = 3
            self.transport.closeChildFD(5)
            return

    def processEnded(self, status):
        rc = status.value.exitCode
        if self.state != 4:
            self.fail("processEnded early, rc %d" % rc)
            return
        if status.value.signal != None:
            self.fail("processEnded with signal %s" % status.value.signal)
            return
        if rc != 0:
            self.fail("processEnded with rc %d" % rc)
            return
        self.deferred.callback(None)


class FDTest(unittest.TestCase):

    def testFD(self):
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_fds.py")
        d = defer.Deferred()
        p = FDChecker(d)
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=None,
                             path=None,
                             childFDs={0:"w", 1:"r", 2:2,
                                       3:"w", 4:"r", 5:"w"})
        d.addCallback(lambda x : self.failIf(p.failed, p.failed))
        return d

    def testLinger(self):
        # See what happens when all the pipes close before the process
        # actually stops. This test *requires* SIGCHLD catching to work,
        # as there is no other way to find out the process is done.
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_linger.py")
        p = Accumulator()
        d = p.endedDeferred = defer.Deferred()
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=None,
                             path=None,
                             childFDs={1:"r", 2:2},
                             )
        def processEnded(ign):
            self.failUnlessEqual(p.outF.getvalue(),
                                 "here is some text\ngoodbye\n")
        return d.addCallback(processEnded)



class Accumulator(protocol.ProcessProtocol):
    """Accumulate data from a process."""

    closed = 0
    endedDeferred = None

    def connectionMade(self):
        self.outF = StringIO.StringIO()
        self.errF = StringIO.StringIO()

    def outReceived(self, d):
        self.outF.write(d)

    def errReceived(self, d):
        self.errF.write(d)

    def outConnectionLost(self):
        pass

    def errConnectionLost(self):
        pass

    def processEnded(self, reason):
        self.closed = 1
        if self.endedDeferred is not None:
            d, self.endedDeferred = self.endedDeferred, None
            d.callback(None)


class PosixProcessBase:
    """
    Test running processes.
    """
    usePTY = False

    def getCommand(self, commandName):
        """
        Return the path of the shell command named C{commandName}, looking at
        common locations.
        """
        if os.path.exists('/bin/%s' % (commandName,)):
            cmd = '/bin/%s' % (commandName,)
        elif os.path.exists('/usr/bin/%s' % (commandName,)):
            cmd = '/usr/bin/%s' % (commandName,)
        else:
            raise RuntimeError(
                "%s not found in /bin or /usr/bin" % (commandName,))
        return cmd

    def testNormalTermination(self):
        cmd = self.getCommand('true')

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        reactor.spawnProcess(p, cmd, ['true'], env=None,
                             usePTY=self.usePTY)
        def check(ignored):
            p.reason.trap(error.ProcessDone)
            self.assertEquals(p.reason.value.exitCode, 0)
            self.assertEquals(p.reason.value.signal, None)
        d.addCallback(check)
        return d


    def test_abnormalTermination(self):
        """
        When a process terminates with a system exit code set to 1,
        C{processEnded} is called with a L{error.ProcessTerminated} error,
        the C{exitCode} attribute reflecting the system exit code.
        """
        exe = sys.executable

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        reactor.spawnProcess(p, exe, [exe, '-c', 'import sys; sys.exit(1)'],
                             env=None, usePTY=self.usePTY)

        def check(ignored):
            p.reason.trap(error.ProcessTerminated)
            self.assertEquals(p.reason.value.exitCode, 1)
            self.assertEquals(p.reason.value.signal, None)
        d.addCallback(check)
        return d


    def _testSignal(self, sig):
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_signal.py")
        d = defer.Deferred()
        p = SignalProtocol(d, sig)
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=None,
                             usePTY=self.usePTY)
        return d


    def test_signalHUP(self):
        """
        Sending the SIGHUP signal to a running process interrupts it, and
        C{processEnded} is called with a L{error.ProcessTerminated} instance
        with the C{exitCode} set to C{None} and the C{signal} attribute set to
        C{signal.SIGHUP}. C{os.WTERMSIG} can also be used on the C{status}
        attribute to extract the signal value.
        """
        return self._testSignal('HUP')


    def test_signalINT(self):
        """
        Sending the SIGINT signal to a running process interrupts it, and
        C{processEnded} is called with a L{error.ProcessTerminated} instance
        with the C{exitCode} set to C{None} and the C{signal} attribute set to
        C{signal.SIGINT}. C{os.WTERMSIG} can also be used on the C{status}
        attribute to extract the signal value.
        """
        return self._testSignal('INT')


    def test_signalKILL(self):
        """
        Sending the SIGKILL signal to a running process interrupts it, and
        C{processEnded} is called with a L{error.ProcessTerminated} instance
        with the C{exitCode} set to C{None} and the C{signal} attribute set to
        C{signal.SIGKILL}. C{os.WTERMSIG} can also be used on the C{status}
        attribute to extract the signal value.
        """
        return self._testSignal('KILL')


    def test_signalTERM(self):
        """
        Sending the SIGTERM signal to a running process interrupts it, and
        C{processEnded} is called with a L{error.ProcessTerminated} instance
        with the C{exitCode} set to C{None} and the C{signal} attribute set to
        C{signal.SIGTERM}. C{os.WTERMSIG} can also be used on the C{status}
        attribute to extract the signal value.
        """
        return self._testSignal('TERM')


    def test_executionError(self):
        """
        Raise an error during execvpe to check error management.
        """
        cmd = self.getCommand('false')

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        def buggyexecvpe(command, args, environment):
            raise RuntimeError("Ouch")
        oldexecvpe = os.execvpe
        os.execvpe = buggyexecvpe
        try:
            reactor.spawnProcess(p, cmd, ['false'], env=None,
                                 usePTY=self.usePTY)

            def check(ignored):
                errData = "".join(p.errData + p.outData)
                self.assertIn("Upon execvpe", errData)
                self.assertIn("Ouch", errData)
            d.addCallback(check)
        finally:
            os.execvpe = oldexecvpe
        return d



class MockOS(object):
    """
    The mock OS: overwrite L{os}, L{fcntl} and {sys} functions with fake ones.

    @ivar exited: set to True when C{_exit} is called.
    @type exited: C{bool}

    @ivar O_RDWR: dumb value faking C{os.O_RDWR}.
    @type O_RDWR: C{int}

    @ivar O_NOCTTY: dumb value faking C{os.O_NOCTTY}.
    @type O_NOCTTY: C{int}

    @ivar WNOHANG: dumb value faking C{os.WNOHANG}.
    @type WNOHANG: C{int}

    @ivar raiseFork: if not C{None}, subsequent calls to fork will raise this
        object.
    @type raiseFork: C{NoneType} or C{Exception}

    @ivar raiseExec: if set, subsequent calls to execvpe will raise an error.
    @type raiseExec: C{bool}

    @ivar fdio: fake file object returned by calls to fdopen.
    @type fdio: C{StringIO.StringIO}

    @ivar actions: hold names of some actions executed by the object, in order
        of execution.

    @type actions: C{list} of C{str}

    @ivar closed: keep track of the file descriptor closed.
    @param closed: C{list} of C{int}

    @ivar child: whether fork return for the child or the parent.
    @type child: C{bool}

    @ivar pipeCount: count the number of time that C{os.pipe} has been called.
    @type pipeCount: C{int}

    @ivar raiseWaitPid: if set, subsequent calls to waitpid will raise an
        the error specified.
    @type raiseWaitPid: C{None} or a class

    @ivar waitChild: if set, subsequent calls to waitpid will return it.
    @type waitChild: C{None} or a tuple
    """
    exited = False
    O_RDWR = 1
    O_NOCTTY = 1
    WNOHANG = 1
    raiseExec = False
    fdio = None
    child = True
    raiseWaitPid = None
    raiseFork = None
    waitChild = None

    def __init__(self):
        """
        Initialiaze data structures.
        """
        self.actions = []
        self.closed = []
        self.pipeCount = 0


    def open(self, dev, flags):
        """
        Fake C{os.open}. Return a non fd number to be sure it's not used
        elsewhere.
        """
        return -3


    def fdopen(self, fd, flag):
        """
        Fake C{os.fdopen}. Return a StringIO object whose content can be tested
        later via C{self.fdio}.
        """
        self.fdio = StringIO.StringIO()
        return self.fdio


    def setsid(self):
        """
        Fake C{os.setsid}. Do nothing.
        """


    def fork(self):
        """
        Fake C{os.fork}. Save the action in C{self.actions}, and return 0 if
        C{self.child} is set, or a dumb number.
        """
        self.actions.append(('fork', gc.isenabled()))
        if self.raiseFork is not None:
            raise self.raiseFork
        elif self.child:
            # Child result is 0
            return 0
        else:
            return 21


    def close(self, fd):
        """
        Fake C{os.close}, saving the closed fd in C{self.closed}.
        """
        self.closed.append(fd)


    def dup2(self, fd1, fd2):
        """
        Fake C{os.dup2}. Do nothing.
        """


    def write(self, fd, data):
        """
        Fake C{os.write}. Do nothing.
        """


    def execvpe(self, command, args, env):
        """
        Fake C{os.execvpe}. Save the action, and raise an error if
        C{self.raiseExec} is set.
        """
        self.actions.append('exec')
        if self.raiseExec:
            raise RuntimeError("Bar")


    def pipe(self):
        """
        Fake C{os.pipe}. Return non fd numbers to be sure it's not used
        elsewhere, and increment C{self.pipeCount}. This is used to uniquify
        the result.
        """
        self.pipeCount += 1
        return - 2 * self.pipeCount + 1,  - 2 * self.pipeCount


    def ttyname(self, fd):
        """
        Fake C{os.ttyname}. Return a dumb string.
        """
        return "foo"


    def _exit(self, code):
        """
        Fake C{os._exit}. Save the action, set the C{self.exited} flag, and
        raise C{SystemError}.
        """
        self.actions.append('exit')
        self.exited = True
        # Don't forget to raise an error, or you'll end up in parent
        # code path.
        raise SystemError()


    def ioctl(self, fd, flags, arg):
        """
        Override C{fcntl.ioctl}. Do nothing.
        """


    def setNonBlocking(self, fd):
        """
        Override C{fdesc.setNonBlocking}. Do nothing.
        """


    def waitpid(self, pid, options):
        """
        Override C{os.waitpid}. Return values meaning that the child process
        has exited, save executed action.
        """
        self.actions.append('waitpid')
        if self.raiseWaitPid is not None:
            raise self.raiseWaitPid
        if self.waitChild is not None:
            return self.waitChild
        return 1, 0


    def settrace(self, arg):
        """
        Override C{sys.settrace} to keep coverage working.
        """


    def getegid(self):
        """
        Override C{os.getegid}. Return a dumb number.
        """
        return 1234


    def getgid(self):
        """
        Override C{os.getgid}. Return a dumb number.
        """
        return 1235


    def geteuid(self):
        """
        Override C{os.geteuid}. Return a dumb number.
        """
        return 1236


    def getuid(self):
        """
        Override C{os.getuid}. Return a dumb number.
        """
        return 1237


    def setuid(self, val):
        """
        Override C{os.setuid}. Do nothing.
        """
        self.actions.append(('setuid', val))


    def setgid(self, val):
        """
        Override C{os.setgid}. Do nothing.
        """
        self.actions.append(('setgid', val))


    def setregid(self, val1, val2):
        """
        Override C{os.setregid}. Do nothing.
        """
        self.actions.append(('setregid', val1, val2))


    def setreuid(self, val1, val2):
        """
        Override C{os.setreuid}.  Save the action.
        """
        self.actions.append(('setreuid', val1, val2))


    def switchUID(self, uid, gid):
        """
        Override C{util.switchuid}. Save the action.
        """
        self.actions.append(('switchuid', uid, gid))



if process is not None:
    class DumbProcessWriter(process.ProcessWriter):
        """
        A fake L{process.ProcessWriter} used for tests.
        """

        def startReading(self):
            """
            Here's the faking: don't do anything here.
            """



    class DumbProcessReader(process.ProcessReader):
        """
        A fake L{process.ProcessReader} used for tests.
        """

        def startReading(self):
            """
            Here's the faking: don't do anything here.
            """



    class DumbPTYProcess(process.PTYProcess):
        """
        A fake L{process.PTYProcess} used for tests.
        """

        def startReading(self):
            """
            Here's the faking: don't do anything here.
            """



class MockProcessTestCase(unittest.TestCase):
    """
    Mock a process runner to test forked child code path.
    """

    def setUp(self):
        """
        Replace L{process} os, fcntl, sys, switchUID modules with the mock
        class L{MockOS}.
        """
        if gc.isenabled():
            self.addCleanup(gc.enable)
        else:
            self.addCleanup(gc.disable)
        self.mockos = MockOS()
        self.oldos = os
        self.oldfcntl = fcntl
        self.oldsys = sys
        self.oldSwitchUID = util.switchUID
        self.oldFdesc = process.fdesc
        process.os = self.mockos
        process.fcntl = self.mockos
        process.sys = self.mockos
        process.switchUID = self.mockos.switchUID
        process.fdesc = self.mockos
        process.Process.processReaderFactory = DumbProcessReader
        process.Process.processWriterFactory = DumbProcessWriter


    def tearDown(self):
        """
        Restore L{process} modules, and reset processes registered for reap.
        """
        process.os = self.oldos
        process.fcntl = self.oldfcntl
        process.sys = self.oldsys
        process.switchUID = self.oldSwitchUID
        process.fdesc = self.oldFdesc
        process.Process.processReaderFactory = process.ProcessReader
        process.Process.processWriterFactory = process.ProcessWriter
        process.reapProcessHandlers = {}


    def test_mockFork(self):
        """
        Test a classic spawnProcess. Check the path of the client code:
        fork, exec, exit.
        """
        gc.enable()

        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        try:
            reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                                 usePTY=False)
        except SystemError:
            self.assert_(self.mockos.exited)
            self.assertEquals(
                self.mockos.actions, [("fork", False), "exec", "exit"])
        else:
            self.fail("Should not be here")

        # It should leave the garbage collector disabled.
        self.assertFalse(gc.isenabled())


    def _mockForkInParentTest(self):
        """
        Assert that in the main process, spawnProcess disables the garbage
        collector, calls fork, closes the pipe file descriptors it created for
        the child process, and calls waitpid.
        """
        self.mockos.child = False
        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                             usePTY=False)
        # It should close the first read pipe, and the 2 last write pipes
        self.assertEqual(self.mockos.closed, [-1, -4, -6])
        self.assertEquals(self.mockos.actions, [("fork", False), "waitpid"])


    def test_mockForkInParentGarbageCollectorEnabled(self):
        """
        The garbage collector should be enabled when L{reactor.spawnProcess}
        returns if it was initially enabled.

        @see L{_mockForkInParentTest}
        """
        gc.enable()
        self._mockForkInParentTest()
        self.assertTrue(gc.isenabled())


    def test_mockForkInParentGarbageCollectorDisabled(self):
        """
        The garbage collector should be disabled when L{reactor.spawnProcess}
        returns if it was initially disabled.

        @see L{_mockForkInParentTest}
        """
        gc.disable()
        self._mockForkInParentTest()
        self.assertFalse(gc.isenabled())


    def test_mockForkTTY(self):
        """
        Test a TTY spawnProcess: check the path of the client code:
        fork, exec, exit.
        """
        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        try:
            reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                                 usePTY=True)
        except SystemError:
            self.assert_(self.mockos.exited)
            self.assertEquals(
                self.mockos.actions, [("fork", False), "exec", "exit"])
        else:
            self.fail("Should not be here")


    def _mockWithForkError(self):
        """
        Assert that if the fork call fails, no other process setup calls are
        made and that spawnProcess raises the exception fork raised.
        """
        self.mockos.raiseFork = OSError(errno.EAGAIN, None)
        protocol = TrivialProcessProtocol(None)
        self.assertRaises(OSError, reactor.spawnProcess, protocol, None)
        self.assertEqual(self.mockos.actions, [("fork", False)])


    def test_mockWithForkErrorGarbageCollectorEnabled(self):
        """
        The garbage collector should be enabled when L{reactor.spawnProcess}
        raises because L{os.fork} raised, if it was initially enabled.
        """
        gc.enable()
        self._mockWithForkError()
        self.assertTrue(gc.isenabled())


    def test_mockWithForkErrorGarbageCollectorDisabled(self):
        """
        The garbage collector should be disabled when
        L{reactor.spawnProcess} raises because L{os.fork} raised, if it was
        initially disabled.
        """
        gc.disable()
        self._mockWithForkError()
        self.assertFalse(gc.isenabled())


    def test_mockWithExecError(self):
        """
        Spawn a process but simulate an error during execution in the client
        path: C{os.execvpe} raises an error. It should close all the standard
        fds, try to print the error encountered, and exit cleanly.
        """
        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        self.mockos.raiseExec = True
        try:
            reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                                 usePTY=False)
        except SystemError:
            self.assert_(self.mockos.exited)
            self.assertEquals(
                self.mockos.actions, [("fork", False), "exec", "exit"])
            # Check that fd have been closed
            self.assertIn(0, self.mockos.closed)
            self.assertIn(1, self.mockos.closed)
            self.assertIn(2, self.mockos.closed)
            # Check content of traceback
            self.assertIn("RuntimeError: Bar", self.mockos.fdio.getvalue())
        else:
            self.fail("Should not be here")


    def test_mockSetUid(self):
        """
        Try creating a process with setting its uid: it's almost the same path
        as the standard path, but with a C{switchUID} call before the exec.
        """
        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        try:
            reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                                 usePTY=False, uid=8080)
        except SystemError:
            self.assert_(self.mockos.exited)
            self.assertEquals(self.mockos.actions,
                [('setuid', 0), ('setgid', 0), ('fork', False),
                  ('switchuid', 8080, 1234), 'exec', 'exit'])
        else:
            self.fail("Should not be here")


    def test_mockSetUidInParent(self):
        """
        Try creating a process with setting its uid, in the parent path: it
        should switch to root before fork, then restore initial uid/gids.
        """
        self.mockos.child = False
        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                             usePTY=False, uid=8080)
        self.assertEquals(self.mockos.actions,
            [('setuid', 0), ('setgid', 0), ('fork', False),
             ('setregid', 1235, 1234), ('setreuid', 1237, 1236), 'waitpid'])


    def test_mockPTYSetUid(self):
        """
        Try creating a PTY process with setting its uid: it's almost the same
        path as the standard path, but with a C{switchUID} call before the
        exec.
        """
        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        try:
            reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                                 usePTY=True, uid=8081)
        except SystemError:
            self.assert_(self.mockos.exited)
            self.assertEquals(self.mockos.actions,
                [('setuid', 0), ('setgid', 0), ('fork', False),
                  ('switchuid', 8081, 1234), 'exec', 'exit'])
        else:
            self.fail("Should not be here")


    def test_mockPTYSetUidInParent(self):
        """
        Try creating a PTY process with setting its uid, in the parent path: it
        should switch to root before fork, then restore initial uid/gids.
        """
        self.mockos.child = False
        cmd = '/mock/ouch'

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        oldPTYProcess = process.PTYProcess
        try:
            process.PTYProcess = DumbPTYProcess
            reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                                 usePTY=True, uid=8080)
        finally:
            process.PTYProcess = oldPTYProcess
        self.assertEquals(self.mockos.actions,
            [('setuid', 0), ('setgid', 0), ('fork', False),
             ('setregid', 1235, 1234), ('setreuid', 1237, 1236), 'waitpid'])


    def test_mockWithWaitError(self):
        """
        Test that reapProcess logs errors raised.
        """
        self.mockos.child = False
        cmd = '/mock/ouch'
        self.mockos.waitChild = (0, 0)

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        proc = reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                             usePTY=False)
        self.assertEquals(self.mockos.actions, [("fork", False), "waitpid"])

        self.mockos.raiseWaitPid = OSError()
        proc.reapProcess()
        errors = self.flushLoggedErrors()
        self.assertEquals(len(errors), 1)
        errors[0].trap(OSError)


    def test_mockErrorECHILDInReapProcess(self):
        """
        Test that reapProcess doesn't log anything when waitpid raises a
        C{OSError} with errno C{ECHILD}.
        """
        self.mockos.child = False
        cmd = '/mock/ouch'
        self.mockos.waitChild = (0, 0)

        d = defer.Deferred()
        p = TrivialProcessProtocol(d)
        proc = reactor.spawnProcess(p, cmd, ['ouch'], env=None,
                                    usePTY=False)
        self.assertEquals(self.mockos.actions, [("fork", False), "waitpid"])

        self.mockos.raiseWaitPid = OSError()
        self.mockos.raiseWaitPid.errno = errno.ECHILD
        # This should not produce any errors
        proc.reapProcess()


class PosixProcessTestCase(unittest.TestCase, PosixProcessBase):
    # add three non-pty test cases

    def testStderr(self):
        # we assume there is no file named ZZXXX..., both in . and in /tmp
        cmd = self.getCommand('ls')

        p = Accumulator()
        d = p.endedDeferred = defer.Deferred()
        reactor.spawnProcess(p, cmd,
                             [cmd,
                              "ZZXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"],
                             env=None, path="/tmp",
                             usePTY=self.usePTY)

        def processEnded(ign):
            self.assertEquals(lsOut, p.errF.getvalue())
        return d.addCallback(processEnded)

    def testProcess(self):
        cmd = self.getCommand('gzip')
        s = "there's no place like home!\n" * 3
        p = Accumulator()
        d = p.endedDeferred = defer.Deferred()
        reactor.spawnProcess(p, cmd, [cmd, "-c"], env=None, path="/tmp",
                             usePTY=self.usePTY)
        p.transport.write(s)
        p.transport.closeStdin()

        def processEnded(ign):
            f = p.outF
            f.seek(0, 0)
            gf = gzip.GzipFile(fileobj=f)
            self.assertEquals(gf.read(), s)
        return d.addCallback(processEnded)



class PosixProcessTestCasePTY(unittest.TestCase, PosixProcessBase):
    """
    Just like PosixProcessTestCase, but use ptys instead of pipes.
    """
    usePTY = True
    # PTYs only offer one input and one output. What still makes sense?
    # testNormalTermination
    # test_abnormalTermination
    # testSignal
    # testProcess, but not without p.transport.closeStdin
    #  might be solveable: TODO: add test if so

    def testOpeningTTY(self):
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_tty.py")
        p = Accumulator()
        d = p.endedDeferred = defer.Deferred()
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=None,
                            path=None, usePTY=self.usePTY)
        p.transport.write("hello world!\n")

        def processEnded(ign):
            self.assertRaises(
                error.ProcessExitedAlready, p.transport.signalProcess, 'HUP')
            self.assertEquals(
                p.outF.getvalue(),
                "hello world!\r\nhello world!\r\n",
                "Error message from process_tty follows:\n\n%s\n\n" % p.outF.getvalue())
        return d.addCallback(processEnded)


    def testBadArgs(self):
        pyExe = sys.executable
        pyArgs = [pyExe, "-u", "-c", "print 'hello'"]
        p = Accumulator()
        self.assertRaises(ValueError, reactor.spawnProcess, p, pyExe, pyArgs,
            usePTY=1, childFDs={1:'r'})



class Win32SignalProtocol(SignalProtocol):
    """
    A win32-specific process protocol that handles C{processEnded}
    differently: processes should exit with exit code 1.
    """

    def processEnded(self, reason):
        """
        Callback C{self.deferred} with C{None} if C{reason} is a
        L{error.ProcessTerminated} failure with C{exitCode} set to 1.
        Otherwise, errback with a C{ValueError} describing the problem.
        """
        if not reason.check(error.ProcessTerminated):
            return self.deferred.errback(
                ValueError("wrong termination: %s" % (reason,)))
        v = reason.value
        if v.exitCode != 1:
            return self.deferred.errback(
                ValueError("Wrong exit code: %s" % (reason.exitCode,)))
        self.deferred.callback(None)



class Win32ProcessTestCase(unittest.TestCase):
    """
    Test process programs that are packaged with twisted.
    """

    def testStdinReader(self):
        pyExe = sys.executable
        scriptPath = util.sibpath(__file__, "process_stdinreader.py")
        p = Accumulator()
        d = p.endedDeferred = defer.Deferred()
        reactor.spawnProcess(p, pyExe, [pyExe, "-u", scriptPath], env=None,
                             path=None)
        p.transport.write("hello, world")
        p.transport.closeStdin()

        def processEnded(ign):
            self.assertEquals(p.errF.getvalue(), "err\nerr\n")
            self.assertEquals(p.outF.getvalue(), "out\nhello, world\nout\n")
        return d.addCallback(processEnded)


    def testBadArgs(self):
        pyExe = sys.executable
        pyArgs = [pyExe, "-u", "-c", "print 'hello'"]
        p = Accumulator()
        self.assertRaises(ValueError,
            reactor.spawnProcess, p, pyExe, pyArgs, uid=1)
        self.assertRaises(ValueError,
            reactor.spawnProcess, p, pyExe, pyArgs, gid=1)
        self.assertRaises(ValueError,
            reactor.spawnProcess, p, pyExe, pyArgs, usePTY=1)
        self.assertRaises(ValueError,
            reactor.spawnProcess, p, pyExe, pyArgs, childFDs={1:'r'})


    def _testSignal(self, sig):
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_signal.py")
        d = defer.Deferred()
        p = Win32SignalProtocol(d, sig)
        reactor.spawnProcess(p, exe, [exe, "-u", scriptPath], env=None)
        return d


    def test_signalTERM(self):
        """
        Sending the SIGTERM signal terminates a created process, and
        C{processEnded} is called with a L{error.ProcessTerminated} instance
        with the C{exitCode} attribute set to 1.
        """
        return self._testSignal('TERM')


    def test_signalINT(self):
        """
        Sending the SIGINT signal terminates a created process, and
        C{processEnded} is called with a L{error.ProcessTerminated} instance
        with the C{exitCode} attribute set to 1.
        """
        return self._testSignal('INT')


    def test_signalKILL(self):
        """
        Sending the SIGKILL signal terminates a created process, and
        C{processEnded} is called with a L{error.ProcessTerminated} instance
        with the C{exitCode} attribute set to 1.
        """
        return self._testSignal('KILL')



class Dumbwin32procPidTest(unittest.TestCase):
    """
    Simple test for the pid attribute of Process on win32.
    """

    def test_pid(self):
        """
        Launch process with mock win32process. The only mock aspect of this
        module is that the pid of the process created will always be 42.
        """
        from twisted.internet import _dumbwin32proc
        from twisted.test import mock_win32process
        self.patch(_dumbwin32proc, "win32process", mock_win32process)
        exe = sys.executable
        scriptPath = util.sibpath(__file__, "process_cmdline.py")

        d = defer.Deferred()
        processProto = TrivialProcessProtocol(d)
        comspec = str(os.environ["COMSPEC"])
        cmd = [comspec, "/c", exe, scriptPath]

        p = _dumbwin32proc.Process(reactor,
                                  processProto,
                                  None,
                                  cmd,
                                  {},
                                  None)
        self.assertEquals(42, p.pid)
        self.assertEquals("<Process pid=42>", repr(p))

        def pidCompleteCb(result):
            self.assertEquals(None, p.pid)
        return d.addCallback(pidCompleteCb)



class UtilTestCase(unittest.TestCase):
    """
    Tests for process-related helper functions (currently only
    L{procutils.which}.
    """
    def setUp(self):
        """
        Create several directories and files, some of which are executable
        and some of which are not.  Save the current PATH setting.
        """
        j = os.path.join

        base = self.mktemp()

        self.foo = j(base, "foo")
        self.baz = j(base, "baz")
        self.foobar = j(self.foo, "bar")
        self.foobaz = j(self.foo, "baz")
        self.bazfoo = j(self.baz, "foo")
        self.bazbar = j(self.baz, "bar")

        for d in self.foobar, self.foobaz, self.bazfoo, self.bazbar:
            os.makedirs(d)

        for name, mode in [(j(self.foobaz, "executable"), 0700),
                           (j(self.foo, "executable"), 0700),
                           (j(self.bazfoo, "executable"), 0700),
                           (j(self.bazfoo, "executable.bin"), 0700),
                           (j(self.bazbar, "executable"), 0)]:
            f = file(name, "w")
            f.close()
            os.chmod(name, mode)

        self.oldPath = os.environ.get('PATH', None)
        os.environ['PATH'] = os.pathsep.join((
            self.foobar, self.foobaz, self.bazfoo, self.bazbar))


    def tearDown(self):
        """
        Restore the saved PATH setting.
        """
        if self.oldPath is None:
            try:
                del os.environ['PATH']
            except KeyError:
                pass
        else:
            os.environ['PATH'] = self.oldPath


    def test_whichWithoutPATH(self):
        """
        Test that if C{os.environ} does not have a C{'PATH'} key,
        L{procutils.which} returns an empty list.
        """
        del os.environ['PATH']
        self.assertEqual(procutils.which("executable"), [])


    def testWhich(self):
        j = os.path.join
        paths = procutils.which("executable")
        expectedPaths = [j(self.foobaz, "executable"),
                         j(self.bazfoo, "executable")]
        if runtime.platform.isWindows():
            expectedPaths.append(j(self.bazbar, "executable"))
        self.assertEquals(paths, expectedPaths)


    def testWhichPathExt(self):
        j = os.path.join
        old = os.environ.get('PATHEXT', None)
        os.environ['PATHEXT'] = os.pathsep.join(('.bin', '.exe', '.sh'))
        try:
            paths = procutils.which("executable")
        finally:
            if old is None:
                del os.environ['PATHEXT']
            else:
                os.environ['PATHEXT'] = old
        expectedPaths = [j(self.foobaz, "executable"),
                         j(self.bazfoo, "executable"),
                         j(self.bazfoo, "executable.bin")]
        if runtime.platform.isWindows():
            expectedPaths.append(j(self.bazbar, "executable"))
        self.assertEquals(paths, expectedPaths)



class ClosingPipesProcessProtocol(protocol.ProcessProtocol):
    output = ''
    errput = ''

    def __init__(self, outOrErr):
        self.deferred = defer.Deferred()
        self.outOrErr = outOrErr

    def processEnded(self, reason):
        self.deferred.callback(reason)

    def outReceived(self, data):
        self.output += data

    def errReceived(self, data):
        self.errput += data


class ClosingPipes(unittest.TestCase):

    def doit(self, fd):
        p = ClosingPipesProcessProtocol(True)
        p.deferred.addCallbacks(
            callback=lambda _: self.fail("I wanted an errback."),
            errback=self._endProcess, errbackArgs=(p,))
        reactor.spawnProcess(p, sys.executable,
                             [sys.executable, '-u', '-c',
                              r'raw_input(); import sys, os; os.write(%d, "foo\n"); sys.exit(42)' % fd],
                             env=None)
        p.transport.write('go\n')

        if fd == 1:
            p.transport.closeStdout()
        elif fd == 2:
            p.transport.closeStderr()
        else:
            raise RuntimeError

        # make the buggy case not hang
        p.transport.closeStdin()
        return p.deferred

    def _endProcess(self, reason, p):
        self.failIf(reason.check(error.ProcessDone),
                    'Child should fail due to EPIPE.')
        reason.trap(error.ProcessTerminated)
        # child must not get past that write without raising
        self.failIfEqual(reason.value.exitCode, 42,
                         'process reason was %r' % reason)
        self.failUnlessEqual(p.output, '')
        return p.errput

    def test_stdout(self):
        """ProcessProtocol.transport.closeStdout actually closes the pipe."""
        d = self.doit(1)
        def _check(errput):
            self.failIfEqual(errput.find('OSError'), -1)
            if runtime.platform.getType() != 'win32':
                self.failIfEqual(errput.find('Broken pipe'), -1)
        d.addCallback(_check)
        return d

    def test_stderr(self):
        """ProcessProtocol.transport.closeStderr actually closes the pipe."""
        d = self.doit(2)
        def _check(errput):
            # there should be no stderr open, so nothing for it to
            # write the error to.
            self.failUnlessEqual(errput, '')
        d.addCallback(_check)
        return d


class SystemEventOrderRegressionTests(unittest.TestCase):
    """
    Ordering and reentrancy tests for C{reactor.callWhenRunning} and reactor
    shutdown (see #3146 and #3168).
    """
    def setUp(self):
        """
        Clear the SIGCHLD handler, if there is one, to ensure an environment
        like the one which exists prior to a call to L{reactor.run}.
        """
        self.originalHandler = signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        self.processTransports = []


    def tearDown(self):
        """
        Restore the original SIGCHLD handler and reap processes as long as
        there seem to be any remaining.
        """
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)
        while self.processTransports:
            transport = self.processTransports.pop()
            if transport.pid is not None:
                os.waitpid(transport.pid, 0)
        signal.signal(signal.SIGCHLD, self.originalHandler)


    def unbuildReactor(self, reactor):
        """
        Clean up any resources which may have been allocated for the given
        reactor by its creation or by a test which used it.
        """
        # Chris says:
        #
        # XXX This explicit calls to clean up the waker should become obsolete
        # when bug #3063 is fixed. -radix, 2008-02-29. Fortunately it should
        # probably cause an error when bug #3063 is fixed, so it should be
        # removed in the same branch that fixes it.
        #
        # -exarkun
        reactor.removeReader(reactor.waker)
        reactor.waker.connectionLost(None)

        # Here's an extra thing unrelated to wakers but necessary for
        # cleaning up after the reactors we make.  -exarkun
        reactor.disconnectAll()


    def buildReactor(self):
        """
        Create and return an instance of L{selectreactor.SelectReactor}.
        """
        reactor = selectreactor.SelectReactor()
        self.addCleanup(self.unbuildReactor, reactor)
        return reactor


    def spawnProcess(self, reactor):
        """
        Call C{reactor.spawnProcess} with some simple arguments.  Do this here
        so that code object referenced by the stack frame has a C{co_filename}
        attribute set to this file so that L{TestCase.assertWarns} can be used.
        """
        self.processTransports.append(
            reactor.spawnProcess(
                protocol.ProcessProtocol(), sys.executable,
                [sys.executable, "-c", ""]))


    def test_spawnProcessTooEarlyWarns(self):
        """
        C{reactor.spawnProcess} emits a warning if it is called before
        C{reactor.run}.

        If you can figure out a way to make it safe to run
        C{reactor.spawnProcess} before C{reactor.run}, you may delete the
        warning and this test.
        """
        reactor = self.buildReactor()
        self.assertWarns(
            error.PotentialZombieWarning,
            error.PotentialZombieWarning.MESSAGE, __file__,
            self.spawnProcess, reactor)


    def test_callWhenRunningSpawnProcessWarningFree(self):
        """
        L{PotentialZombieWarning} is not emitted when the reactor is run after
        C{reactor.callWhenRunning(reactor.spawnProcess, ...)} has been called.
        """
        events = []
        self.patch(warnings, 'warn', lambda *a, **kw: events.append(a))
        reactor = self.buildReactor()
        reactor.callWhenRunning(self.spawnProcess, reactor)
        reactor.callWhenRunning(reactor.stop)
        reactor.run()
        self.assertFalse(events)


    def test_clientConnectionFailedStopsReactor(self):
        """
        The reactor can be stopped by a client factory's
        C{clientConnectionFailed} method.

        This isn't really a process test but it's here for simplicity of
        implementation and it won't be very long lived.
        """
        class Stop(protocol.ClientFactory):
            def clientConnectionFailed(self, connector, reason):
                reactor.stop()
        probe = socket.socket()
        probe.bind(('', 0))
        host, port = probe.getsockname()
        probe.close()
        reactor = self.buildReactor()
        reactor.connectTCP(host, port, Stop())
        reactor.run()


    def test_shutdownTriggersRun(self):
        """
        C{reactor.run()} does not return until shutdown triggers have all run.
        """
        events = []
        reactor = self.buildReactor()
        reactor.addSystemEventTrigger(
            'after', 'shutdown', events.append, "done")
        reactor.callWhenRunning(reactor.stop)
        reactor.run()
        self.assertEqual(events, ["done"])



skipMessage = "wrong platform or reactor doesn't support IReactorProcess"
if (runtime.platform.getType() != 'posix') or (not interfaces.IReactorProcess(reactor, None)):
    PosixProcessTestCase.skip = skipMessage
    PosixProcessTestCasePTY.skip = skipMessage
    TestTwoProcessesPosix.skip = skipMessage
    FDTest.skip = skipMessage
else:
    # do this before running the tests: it uses SIGCHLD and stuff internally
    lsOut = popen2.popen3("/bin/ls ZZXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")[2].read()

if (runtime.platform.getType() != 'win32') or (not interfaces.IReactorProcess(reactor, None)):
    Win32ProcessTestCase.skip = skipMessage
    TestTwoProcessesNonPosix.skip = skipMessage
    Dumbwin32procPidTest.skip = skipMessage

if not interfaces.IReactorProcess(reactor, None):
    ProcessTestCase.skip = skipMessage
    ClosingPipes.skip = skipMessage

if process is None:
    MockProcessTestCase.skip = skipMessage
    SystemEventOrderRegressionTests.skip = skipMessage

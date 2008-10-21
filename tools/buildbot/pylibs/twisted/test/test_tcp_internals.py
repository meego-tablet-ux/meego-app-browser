# Copyright (c) 2006 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Whitebox tests for TCP APIs.
"""

import errno, socket, os

try:
    import resource
except ImportError:
    resource = None

from twisted.trial.unittest import TestCase

from twisted.python import log
from twisted.internet.tcp import ECONNABORTED, ENOMEM, ENFILE, EMFILE, ENOBUFS, EINPROGRESS, Port
from twisted.internet.protocol import ServerFactory
from twisted.python.runtime import platform
from twisted.internet.defer import maybeDeferred, gatherResults
from twisted.internet import reactor, interfaces


class PlatformAssumptionsTestCase(TestCase):
    """
    Test assumptions about platform behaviors.
    """
    socketLimit = 8192

    def setUp(self):
        self.openSockets = []
        if resource is not None:
            self.originalFileLimit = resource.getrlimit(resource.RLIMIT_NOFILE)
            resource.setrlimit(resource.RLIMIT_NOFILE, (128, self.originalFileLimit[1]))
            self.socketLimit = 256


    def tearDown(self):
        while self.openSockets:
            self.openSockets.pop().close()
        if resource is not None:
            # OS X implicitly lowers the hard limit in the setrlimit call
            # above.  Retrieve the new hard limit to pass in to this
            # setrlimit call, so that it doesn't give us a permission denied
            # error.
            currentHardLimit = resource.getrlimit(resource.RLIMIT_NOFILE)[1]
            newSoftLimit = min(self.originalFileLimit[0], currentHardLimit)
            resource.setrlimit(resource.RLIMIT_NOFILE, (newSoftLimit, currentHardLimit))


    def socket(self):
        """
        Create and return a new socket object, also tracking it so it can be
        closed in the test tear down.
        """
        s = socket.socket()
        self.openSockets.append(s)
        return s


    def test_acceptOutOfFiles(self):
        """
        Test that the platform accept(2) call fails with either L{EMFILE} or
        L{ENOBUFS} when there are too many file descriptors open.
        """
        # Make a server to which to connect
        port = self.socket()
        port.bind(('127.0.0.1', 0))
        serverPortNumber = port.getsockname()[1]
        port.listen(5)

        # Use up all the file descriptors
        for i in xrange(self.socketLimit):
            try:
                self.socket()
            except socket.error, e:
                if e.args[0] in (EMFILE, ENOBUFS):
                    self.openSockets.pop().close()
                    break
                else:
                    raise
        else:
            self.fail("Could provoke neither EMFILE nor ENOBUFS from platform.")

        # Make a client to use to connect to the server
        client = self.socket()
        client.setblocking(False)

        # Non-blocking connect is supposed to fail, but this is not true
        # everywhere (e.g. freeBSD)
        self.assertIn(client.connect_ex(('127.0.0.1', serverPortNumber)),
                      (0, EINPROGRESS))

        # Make sure that the accept call fails in the way we expect.
        exc = self.assertRaises(socket.error, port.accept)
        self.assertIn(exc.args[0], (EMFILE, ENOBUFS))
    if platform.getType() == "win32":
        test_acceptOutOfFiles.skip = (
            "Windows requires an unacceptably large amount of resources to "
            "provoke this behavior in the naive manner.")



class SelectReactorTestCase(TestCase):
    """
    Tests for select-specific failure conditions.
    """

    def setUp(self):
        self.ports = []
        self.messages = []
        log.addObserver(self.messages.append)


    def tearDown(self):
        log.removeObserver(self.messages.append)
        return gatherResults([
            maybeDeferred(p.stopListening)
            for p in self.ports])


    def port(self, portNumber, factory, interface):
        """
        Create, start, and return a new L{Port}, also tracking it so it can
        be stopped in the test tear down.
        """
        p = Port(portNumber, factory, interface=interface)
        p.startListening()
        self.ports.append(p)
        return p


    def _acceptFailureTest(self, socketErrorNumber):
        """
        Test behavior in the face of an exception from C{accept(2)}.

        On any exception which indicates the platform is unable or unwilling
        to allocate further resources to us, the existing port should remain
        listening, a message should be logged, and the exception should not
        propagate outward from doRead.

        @param socketErrorNumber: The errno to simulate from accept.
        """
        class FakeSocket(object):
            """
            Pretend to be a socket in an overloaded system.
            """
            def accept(self):
                raise socket.error(
                    socketErrorNumber, os.strerror(socketErrorNumber))

        factory = ServerFactory()
        port = self.port(0, factory, interface='127.0.0.1')
        originalSocket = port.socket
        try:
            port.socket = FakeSocket()

            port.doRead()

            expectedFormat = "Could not accept new connection (%s)"
            expectedErrorCode = errno.errorcode[socketErrorNumber]
            expectedMessage = expectedFormat % (expectedErrorCode,)
            for msg in self.messages:
                if msg.get('message') == (expectedMessage,):
                    break
            else:
                self.fail("Log event for failed accept not found in "
                          "%r" % (self.messages,))
        finally:
            port.socket = originalSocket


    def test_tooManyFilesFromAccept(self):
        """
        C{accept(2)} can fail with C{EMFILE} when there are too many open file
        descriptors in the process.  Test that this doesn't negatively impact
        any other existing connections.

        C{EMFILE} mainly occurs on Linux when the open file rlimit is
        encountered.
        """
        return self._acceptFailureTest(EMFILE)


    def test_noBufferSpaceFromAccept(self):
        """
        Similar to L{test_tooManyFilesFromAccept}, but test the case where
        C{accept(2)} fails with C{ENOBUFS}.

        This mainly occurs on Windows and FreeBSD, but may be possible on
        Linux and other platforms as well.
        """
        return self._acceptFailureTest(ENOBUFS)


    def test_connectionAbortedFromAccept(self):
        """
        Similar to L{test_tooManyFilesFromAccept}, but test the case where
        C{accept(2)} fails with C{ECONNABORTED}.

        It is not clear whether this is actually possible for TCP
        connections on modern versions of Linux.
        """
        return self._acceptFailureTest(ECONNABORTED)


    def test_noFilesFromAccept(self):
        """
        Similar to L{test_tooManyFilesFromAccept}, but test the case where
        C{accept(2)} fails with C{ENFILE}.

        This can occur on Linux when the system has exhausted (!) its supply
        of inodes.
        """
        return self._acceptFailureTest(ENFILE)
    if platform.getType() == 'win32':
        test_noFilesFromAccept.skip = "Windows accept(2) cannot generate ENFILE"


    def test_noMemoryFromAccept(self):
        """
        Similar to L{test_tooManyFilesFromAccept}, but test the case where
        C{accept(2)} fails with C{ENOMEM}.

        On Linux at least, this can sensibly occur, even in a Python program
        (which eats memory like no ones business), when memory has become
        fragmented or low memory has been filled (d_alloc calls
        kmem_cache_alloc calls kmalloc - kmalloc only allocates out of low
        memory).
        """
        return self._acceptFailureTest(ENOMEM)
    if platform.getType() == 'win32':
        test_noMemoryFromAccept.skip = "Windows accept(2) cannot generate ENOMEM"

if not interfaces.IReactorFDSet.providedBy(reactor):
    skipMsg = 'This test only applies to reactors that implement IReactorFDset'
    PlatformAssumptionsTestCase.skip = skipMsg
    SelectReactorTestCase.skip = skipMsg


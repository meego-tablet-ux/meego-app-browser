# -*- test-case-name: twisted.test.test_udp -*-
# Copyright (c) 2001-2004 Twisted Matrix Laboratories.
# See LICENSE for details.

#
from twisted.trial import unittest, util

from twisted.internet import protocol, reactor, error, defer, interfaces, address
from twisted.python import log, failure, components, runtime


class Mixin:

    started = 0
    stopped = 0

    startedDeferred = None

    def __init__(self):
        self.packets = []

    def startProtocol(self):
        self.started = 1
        if self.startedDeferred is not None:
            d, self.startedDeferred = self.startedDeferred, None
            d.callback(None)

    def stopProtocol(self):
        self.stopped = 1


class Server(Mixin, protocol.DatagramProtocol):

    packetReceived = None
    refused = 0

    def datagramReceived(self, data, addr):
        self.packets.append((data, addr))
        if self.packetReceived is not None:
            d, self.packetReceived = self.packetReceived, None
            d.callback(None)



class Client(Mixin, protocol.ConnectedDatagramProtocol):

    packetReceived = None
    refused = 0

    def datagramReceived(self, data):
        self.packets.append(data)
        if self.packetReceived is not None:
            d, self.packetReceived = self.packetReceived, None
            d.callback(None)

    def connectionFailed(self, failure):
        if self.startedDeferred is not None:
            d, self.startedDeferred = self.startedDeferred, None
            d.errback(failure)
        self.failure = failure

    def connectionRefused(self):
        if self.startedDeferred is not None:
            d, self.startedDeferred = self.startedDeferred, None
            d.errback(error.ConnectionRefusedError("yup"))
        self.refused = 1


class GoodClient(Server):

    def connectionRefused(self):
        if self.startedDeferred is not None:
            d, self.startedDeferred = self.startedDeferred, None
            d.errback(error.ConnectionRefusedError("yup"))
        self.refused = 1



class BadClientError(Exception):
    """
    Raised by BadClient at the end of every datagramReceived call to try and
    screw stuff up.
    """



class BadClient(protocol.DatagramProtocol):
    """
    A DatagramProtocol which always raises an exception from datagramReceived.
    Used to test error handling behavior in the reactor for that method.
    """
    d = None

    def setDeferred(self, d):
        """
        Set the Deferred which will be called back when datagramReceived is
        called.
        """
        self.d = d


    def datagramReceived(self, bytes, addr):
        if self.d is not None:
            d, self.d = self.d, None
            d.callback(bytes)
        raise BadClientError("Application code is very buggy!")



class PortCleanerUpper(unittest.TestCase):
    callToLoseCnx = 'loseConnection'
    def setUp(self):
        self.ports = []

    def tearDown(self):
        return self.cleanPorts(*self.ports)

    def _addPorts(self, *ports):
        for p in ports:
            self.ports.append(p)

    def cleanPorts(self, *ports):
        for p in ports:
            if not hasattr(p, 'disconnected'):
                raise RuntimeError(
                    "You handed something to cleanPorts that"
                    " doesn't have a disconnected attribute, dummy!")
        ds = [ defer.maybeDeferred(getattr(p, self.callToLoseCnx))
               for p in ports if not p.disconnected ]
        return defer.gatherResults(ds)


class OldConnectedUDPTestCase(PortCleanerUpper):
    def testStartStop(self):
        client = Client()
        d = client.startedDeferred = defer.Deferred()
        port2 = reactor.connectUDP("127.0.0.1", 8888, client)

        def assertName():
            self.failUnless(repr(port2).find('test_udp.Client') >= 0)

        def cbStarted(ignored):
            self.assertEquals(client.started, 1)
            self.assertEquals(client.stopped, 0)
            assertName()
            d = defer.maybeDeferred(port2.stopListening)
            d.addCallback(lambda ign: assertName())
            return d

        return d.addCallback(cbStarted)
    testStartStop.suppress = [
        util.suppress(message='use listenUDP and then transport.connect',
                      category=DeprecationWarning)]


    def testDNSFailure(self):
        client = Client()
        d = client.startedDeferred = defer.Deferred()
        # if this domain exists, shoot your sysadmin
        reactor.connectUDP("xxxxxxxxx.zzzzzzzzz.yyyyy.", 8888, client)

        def didNotConnect(ign):
            self.assertEquals(client.stopped, 0)
            self.assertEquals(client.started, 0)

        d = self.assertFailure(d, error.DNSLookupError)
        d.addCallback(didNotConnect)
        return d
    testDNSFailure.suppress = [
        util.suppress(message='use listenUDP and then transport.connect',
                      category=DeprecationWarning)]


    def testSendPackets(self):
        server = Server()
        serverStarted = server.startedDeferred = defer.Deferred()

        client = Client()
        clientStarted = client.startedDeferred = defer.Deferred()

        port1 = reactor.listenUDP(0, server, interface="127.0.0.1")

        def cbServerStarted(ignored):
            self.port2 = reactor.connectUDP("127.0.0.1",
                                            server.transport.getHost().port,
                                            client)
            return clientStarted

        d = serverStarted.addCallback(cbServerStarted)

        def cbClientStarted(ignored):
            clientSend = server.packetReceived = defer.Deferred()
            serverSend = client.packetReceived = defer.Deferred()

            cAddr = client.transport.getHost()
            server.transport.write("hello", (cAddr.host, cAddr.port))
            client.transport.write("world")

            # No one will ever call errback on either of these Deferreds,
            # otherwise I would pass fireOnOneErrback=True here.
            return defer.DeferredList([clientSend, serverSend])

        d.addCallback(cbClientStarted)

        def cbPackets(ignored):
            self.assertEquals(client.packets, ["hello"])
            self.assertEquals(server.packets,
                              [("world", ("127.0.0.1",
                                          client.transport.getHost().port))])

            return defer.DeferredList([
                defer.maybeDeferred(port1.stopListening),
                defer.maybeDeferred(self.port2.stopListening)],
                fireOnOneErrback=True)

        d.addCallback(cbPackets)
        return d
    testSendPackets.suppress = [
        util.suppress(message='use listenUDP and then transport.connect',
                      category=DeprecationWarning)]


    def testConnectionRefused(self):
        # assume no one listening on port 80 UDP
        client = Client()
        port = reactor.connectUDP("127.0.0.1", 80, client)
        server = Server()
        port2 = reactor.listenUDP(0, server, interface="127.0.0.1")
        reactor.iterate()
        reactor.iterate()
        reactor.iterate()
        client.transport.write("a")
        client.transport.write("b")
        server.transport.write("c", ("127.0.0.1", 80))
        server.transport.write("d", ("127.0.0.1", 80))
        server.transport.write("e", ("127.0.0.1", 80))
        server.transport.write("toserver", (port2.getHost().host,
                                            port2.getHost().port))
        server.transport.write("toclient", (port.getHost().host,
                                            port.getHost().port))
        reactor.iterate(); reactor.iterate()
        self.assertEquals(client.refused, 1)
        port.stopListening()
        port2.stopListening()
        reactor.iterate(); reactor.iterate()
    testConnectionRefused.suppress = [
        util.suppress(message='use listenUDP and then transport.connect',
                      category=DeprecationWarning)]



class UDPTestCase(unittest.TestCase):

    def testOldAddress(self):
        server = Server()
        d = server.startedDeferred = defer.Deferred()
        p = reactor.listenUDP(0, server, interface="127.0.0.1")
        def cbStarted(ignored):
            addr = p.getHost()
            self.assertEquals(addr, ('INET_UDP', addr.host, addr.port))
            return p.stopListening()
        return d.addCallback(cbStarted)
    testOldAddress.suppress = [
        util.suppress(message='IPv4Address.__getitem__',
                      category=DeprecationWarning)]


    def testStartStop(self):
        server = Server()
        d = server.startedDeferred = defer.Deferred()
        port1 = reactor.listenUDP(0, server, interface="127.0.0.1")
        def cbStarted(ignored):
            self.assertEquals(server.started, 1)
            self.assertEquals(server.stopped, 0)
            return port1.stopListening()
        def cbStopped(ignored):
            self.assertEquals(server.stopped, 1)
        return d.addCallback(cbStarted).addCallback(cbStopped)

    def testRebind(self):
        # Ensure binding the same DatagramProtocol repeatedly invokes all
        # the right callbacks.
        server = Server()
        d = server.startedDeferred = defer.Deferred()
        p = reactor.listenUDP(0, server, interface="127.0.0.1")

        def cbStarted(ignored, port):
            return port.stopListening()

        def cbStopped(ignored):
            d = server.startedDeferred = defer.Deferred()
            p = reactor.listenUDP(0, server, interface="127.0.0.1")
            return d.addCallback(cbStarted, p)

        return d.addCallback(cbStarted, p)


    def testBindError(self):
        server = Server()
        d = server.startedDeferred = defer.Deferred()
        port = reactor.listenUDP(0, server, interface='127.0.0.1')

        def cbStarted(ignored):
            self.assertEquals(port.getHost(), server.transport.getHost())

            server2 = Server()
            self.assertRaises(
                error.CannotListenError,
                reactor.listenUDP, port.getHost().port, server2,
                interface='127.0.0.1')
        d.addCallback(cbStarted)

        def cbFinished(ignored):
            return port.stopListening()
        d.addCallback(cbFinished)
        return d

    def testSendPackets(self):
        server = Server()
        serverStarted = server.startedDeferred = defer.Deferred()
        port1 = reactor.listenUDP(0, server, interface="127.0.0.1")

        client = GoodClient()
        clientStarted = client.startedDeferred = defer.Deferred()

        def cbServerStarted(ignored):
            self.port2 = reactor.listenUDP(0, client, interface="127.0.0.1")
            return clientStarted

        d = serverStarted.addCallback(cbServerStarted)

        def cbClientStarted(ignored):
            client.transport.connect("127.0.0.1",
                                     server.transport.getHost().port)
            cAddr = client.transport.getHost()
            sAddr = server.transport.getHost()

            serverSend = client.packetReceived = defer.Deferred()
            server.transport.write("hello", (cAddr.host, cAddr.port))

            clientWrites = [
                ("a",),
                ("b", None),
                ("c", (sAddr.host, sAddr.port))]

            def cbClientSend(ignored):
                if clientWrites:
                    nextClientWrite = server.packetReceived = defer.Deferred()
                    nextClientWrite.addCallback(cbClientSend)
                    client.transport.write(*clientWrites.pop(0))
                    return nextClientWrite

            # No one will ever call .errback on either of these Deferreds,
            # but there is a non-trivial amount of test code which might
            # cause them to fail somehow.  So fireOnOneErrback=True.
            return defer.DeferredList([
                cbClientSend(None),
                serverSend],
                fireOnOneErrback=True)

        d.addCallback(cbClientStarted)

        def cbSendsFinished(ignored):
            cAddr = client.transport.getHost()
            sAddr = server.transport.getHost()
            self.assertEquals(
                client.packets,
                [("hello", (sAddr.host, sAddr.port))])
            clientAddr = (cAddr.host, cAddr.port)
            self.assertEquals(
                server.packets,
                [("a", clientAddr),
                 ("b", clientAddr),
                 ("c", clientAddr)])

        d.addCallback(cbSendsFinished)

        def cbFinished(ignored):
            return defer.DeferredList([
                defer.maybeDeferred(port1.stopListening),
                defer.maybeDeferred(self.port2.stopListening)],
                fireOnOneErrback=True)

        d.addCallback(cbFinished)
        return d


    def testConnectionRefused(self):
        # assume no one listening on port 80 UDP
        client = GoodClient()
        clientStarted = client.startedDeferred = defer.Deferred()
        port = reactor.listenUDP(0, client, interface="127.0.0.1")

        server = Server()
        serverStarted = server.startedDeferred = defer.Deferred()
        port2 = reactor.listenUDP(0, server, interface="127.0.0.1")

        d = defer.DeferredList(
            [clientStarted, serverStarted],
            fireOnOneErrback=True)

        def cbStarted(ignored):
            connectionRefused = client.startedDeferred = defer.Deferred()
            client.transport.connect("127.0.0.1", 80)

            for i in range(10):
                client.transport.write(str(i))
                server.transport.write(str(i), ("127.0.0.1", 80))

            return self.assertFailure(
                connectionRefused,
                error.ConnectionRefusedError)

        d.addCallback(cbStarted)

        def cbFinished(ignored):
            return defer.DeferredList([
                defer.maybeDeferred(port.stopListening),
                defer.maybeDeferred(port2.stopListening)],
                fireOnOneErrback=True)

        d.addCallback(cbFinished)
        return d

    def testBadConnect(self):
        client = GoodClient()
        port = reactor.listenUDP(0, client, interface="127.0.0.1")
        self.assertRaises(ValueError, client.transport.connect,
                          "localhost", 80)
        client.transport.connect("127.0.0.1", 80)
        self.assertRaises(RuntimeError, client.transport.connect,
                          "127.0.0.1", 80)
        return port.stopListening()



    def testDatagramReceivedError(self):
        """
        Test that when datagramReceived raises an exception it is logged but
        the port is not disconnected.
        """
        finalDeferred = defer.Deferred()

        def cbCompleted(ign):
            """
            Flush the exceptions which the reactor should have logged and make
            sure they're actually there.
            """
            errs = log.flushErrors(BadClientError)
            self.assertEquals(len(errs), 2, "Incorrectly found %d errors, expected 2" % (len(errs),))
        finalDeferred.addCallback(cbCompleted)

        client = BadClient()
        port = reactor.listenUDP(0, client, interface='127.0.0.1')

        def cbCleanup(result):
            """
            Disconnect the port we started and pass on whatever was given to us
            in case it was a Failure.
            """
            return defer.maybeDeferred(port.stopListening).addBoth(lambda ign: result)
        finalDeferred.addBoth(cbCleanup)

        addr = port.getHost()

        # UDP is not reliable.  Try to send as many as 60 packets before giving
        # up.  Conceivably, all sixty could be lost, but they probably won't be
        # unless all UDP traffic is being dropped, and then the rest of these
        # UDP tests will likely fail as well.  Ideally, this test (and probably
        # others) wouldn't even use actual UDP traffic: instead, they would
        # stub out the socket with a fake one which could be made to behave in
        # whatever way the test desires.  Unfortunately, this is hard because
        # of differences in various reactor implementations.
        attempts = range(60)
        succeededAttempts = []

        def makeAttempt():
            """
            Send one packet to the listening BadClient.  Set up a 0.1 second
            timeout to do re-transmits in case the packet is dropped.  When two
            packets have been received by the BadClient, stop sending and let
            the finalDeferred's callbacks do some assertions.
            """
            if not attempts:
                try:
                    self.fail("Not enough packets received")
                except:
                    finalDeferred.errback()

            self.failIfIdentical(client.transport, None, "UDP Protocol lost its transport")

            packet = str(attempts.pop(0))
            packetDeferred = defer.Deferred()
            client.setDeferred(packetDeferred)
            client.transport.write(packet, (addr.host, addr.port))

            def cbPacketReceived(packet):
                """
                A packet arrived.  Cancel the timeout for it, record it, and
                maybe finish the test.
                """
                timeoutCall.cancel()
                succeededAttempts.append(packet)
                if len(succeededAttempts) == 2:
                    # The second error has not yet been logged, since the
                    # exception which causes it hasn't even been raised yet.
                    # Give the datagramReceived call a chance to finish, then
                    # let the test finish asserting things.
                    reactor.callLater(0, finalDeferred.callback, None)
                else:
                    makeAttempt()

            def ebPacketTimeout(err):
                """
                The packet wasn't received quickly enough.  Try sending another
                one.  It doesn't matter if the packet for which this was the
                timeout eventually arrives: makeAttempt throws away the
                Deferred on which this function is the errback, so when
                datagramReceived callbacks, so it won't be on this Deferred, so
                it won't raise an AlreadyCalledError.
                """
                makeAttempt()

            packetDeferred.addCallbacks(cbPacketReceived, ebPacketTimeout)
            packetDeferred.addErrback(finalDeferred.errback)

            timeoutCall = reactor.callLater(
                0.1, packetDeferred.errback,
                error.TimeoutError(
                    "Timed out in testDatagramReceivedError"))

        makeAttempt()
        return finalDeferred


    def testPortRepr(self):
        client = GoodClient()
        p = reactor.listenUDP(0, client)
        portNo = str(p.getHost().port)
        self.failIf(repr(p).find(portNo) == -1)
        def stoppedListening(ign):
            self.failIf(repr(p).find(portNo) != -1)
        d = defer.maybeDeferred(p.stopListening)
        d.addCallback(stoppedListening)
        return d


class ReactorShutdownInteraction(unittest.TestCase):

    def testShutdownFromDatagramReceived(self):

        # udp.Port's doRead calls recvfrom() in a loop, as an optimization.
        # It is important this loop terminate under various conditions.
        # Previously, if datagramReceived synchronously invoked
        # reactor.stop(), under certain reactors, the Port's socket would
        # synchronously disappear, causing an AttributeError inside that
        # loop.  This was mishandled, causing the loop to spin forever.
        # This test is primarily to ensure that the loop never spins
        # forever.

        server = Server()
        finished = defer.Deferred()
        p = reactor.listenUDP(0, server, interface='127.0.0.1')
        pr = server.packetReceived = defer.Deferred()

        def pktRece(ignored):
            # Simulate reactor.stop() behavior :(
            server.transport.connectionLost()
            # Then delay this Deferred chain until the protocol has been
            # disconnected, as the reactor should do in an error condition
            # such as we are inducing.  This is very much a whitebox test.
            reactor.callLater(0, finished.callback, None)
        pr.addCallback(pktRece)

        def flushErrors(ignored):
            # We are breaking abstraction and calling private APIs, any
            # number of horrible errors might occur.  As long as the reactor
            # doesn't hang, this test is satisfied.  (There may be room for
            # another, stricter test.)
            log.flushErrors()
        finished.addCallback(flushErrors)
        server.transport.write('\0' * 64, ('127.0.0.1',
                                           server.transport.getHost().port))
        return finished



class MulticastTestCase(unittest.TestCase):

    def _resultSet(self, result, l):
        l.append(result)

    def runUntilSuccess(self, method, *args, **kwargs):
        l = []
        d = method(*args, **kwargs)
        d.addCallback(self._resultSet, l).addErrback(self._resultSet, l)
        while not l:
            reactor.iterate()
        if isinstance(l[0], failure.Failure):
            raise l[0].value

    def setUp(self):
        self.server = Server()
        self.client = Client()
        # multicast won't work if we listen over loopback, apparently
        self.port1 = reactor.listenMulticast(0, self.server)
        self.port2 = reactor.listenMulticast(0, self.client)
        reactor.iterate()
        reactor.iterate()
        self.client.transport.connect("127.0.0.1",
                                      self.server.transport.getHost().port)

    def tearDown(self):
        self.port1.stopListening()
        self.port2.stopListening()
        del self.server
        del self.client
        del self.port1
        del self.port2
        reactor.iterate()
        reactor.iterate()

    def testTTL(self):
        for o in self.client, self.server:
            self.assertEquals(o.transport.getTTL(), 1)
            o.transport.setTTL(2)
            self.assertEquals(o.transport.getTTL(), 2)

    def testLoopback(self):
        self.assertEquals(self.server.transport.getLoopbackMode(), 1)
        self.runUntilSuccess(self.server.transport.joinGroup, "225.0.0.250")
        self.server.transport.write("hello",
                                    ("225.0.0.250",
                                     self.server.transport.getHost().port))
        reactor.iterate()
        self.assertEquals(len(self.server.packets), 1)
        self.server.transport.setLoopbackMode(0)
        self.assertEquals(self.server.transport.getLoopbackMode(), 0)
        self.server.transport.write("hello",
                                    ("225.0.0.250",
                                     self.server.transport.getHost().port))
        reactor.iterate()
        self.assertEquals(len(self.server.packets), 1)

    def testInterface(self):
        for o in self.client, self.server:
            self.assertEquals(o.transport.getOutgoingInterface(), "0.0.0.0")
            self.runUntilSuccess(o.transport.setOutgoingInterface, "127.0.0.1")
            self.assertEquals(o.transport.getOutgoingInterface(), "127.0.0.1")

    def testJoinLeave(self):
        for o in self.client, self.server:
            self.runUntilSuccess(o.transport.joinGroup, "225.0.0.250")
            self.runUntilSuccess(o.transport.leaveGroup, "225.0.0.250")


    def testJoinFailure(self):
        # 127.0.0.1 is not a multicast address, so joining it should fail.
        return self.assertFailure(
            self.client.transport.joinGroup("127.0.0.1"),
            error.MulticastJoinError)

    if runtime.platform.isWindows():
        testJoinFailure.todo = "Windows' multicast is wonky"


    def testMulticast(self):
        c = Server()
        p = reactor.listenMulticast(0, c)
        self.runUntilSuccess(self.server.transport.joinGroup, "225.0.0.250")
        c.transport.write("hello world",
                          ("225.0.0.250",
                           self.server.transport.getHost().port))

        iters = 0
        while iters < 100 and len(self.server.packets) == 0:
            reactor.iterate(0.05);
            iters += 1
        self.assertEquals(self.server.packets[0][0], "hello world")
        p.stopListening()

    def testMultiListen(self):
        c = Server()
        p = reactor.listenMulticast(0, c, listenMultiple=True)
        self.runUntilSuccess(self.server.transport.joinGroup, "225.0.0.250")
        portno = p.getHost().port
        c2 = Server()
        p2 = reactor.listenMulticast(portno, c2, listenMultiple=True)
        self.runUntilSuccess(self.server.transport.joinGroup, "225.0.0.250")
        c.transport.write("hello world", ("225.0.0.250", portno))
        d = defer.Deferred()
        reactor.callLater(0.4, d.callback, None, c, c2, p, p2)
        return d

    def _cbTestMultiListen(self, ignored, c, c2, p, p2):
        self.assertEquals(c.packets[0][0], "hello world")
        self.assertEquals(c2.packets[0][0], "hello world")
        p.stopListening()
        p2.stopListening()

    testMultiListen.skip = ("on non-linux platforms it appears multiple "
                            "processes can listen, but not multiple sockets "
                            "in same process?")

if not interfaces.IReactorUDP(reactor, None):
    UDPTestCase.skip = "This reactor does not support UDP"
if not hasattr(reactor, "connectUDP"):
    OldConnectedUDPTestCase.skip = "This reactor does not support connectUDP"
if not interfaces.IReactorMulticast(reactor, None):
    MulticastTestCase.skip = "This reactor does not support multicast"

def checkForLinux22():
    import os
    if os.path.exists("/proc/version"):
        s = open("/proc/version").read()
        if s.startswith("Linux version"):
            s = s.split()[2]
            if s.split(".")[:2] == ["2", "2"]:
                f = MulticastTestCase.testInterface.im_func
                f.todo = "figure out why this fails in linux 2.2"
checkForLinux22()

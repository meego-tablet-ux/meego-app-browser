# Copyright (c) 2001-2007 Twisted Matrix Laboratories.
# See LICENSE for details.


"""
Test cases for twisted.protocols.ident module.
"""

import struct

from twisted.protocols import ident
from twisted.python import failure
from twisted.internet import error
from twisted.internet import defer

from twisted.trial import unittest
from twisted.test.proto_helpers import StringTransport



class ClassParserTestCase(unittest.TestCase):
    """
    Test parsing of ident responses.
    """

    def setUp(self):
        """
        Create a ident client used in tests.
        """
        self.client = ident.IdentClient()


    def test_indentError(self):
        """
        'UNKNOWN-ERROR' error should map to the L{ident.IdentError} exception.
        """
        d = defer.Deferred()
        self.client.queries.append((d, 123, 456))
        self.client.lineReceived('123, 456 : ERROR : UNKNOWN-ERROR')
        return self.assertFailure(d, ident.IdentError)


    def test_noUSerError(self):
        """
        'NO-USER' error should map to the L{ident.NoUser} exception.
        """
        d = defer.Deferred()
        self.client.queries.append((d, 234, 456))
        self.client.lineReceived('234, 456 : ERROR : NO-USER')
        return self.assertFailure(d, ident.NoUser)


    def test_invalidPortError(self):
        """
        'INVALID-PORT' error should map to the L{ident.InvalidPort} exception.
        """
        d = defer.Deferred()
        self.client.queries.append((d, 345, 567))
        self.client.lineReceived('345, 567 :  ERROR : INVALID-PORT')
        return self.assertFailure(d, ident.InvalidPort)


    def test_hiddenUserError(self):
        """
        'HIDDEN-USER' error should map to the L{ident.HiddenUser} exception.
        """
        d = defer.Deferred()
        self.client.queries.append((d, 567, 789))
        self.client.lineReceived('567, 789 : ERROR : HIDDEN-USER')
        return self.assertFailure(d, ident.HiddenUser)


    def test_lostConnection(self):
        """
        A pending query which failed because of a ConnectionLost should
        receive an L{ident.IdentError}.
        """
        d = defer.Deferred()
        self.client.queries.append((d, 765, 432))
        self.client.connectionLost(failure.Failure(error.ConnectionLost()))
        return self.assertFailure(d, ident.IdentError)



class TestIdentServer(ident.IdentServer):
    def lookup(self, serverAddress, clientAddress):
        return self.resultValue


class TestErrorIdentServer(ident.IdentServer):
    def lookup(self, serverAddress, clientAddress):
        raise self.exceptionType()


class NewException(RuntimeError):
    pass


class ServerParserTestCase(unittest.TestCase):
    def testErrors(self):
        p = TestErrorIdentServer()
        p.makeConnection(StringTransport())
        L = []
        p.sendLine = L.append

        p.exceptionType = ident.IdentError
        p.lineReceived('123, 345')
        self.assertEquals(L[0], '123, 345 : ERROR : UNKNOWN-ERROR')

        p.exceptionType = ident.NoUser
        p.lineReceived('432, 210')
        self.assertEquals(L[1], '432, 210 : ERROR : NO-USER')

        p.exceptionType = ident.InvalidPort
        p.lineReceived('987, 654')
        self.assertEquals(L[2], '987, 654 : ERROR : INVALID-PORT')

        p.exceptionType = ident.HiddenUser
        p.lineReceived('756, 827')
        self.assertEquals(L[3], '756, 827 : ERROR : HIDDEN-USER')

        p.exceptionType = NewException
        p.lineReceived('987, 789')
        self.assertEquals(L[4], '987, 789 : ERROR : UNKNOWN-ERROR')
        errs = self.flushLoggedErrors(NewException)
        self.assertEquals(len(errs), 1)

        for port in -1, 0, 65536, 65537:
            del L[:]
            p.lineReceived('%d, 5' % (port,))
            p.lineReceived('5, %d' % (port,))
            self.assertEquals(
                L, ['%d, 5 : ERROR : INVALID-PORT' % (port,),
                    '5, %d : ERROR : INVALID-PORT' % (port,)])

    def testSuccess(self):
        p = TestIdentServer()
        p.makeConnection(StringTransport())
        L = []
        p.sendLine = L.append

        p.resultValue = ('SYS', 'USER')
        p.lineReceived('123, 456')
        self.assertEquals(L[0], '123, 456 : USERID : SYS : USER')


if struct.pack('=L', 1)[0] == '\x01':
    _addr1 = '0100007F'
    _addr2 = '04030201'
else:
    _addr1 = '7F000001'
    _addr2 = '01020304'


class ProcMixinTestCase(unittest.TestCase):
    line = ('4: %s:0019 %s:02FA 0A 00000000:00000000 '
            '00:00000000 00000000     0        0 10927 1 f72a5b80 '
            '3000 0 0 2 -1') % (_addr1, _addr2)

    def testDottedQuadFromHexString(self):
        p = ident.ProcServerMixin()
        self.assertEquals(p.dottedQuadFromHexString(_addr1), '127.0.0.1')

    def testUnpackAddress(self):
        p = ident.ProcServerMixin()
        self.assertEquals(p.unpackAddress(_addr1 + ':0277'),
                          ('127.0.0.1', 631))

    def testLineParser(self):
        p = ident.ProcServerMixin()
        self.assertEquals(
            p.parseLine(self.line),
            (('127.0.0.1', 25), ('1.2.3.4', 762), 0))

    def testExistingAddress(self):
        username = []
        p = ident.ProcServerMixin()
        p.entries = lambda: iter([self.line])
        p.getUsername = lambda uid: (username.append(uid), 'root')[1]
        self.assertEquals(
            p.lookup(('127.0.0.1', 25), ('1.2.3.4', 762)),
            (p.SYSTEM_NAME, 'root'))
        self.assertEquals(username, [0])

    def testNonExistingAddress(self):
        p = ident.ProcServerMixin()
        p.entries = lambda: iter([self.line])
        self.assertRaises(ident.NoUser, p.lookup, ('127.0.0.1', 26),
                                                  ('1.2.3.4', 762))
        self.assertRaises(ident.NoUser, p.lookup, ('127.0.0.1', 25),
                                                  ('1.2.3.5', 762))
        self.assertRaises(ident.NoUser, p.lookup, ('127.0.0.1', 25),
                                                  ('1.2.3.4', 763))


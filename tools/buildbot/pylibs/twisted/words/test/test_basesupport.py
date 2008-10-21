# Copyright (c) 2001-2006 Twisted Matrix Laboratories.
# See LICENSE for details.

from twisted.trial import unittest
from twisted.words.im import basesupport
from twisted.internet import error, defer

class DummyAccount(basesupport.AbstractAccount):
    """
    An account object that will do nothing when asked to start to log on.
    """

    loginHasFailed = False
    loginCallbackCalled = False

    def _startLogOn(self, *args):
        """
        Set self.loginDeferred to the same as the deferred returned, allowing a
        testcase to .callback or .errback.
        
        @return: A deferred.
        """
        self.loginDeferred = defer.Deferred()
        return self.loginDeferred

    def _loginFailed(self, result):
        self.loginHasFailed = True
        return basesupport.AbstractAccount._loginFailed(self, result)

    def _cb_logOn(self, result):
        self.loginCallbackCalled = True
        return basesupport.AbstractAccount._cb_logOn(self, result)

class DummyUI(object):
    """
    Provide just the interface required to be passed to AbstractAccount.logOn.
    """
    clientRegistered = False

    def registerAccountClient(self, result): 
        self.clientRegistered = True

class ClientMsgTests(unittest.TestCase):
    def makeUI(self):
        return DummyUI()

    def makeAccount(self):
        return DummyAccount('la', False, 'la', None, 'localhost', 6667)

    def test_connect(self):
        """
        Test that account.logOn works, and it calls the right callback when a
        connection is established.
        """
        account = self.makeAccount()
        ui = self.makeUI()
        d = account.logOn(ui)
        account.loginDeferred.callback(None)

        def check(result):
            self.assert_(not account.loginHasFailed, 
                    "Login shouldn't have failed")
            self.assert_(account.loginCallbackCalled, 
                    "We should be logged in")
        d.addCallback(check)
        return d

    def test_failedConnect(self):
        """
        Test that account.logOn works, and it calls the right callback when a
        connection is established.
        """
        account = self.makeAccount()
        ui = self.makeUI()
        d = account.logOn(ui)
        account.loginDeferred.errback(Exception())

        def err(reason):
            self.assert_(account.loginHasFailed, "Login should have failed")
            self.assert_(not account.loginCallbackCalled, 
                    "We shouldn't be logged in")
            self.assert_(not ui.clientRegistered, 
                    "Client shouldn't be registered in the UI")
        cb = lambda r: self.assert_(False, "Shouldn't get called back")
        d.addCallbacks(cb, err)
        return d

    def test_alreadyConnecting(self):
        """
        Test that it can fail sensibly when someone tried to connect before
        we did. 
        """
        account = self.makeAccount()
        ui = self.makeUI()
        account.logOn(ui)
        self.assertRaises(error.ConnectError, account.logOn, ui)


# Copyright (c) 2001-2006 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Test the interaction between trial and errors logged during test run.
"""

import time

from twisted.internet import defer
from twisted.python import failure, log
from twisted.trial import unittest, reporter, runner


def makeFailure():
    """
    Return a new, realistic failure.
    """
    try:
        1/0
    except ZeroDivisionError:
        f = failure.Failure()
    return f


def deferLater(n):
    """
    Return a L{defer.Deferred} that will fire in C{n} seconds.
    """
    from twisted.internet import reactor
    d = defer.Deferred()
    reactor.callLater(n, d.callback, None)
    return d


class Mask(object):
    """
    Hide C{MockTest}s from Trial's automatic test finder.
    """
    
    class MockTest(unittest.TestCase):
        def test_silent(self):
            """
            Don't log any errors.
            """

        def test_single(self):
            """
            Log a single error.
            """
            log.err(makeFailure())

        def test_double(self):
            """
            Log two errors.
            """
            log.err(makeFailure())
            log.err(makeFailure())

        def test_inCallback(self):
            """
            Log an error in an asynchronous callback.
            """
            d = deferLater(0)
            d.addCallback(lambda _: log.err(makeFailure()))
            return d


class TestObserver(unittest.TestCase):
    def setUp(self):
        self.result = reporter.TestResult()
        self.observer = unittest._LogObserver()

    def test_msg(self):
        """
        Test that a standard log message doesn't go anywhere near the result.
        """
        self.observer.gotEvent({'message': ('some message',),
                                'time': time.time(), 'isError': 0,
                                'system': '-'})
        self.assertEqual(self.observer.getErrors(), [])

    def test_error(self):
        """
        Test that an observed error gets added to the result
        """
        f = makeFailure()
        self.observer.gotEvent({'message': (),
                                'time': time.time(), 'isError': 1,
                                'system': '-', 'failure': f,
                                'why': None})
        self.assertEqual(self.observer.getErrors(), [f])

    def test_flush(self):
        """
        Check that flushing the observer with no args removes all errors.
        """
        self.test_error()
        flushed = self.observer.flushErrors()
        self.assertEqual(self.observer.getErrors(), [])
        self.assertEqual(len(flushed), 1)
        self.assertTrue(flushed[0].check(ZeroDivisionError))

    def _makeRuntimeFailure(self):
        return failure.Failure(RuntimeError('test error'))

    def test_flushByType(self):
        """
        Check that flushing the observer remove all failures of the given type.
        """
        self.test_error() # log a ZeroDivisionError to the observer
        f = self._makeRuntimeFailure()
        self.observer.gotEvent(dict(message=(), time=time.time(), isError=1,
                                    system='-', failure=f, why=None))
        flushed = self.observer.flushErrors(ZeroDivisionError)
        self.assertEqual(self.observer.getErrors(), [f])
        self.assertEqual(len(flushed), 1)
        self.assertTrue(flushed[0].check(ZeroDivisionError))

    def test_ignoreErrors(self):
        """
        Check that C{_ignoreErrors} actually causes errors to be ignored.
        """
        self.observer._ignoreErrors(ZeroDivisionError)
        f = makeFailure()
        self.observer.gotEvent({'message': (),
                                'time': time.time(), 'isError': 1,
                                'system': '-', 'failure': f,
                                'why': None})
        self.assertEqual(self.observer.getErrors(), [])

    def test_clearIgnores(self):
        """
        Check that C{_clearIgnores} ensures that previously ignored errors
        get captured.
        """
        self.observer._ignoreErrors(ZeroDivisionError)
        self.observer._clearIgnores()
        f = makeFailure()
        self.observer.gotEvent({'message': (),
                                'time': time.time(), 'isError': 1,
                                'system': '-', 'failure': f,
                                'why': None})
        self.assertEqual(self.observer.getErrors(), [f])


class LogErrors(unittest.TestCase):
    """
    High-level tests demonstrating the expected behaviour of logged errors
    during tests.
    """
    
    def setUp(self):
        self.result = reporter.TestResult()

    def tearDown(self):
        self.flushLoggedErrors(ZeroDivisionError)

    def test_singleError(self):
        """
        Test that a logged error gets reported as a test error.
        """
        test = Mask.MockTest('test_single')
        test(self.result)
        self.assertEqual(len(self.result.errors), 1)
        self.assertTrue(self.result.errors[0][1].check(ZeroDivisionError),
                        self.result.errors[0][1])

    def test_twoErrors(self):
        """
        Test that when two errors get logged, they both get reported as test
        errors.
        """
        test = Mask.MockTest('test_double')
        test(self.result)
        self.assertEqual(len(self.result.errors), 2)

    def test_inCallback(self):
        """
        Test that errors logged in callbacks get reported as test errors.
        """
        test = Mask.MockTest('test_inCallback')
        test(self.result)
        self.assertEqual(len(self.result.errors), 1)
        self.assertTrue(self.result.errors[0][1].check(ZeroDivisionError),
                        self.result.errors[0][1])
        
    def test_errorsIsolated(self):
        """
        Check that an error logged in one test doesn't fail the next test.
        """
        t1 = Mask.MockTest('test_single')
        t2 = Mask.MockTest('test_silent')
        t1(self.result)
        t2(self.result)
        self.assertEqual(len(self.result.errors), 1)
        self.assertEqual(self.result.errors[0][0], t1)

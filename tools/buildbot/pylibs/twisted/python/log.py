# -*- test-case-name: twisted.test.test_log -*-
# Copyright (c) 2001-2007 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Logging and metrics infrastructure.
"""

from __future__ import division

# System Imports
import sys
import time
import warnings
from datetime import datetime
import logging

# Sibling Imports
from twisted.python import util, context, reflect

class ILogContext:
    """
    Actually, this interface is just a synoym for the dictionary interface,
    but it serves as a key for the default information in a log.

    I do not inherit from Interface because the world is a cruel place.
    """

context.setDefault(ILogContext,
                   {"isError": 0,
                    "system": "-"})

def callWithContext(ctx, func, *args, **kw):
    newCtx = context.get(ILogContext).copy()
    newCtx.update(ctx)
    return context.call({ILogContext: newCtx}, func, *args, **kw)

def callWithLogger(logger, func, *args, **kw):
    """
    Utility method which wraps a function in a try:/except:, logs a failure if
    one occurrs, and uses the system's logPrefix.
    """
    try:
        lp = logger.logPrefix()
    except KeyboardInterrupt:
        raise
    except:
        lp = '(buggy logPrefix method)'
        err(system=lp)
    try:
        return callWithContext({"system": lp}, func, *args, **kw)
    except KeyboardInterrupt:
        raise
    except:
        err(system=lp)

def showwarning(message, category, filename, lineno, file=None):
    if file is None:
        msg(warning=message, category=reflect.qual(category), filename=filename, lineno=lineno,
            format="%(filename)s:%(lineno)s: %(category)s: %(warning)s")
    else:
        _oldshowwarning(message, category, filename, lineno, file)

_keepErrors = 0
_keptErrors = []
_ignoreErrors = []

def startKeepingErrors():
    """
    DEPRECATED in Twisted 2.5.

    Support function for testing frameworks.

    Start keeping errors in a buffer which can be retrieved (and emptied) with
    flushErrors.
    """
    warnings.warn("log.startKeepingErrors is deprecated since Twisted 2.5",
                  category=DeprecationWarning, stacklevel=2)
    global _keepErrors
    _keepErrors = 1


def flushErrors(*errorTypes):
    """
    DEPRECATED in Twisted 2.5.  See L{TestCase.flushLoggedErrors}.

    Support function for testing frameworks.

    Return a list of errors that occurred since the last call to flushErrors().
    (This will return None unless startKeepingErrors has been called.)
    """

    warnings.warn("log.flushErrors is deprecated since Twisted 2.5. "
                  "If you need to flush errors from within a unittest, "
                  "use TestCase.flushLoggedErrors instead.",
                  category=DeprecationWarning, stacklevel=2)
    return _flushErrors(*errorTypes)


def _flushErrors(*errorTypes):
    """
    PRIVATE. DEPRECATED. DON'T USE.
    """
    global _keptErrors
    k = _keptErrors
    _keptErrors = []
    if errorTypes:
        for erk in k:
            shouldReLog = 1
            for errT in errorTypes:
                if erk.check(errT):
                    shouldReLog = 0
            if shouldReLog:
                err(erk)
    return k

def ignoreErrors(*types):
    """
    DEPRECATED
    """
    warnings.warn("log.ignoreErrors is deprecated since Twisted 2.5",
                  category=DeprecationWarning, stacklevel=2)
    _ignore(*types)

def _ignore(*types):
    """
    PRIVATE. DEPRECATED. DON'T USE.
    """
    for type in types:
        _ignoreErrors.append(type)

def clearIgnores():
    """
    DEPRECATED
    """
    warnings.warn("log.clearIgnores is deprecated since Twisted 2.5",
                  category=DeprecationWarning, stacklevel=2)
    _clearIgnores()

def _clearIgnores():
    """
    PRIVATE. DEPRECATED. DON'T USE.
    """
    global _ignoreErrors
    _ignoreErrors = []


def err(_stuff=None, _why=None, **kw):
    """
    Write a failure to the log.
    """
    if _stuff is None:
        _stuff = failure.Failure()
    if isinstance(_stuff, failure.Failure):
        if _keepErrors:
            if _ignoreErrors:
                keep = 0
                for err in _ignoreErrors:
                    r = _stuff.check(err)
                    if r:
                        keep = 0
                        break
                    else:
                        keep = 1
                if keep:
                    _keptErrors.append(_stuff)
            else:
                _keptErrors.append(_stuff)
        msg(failure=_stuff, why=_why, isError=1, **kw)
    elif isinstance(_stuff, Exception):
        msg(failure=failure.Failure(_stuff), why=_why, isError=1, **kw)
    else:
        msg(repr(_stuff), why=_why, isError=1, **kw)

deferr = err


class Logger:
    """
    This represents a class which may 'own' a log. Used by subclassing.
    """
    def logPrefix(self):
        """
        Override this method to insert custom logging behavior.  Its
        return value will be inserted in front of every line.  It may
        be called more times than the number of output lines.
        """
        return '-'


class LogPublisher:
    """
    Class for singleton log message publishing.
    """

    synchronized = ['msg']

    def __init__(self):
        self.observers = []

    def addObserver(self, other):
        """
        Add a new observer.

        Observers are callable objects that will be called with each new log
        message (a dict).
        """
        assert callable(other)
        self.observers.append(other)

    def removeObserver(self, other):
        """
        Remove an observer.
        """
        self.observers.remove(other)

    def msg(self, *message, **kw):
        """
        Log a new message.

        For example::

        >>> log.msg('Hello, world.')

        In particular, you MUST avoid the forms::

        >>> log.msg(u'Hello, world.')
        >>> log.msg('Hello ', 'world.')

        These forms work (sometimes) by accident and will be disabled
        entirely in the future.
        """
        actualEventDict = (context.get(ILogContext) or {}).copy()
        actualEventDict.update(kw)
        actualEventDict['message'] = message
        actualEventDict['time'] = time.time()
        for i in xrange(len(self.observers) - 1, -1, -1):
            try:
                self.observers[i](actualEventDict)
            except KeyboardInterrupt:
                # Don't swallow keyboard interrupt!
                raise
            except UnicodeEncodeError:
                raise
            except:
                o = self.observers.pop(i)
                err(failure.Failure(),
                    "Log observer %s failed, removing from observer list." % (o,))


try:
    theLogPublisher
except NameError:
    theLogPublisher = LogPublisher()
    addObserver = theLogPublisher.addObserver
    removeObserver = theLogPublisher.removeObserver
    msg = theLogPublisher.msg


def _safeFormat(fmtString, fmtDict):
    """
    Try to format the string C{fmtString} using C{fmtDict} arguments,
    swallowing all errors to always return a string.
    """
    # There's a way we could make this if not safer at least more
    # informative: perhaps some sort of str/repr wrapper objects
    # could be wrapped around the things inside of C{fmtDict}. That way
    # if the event dict contains an object with a bad __repr__, we
    # can only cry about that individual object instead of the
    # entire event dict.
    try:
        text = fmtString % fmtDict
    except KeyboardInterrupt:
        raise
    except:
        try:
            text = ('Invalid format string or unformattable object in log message: %r, %s' % (fmtString, fmtDict))
        except:
            try:
                text = 'UNFORMATTABLE OBJECT WRITTEN TO LOG with fmt %r, MESSAGE LOST' % (fmtString,)
            except:
                text = 'PATHOLOGICAL ERROR IN BOTH FORMAT STRING AND MESSAGE DETAILS, MESSAGE LOST'
    return text


def textFromEventDict(eventDict):
    """
    Extract text from an event dict passed to a log observer. If it cannot
    handle the dict, it returns None.

    The possible keys of eventDict are:
     - C{message}: by default, it holds the final text. It's required, but can
       be empty if either C{isError} or C{format} is provided (the first
       having the priority).
     - C{isError}: boolean indicating the nature of the event.
     - C{failure}: L{failure.Failure} instance, required if the event is an
       error.
     - C{why}: if defined, used as header of the traceback in case of errors.
     - C{format}: string format used in place of C{message} to customize
       the event. It uses all keys present in C{eventDict} to format
       the text.
    Other keys will be used when applying the C{format}, or ignored.
    """
    edm = eventDict['message']
    if not edm:
        if eventDict['isError'] and 'failure' in eventDict:
            text = ((eventDict.get('why') or 'Unhandled Error')
                    + '\n' + eventDict['failure'].getTraceback())
        elif 'format' in eventDict:
            text = _safeFormat(eventDict['format'], eventDict)
        else:
            # we don't know how to log this
            return
    else:
        text = ' '.join(map(reflect.safe_str, edm))
    return text


class FileLogObserver:
    """
    Log observer that writes to a file-like object.

    @type timeFormat: C{str} or C{NoneType}
    @ivar timeFormat: If not C{None}, the format string passed to strftime().
    """
    timeFormat = None

    def __init__(self, f):
        self.write = f.write
        self.flush = f.flush

    def getTimezoneOffset(self, when):
        """
        Return the current local timezone offset from UTC.

        @type when: C{int}
        @param when: POSIX (ie, UTC) timestamp for which to find the offset.

        @rtype: C{int}
        @return: The number of seconds offset from UTC.  West is positive,
        east is negative.
        """
        offset = datetime.utcfromtimestamp(when) - datetime.fromtimestamp(when)
        return offset.days * (60 * 60 * 24) + offset.seconds

    def formatTime(self, when):
        """
        Format the given UTC value as a string representing that time in the
        local timezone.

        By default it's formatted as a ISO8601-like string (ISO8601 date and
        ISO8601 time separated by a space). It can be customized using the
        C{timeFormat} attribute, which will be used as input for the underlying
        C{time.strftime} call.

        @type when: C{int}
        @param when: POSIX (ie, UTC) timestamp for which to find the offset.

        @rtype: C{str}
        """
        if self.timeFormat is not None:
            return time.strftime(self.timeFormat, time.localtime(when))

        tzOffset = -self.getTimezoneOffset(when)
        when = datetime.utcfromtimestamp(when + tzOffset)
        tzHour = int(tzOffset / 60 / 60)
        tzMin = int(tzOffset / 60 % 60)
        return '%d-%02d-%02d %02d:%02d:%02d%+03d%02d' % (
            when.year, when.month, when.day,
            when.hour, when.minute, when.second,
            tzHour, tzMin)

    def emit(self, eventDict):
        text = textFromEventDict(eventDict)
        if text is None:
            return

        timeStr = self.formatTime(eventDict['time'])
        fmtDict = {'system': eventDict['system'], 'text': text.replace("\n", "\n\t")}
        msgStr = _safeFormat("[%(system)s] %(text)s\n", fmtDict)

        util.untilConcludes(self.write, timeStr + " " + msgStr)
        util.untilConcludes(self.flush)  # Hoorj!

    def start(self):
        """
        Start observing log events.
        """
        addObserver(self.emit)

    def stop(self):
        """
        Stop observing log events.
        """
        removeObserver(self.emit)


class PythonLoggingObserver(object):
    """
    Output twisted messages to Python standard library L{logging} module.

    WARNING: specific logging configurations (example: network) can lead to
    a blocking system. Nothing is done here to prevent that, so be sure to not
    use this: code within Twisted, such as twisted.web, assumes that logging
    does not block.
    """

    def __init__(self, loggerName="twisted"):
        """
        @param loggerName: identifier used for getting logger.
        @type loggerName: C{str}
        """
        self.logger = logging.getLogger(loggerName)

    def emit(self, eventDict):
        """
        Receive a twisted log entry, format it and bridge it to python.

        By default the logging level used is info; log.err produces error
        level, and you can customize the level by using the C{logLevel} key::

        >>> log.msg('debugging', logLevel=logging.DEBUG)

        """
        if 'logLevel' in eventDict:
            level = eventDict['logLevel']
        elif eventDict['isError']:
            level = logging.ERROR
        else:
            level = logging.INFO
        text = textFromEventDict(eventDict)
        if text is None:
            return
        self.logger.log(level, text)

    def start(self):
        """
        Start observing log events.
        """
        addObserver(self.emit)

    def stop(self):
        """
        Stop observing log events.
        """
        removeObserver(self.emit)


class StdioOnnaStick:
    """
    Class that pretends to be stout/err.
    """

    closed = 0
    softspace = 0
    mode = 'wb'
    name = '<stdio (log)>'

    def __init__(self, isError=0):
        self.isError = isError
        self.buf = ''

    def close(self):
        pass

    def fileno(self):
        return -1

    def flush(self):
        pass

    def read(self):
        raise IOError("can't read from the log!")

    readline = read
    readlines = read
    seek = read
    tell = read

    def write(self, data):
        d = (self.buf + data).split('\n')
        self.buf = d[-1]
        messages = d[0:-1]
        for message in messages:
            msg(message, printed=1, isError=self.isError)

    def writelines(self, lines):
        for line in lines:
            msg(line, printed=1, isError=self.isError)


try:
    _oldshowwarning
except NameError:
    _oldshowwarning = None


def startLogging(file, *a, **kw):
    """
    Initialize logging to a specified file.
    """
    flo = FileLogObserver(file)
    startLoggingWithObserver(flo.emit, *a, **kw)

def startLoggingWithObserver(observer, setStdout=1):
    """
    Initialize logging to a specified observer. If setStdout is true
    (defaults to yes), also redirect sys.stdout and sys.stderr
    to the specified file.
    """
    global defaultObserver, _oldshowwarning
    if not _oldshowwarning:
        _oldshowwarning = warnings.showwarning
        warnings.showwarning = showwarning
    if defaultObserver:
        defaultObserver.stop()
        defaultObserver = None
    addObserver(observer)
    msg("Log opened.")
    if setStdout:
        sys.stdout = logfile
        sys.stderr = logerr


class NullFile:
    softspace = 0
    def read(self): pass
    def write(self, bytes): pass
    def flush(self): pass
    def close(self): pass


def discardLogs():
    """
    Throw away all logs.
    """
    global logfile
    logfile = NullFile()


# Prevent logfile from being erased on reload.  This only works in cpython.
try:
    logfile
except NameError:
    logfile = StdioOnnaStick(0)
    logerr = StdioOnnaStick(1)


class DefaultObserver:
    """
    Default observer.

    Will ignore all non-error messages and send error messages to sys.stderr.
    Will be removed when startLogging() is called for the first time.
    """

    def _emit(self, eventDict):
        if eventDict["isError"]:
            if 'failure' in eventDict:
                text = eventDict['failure'].getTraceback()
            else:
                text = " ".join([str(m) for m in eventDict["message"]]) + "\n"
            sys.stderr.write(text)
            sys.stderr.flush()

    def start(self):
        addObserver(self._emit)

    def stop(self):
        removeObserver(self._emit)


# Some more sibling imports, at the bottom and unqualified to avoid
# unresolvable circularity
import threadable, failure
threadable.synchronize(LogPublisher)


try:
    defaultObserver
except NameError:
    defaultObserver = DefaultObserver()
    defaultObserver.start()


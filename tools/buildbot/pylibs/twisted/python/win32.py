# Copyright (c) 2001-2007 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Win32 utilities.

See also twisted.python.shortcut.
"""

import re
import exceptions
import os

try:
    import win32api
    import win32con
except ImportError:
    pass

from twisted.python.runtime import platform

# http://msdn.microsoft.com/library/default.asp?url=/library/en-us/debug/base/system_error_codes.asp
ERROR_FILE_NOT_FOUND = 2
ERROR_PATH_NOT_FOUND = 3
ERROR_INVALID_NAME = 123
ERROR_DIRECTORY = 267

def _determineWindowsError():
    """
    Determine which WindowsError name to export.
    """
    return getattr(exceptions, 'WindowsError', FakeWindowsError)

class FakeWindowsError(OSError):
    """
    Stand-in for sometimes-builtin exception on platforms for which it
    is missing.
    """

WindowsError = _determineWindowsError()

# XXX fix this to use python's builtin _winreg?

def getProgramsMenuPath():
    """Get the path to the Programs menu.

    Probably will break on non-US Windows.

    @returns: the filesystem location of the common Start Menu->Programs.
    """
    if not platform.isWinNT():
        return "C:\\Windows\\Start Menu\\Programs"
    keyname = 'SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders'
    hShellFolders = win32api.RegOpenKeyEx(win32con.HKEY_LOCAL_MACHINE,
                                          keyname, 0, win32con.KEY_READ)
    return win32api.RegQueryValueEx(hShellFolders, 'Common Programs')[0]


def getProgramFilesPath():
    """Get the path to the Program Files folder."""
    keyname = 'SOFTWARE\\Microsoft\\Windows\\CurrentVersion'
    currentV = win32api.RegOpenKeyEx(win32con.HKEY_LOCAL_MACHINE,
                                     keyname, 0, win32con.KEY_READ)
    return win32api.RegQueryValueEx(currentV, 'ProgramFilesDir')[0]

_cmdLineQuoteRe = re.compile(r'(\\*)"')
_cmdLineQuoteRe2 = re.compile(r'(\\+)\Z')
def cmdLineQuote(s):
    """
    Internal method for quoting a single command-line argument.

    @param s: an unquoted string that you want to quote so that something that
    does cmd.exe-style unquoting will interpret it as a single argument, even
    if it contains spaces.

    @return: a quoted string.
    """
    quote = ((" " in s) or ("\t" in s) or ('"' in s)) and '"' or ''
    return quote + _cmdLineQuoteRe2.sub(r"\1\1", _cmdLineQuoteRe.sub(r'\1\1\\"', s)) + quote

def quoteArguments(arguments):
    """
    Quote an iterable of command-line arguments for passing to CreateProcess or
    a similar API.  This allows the list passed to C{reactor.spawnProcess} to
    match the child process's C{sys.argv} properly.

    @param arglist: an iterable of C{str}, each unquoted.

    @return: a single string, with the given sequence quoted as necessary.
    """
    return ' '.join([cmdLineQuote(a) for a in arguments])


class _ErrorFormatter(object):
    """
    Formatter for Windows error messages.

    @ivar winError: A callable which takes one integer error number argument
        and returns an L{exceptions.WindowsError} instance for that error (like
        L{ctypes.WinError}).

    @ivar formatMessage: A callable which takes one integer error number
        argument and returns a C{str} giving the message for that error (like
        L{win32api.FormatMessage}).

    @ivar errorTab: A mapping from integer error numbers to C{str} messages
        which correspond to those erorrs (like L{socket.errorTab}).
    """
    def __init__(self, WinError, FormatMessage, errorTab):
        self.winError = WinError
        self.formatMessage = FormatMessage
        self.errorTab = errorTab

    def fromEnvironment(cls):
        """
        Get as many of the platform-specific error translation objects as
        possible and return an instance of C{cls} created with them.
        """
        try:
            from ctypes import WinError
        except ImportError:
            WinError = None
        try:
            from win32api import FormatMessage
        except ImportError:
            FormatMessage = None
        try:
            from socket import errorTab
        except ImportError:
            errorTab = None
        return cls(WinError, FormatMessage, errorTab)
    fromEnvironment = classmethod(fromEnvironment)


    def formatError(self, errorcode):
        """
        Returns the string associated with a Windows error message, such as the
        ones found in socket.error.

        Attempts direct lookup against the win32 API via ctypes and then
        pywin32 if available), then in the error table in the socket module,
        then finally defaulting to C{os.strerror}.

        @param errorcode: the Windows error code
        @type errorcode: C{int}

        @return: The error message string
        @rtype: C{str}
        """
        if self.winError is not None:
            return self.winError(errorcode)[1]
        if self.formatMessage is not None:
            return self.formatMessage(errorcode)
        if self.errorTab is not None:
            result = self.errorTab.get(errorcode)
            if result is not None:
                return result
        return os.strerror(errorcode)

formatError = _ErrorFormatter.fromEnvironment().formatError

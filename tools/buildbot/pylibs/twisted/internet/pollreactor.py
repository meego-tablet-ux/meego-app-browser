# Copyright (c) 2001-2007 Twisted Matrix Laboratories.
# See LICENSE for details.


"""
A poll() based implementation of the twisted main loop.

To install the event loop (and you should do this before any connections,
listeners or connectors are added)::

    from twisted.internet import pollreactor
    pollreactor.install()

Maintainer: U{Itamar Shtull-Trauring<mailto:twisted@itamarst.org>}
"""

# System imports
import errno, sys
from select import error as SelectError, poll
from select import POLLIN, POLLOUT, POLLHUP, POLLERR, POLLNVAL

from zope.interface import implements

# Twisted imports
from twisted.python import log
from twisted.internet import main, posixbase, error
from twisted.internet.interfaces import IReactorFDSet

POLL_DISCONNECTED = (POLLHUP | POLLERR | POLLNVAL)


class PollReactor(posixbase.PosixReactorBase):
    """
    A reactor that uses poll(2).

    @ivar _poller: A L{poll} which will be used to check for I/O
        readiness.

    @ivar _selectables: A dictionary mapping integer file descriptors to
        instances of L{FileDescriptor} which have been registered with the
        reactor.  All L{FileDescriptors} which are currently receiving read or
        write readiness notifications will be present as values in this
        dictionary.

    @ivar _reads: A dictionary mapping integer file descriptors to arbitrary
        values (this is essentially a set).  Keys in this dictionary will be
        registered with C{_poller} for read readiness notifications which will
        be dispatched to the corresponding L{FileDescriptor} instances in
        C{_selectables}.

    @ivar _writes: A dictionary mapping integer file descriptors to arbitrary
        values (this is essentially a set).  Keys in this dictionary will be
        registered with C{_poller} for write readiness notifications which will
        be dispatched to the corresponding L{FileDescriptor} instances in
        C{_selectables}.
    """
    implements(IReactorFDSet)

    def __init__(self):
        """
        Initialize polling object, file descriptor tracking dictionaries, and
        the base class.
        """
        self._poller = poll()
        self._selectables = {}
        self._reads = {}
        self._writes = {}
        posixbase.PosixReactorBase.__init__(self)


    def _updateRegistration(self, fd):
        """Register/unregister an fd with the poller."""
        try:
            self._poller.unregister(fd)
        except KeyError:
            pass

        mask = 0
        if fd in self._reads:
            mask = mask | POLLIN
        if fd in self._writes:
            mask = mask | POLLOUT
        if mask != 0:
            self._poller.register(fd, mask)
        else:
            if fd in self._selectables:
                del self._selectables[fd]

    def _dictRemove(self, selectable, mdict):
        try:
            # the easy way
            fd = selectable.fileno()
            # make sure the fd is actually real.  In some situations we can get
            # -1 here.
            mdict[fd]
        except:
            # the hard way: necessary because fileno() may disappear at any
            # moment, thanks to python's underlying sockets impl
            for fd, fdes in self._selectables.items():
                if selectable is fdes:
                    break
            else:
                # Hmm, maybe not the right course of action?  This method can't
                # fail, because it happens inside error detection...
                return
        if fd in mdict:
            del mdict[fd]
            self._updateRegistration(fd)

    def addReader(self, reader):
        """Add a FileDescriptor for notification of data available to read.
        """
        fd = reader.fileno()
        if fd not in self._reads:
            self._selectables[fd] = reader
            self._reads[fd] =  1
            self._updateRegistration(fd)

    def addWriter(self, writer):
        """Add a FileDescriptor for notification of data available to write.
        """
        fd = writer.fileno()
        if fd not in self._writes:
            self._selectables[fd] = writer
            self._writes[fd] =  1
            self._updateRegistration(fd)

    def removeReader(self, reader):
        """Remove a Selectable for notification of data available to read.
        """
        return self._dictRemove(reader, self._reads)

    def removeWriter(self, writer):
        """Remove a Selectable for notification of data available to write.
        """
        return self._dictRemove(writer, self._writes)

    def removeAll(self):
        """Remove all selectables, and return a list of them."""
        if self.waker is not None:
            self.removeReader(self.waker)
        result = self._selectables.values()
        fds = self._selectables.keys()
        self._reads.clear()
        self._writes.clear()
        self._selectables.clear()
        for fd in fds:
            self._poller.unregister(fd)

        if self.waker is not None:
            self.addReader(self.waker)
        return result

    def doPoll(self, timeout):
        """Poll the poller for new events."""
        if timeout is not None:
            timeout = int(timeout * 1000) # convert seconds to milliseconds

        try:
            l = self._poller.poll(timeout)
        except SelectError, e:
            if e[0] == errno.EINTR:
                return
            else:
                raise
        _drdw = self._doReadOrWrite
        for fd, event in l:
            try:
                selectable = self._selectables[fd]
            except KeyError:
                # Handles the infrequent case where one selectable's
                # handler disconnects another.
                continue
            log.callWithLogger(selectable, _drdw, selectable, fd, event)

    doIteration = doPoll

    def _doReadOrWrite(self, selectable, fd, event):
        why = None
        inRead = False
        if event & POLL_DISCONNECTED and not (event & POLLIN):
            why = main.CONNECTION_LOST
        else:
            try:
                if event & POLLIN:
                    why = selectable.doRead()
                    inRead = True
                if not why and event & POLLOUT:
                    why = selectable.doWrite()
                    inRead = False
                if not selectable.fileno() == fd:
                    why = error.ConnectionFdescWentAway('Filedescriptor went away')
                    inRead = False
            except:
                log.deferr()
                why = sys.exc_info()[1]
        if why:
            self._disconnectSelectable(selectable, why, inRead)


    def getReaders(self):
        return [self._selectables[fd] for fd in self._reads]


    def getWriters(self):
        return [self._selectables[fd] for fd in self._writes]



def install():
    """Install the poll() reactor."""
    p = PollReactor()
    from twisted.internet.main import installReactor
    installReactor(p)


__all__ = ["PollReactor", "install"]

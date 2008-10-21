# Copyright (c) 2001-2004 Twisted Matrix Laboratories.
# See LICENSE for details.

#
"""
Service architecture for Twisted

Services are arranged in a hierarchy. At the leafs of the hierarchy,
the services which actually interact with the outside world are started.
Services can be named or anonymous -- usually, they will be named if
there is need to access them through the hierarchy (from a parent or
a sibling).

Maintainer: U{Moshe Zadka<mailto:moshez@twistedmatrix.com>}
"""

from zope.interface import implements, Interface, Attribute

from twisted.python.reflect import namedAny
from twisted.python import components
from twisted.internet import defer
from twisted.persisted import sob
from twisted.plugin import IPlugin

class IServiceMaker(Interface):
    """
    An object which can be used to construct services in a flexible
    way.

    This interface should most often be implemented along with
    twisted.plugin.IPlugin, and will most often be used by the
    'twistd' command.
    """
    tapname = Attribute(
        "A short string naming this Twisted plugin, for example 'web' or "
        "'pencil'. This name will be used as the subcommand of 'twistd'.")

    description = Attribute(
        "A brief summary of the features provided by this "
        "Twisted application plugin.")

    options = Attribute(
        "A C{twisted.python.usage.Options} subclass defining the"
        "configuration options for this application.")


    def makeService(options):
        """
        Create and return an object providing 
        L{twisted.application.service.IService}.

        @param options: A mapping (typically a C{dict} or
        C{twisted.python.usage.Options} instance) of configuration
        options to desired configuration values.
        """



class ServiceMaker(object):
    """
    Utility class to simplify the definition of L{IServiceMaker} plugins.
    """
    implements(IPlugin, IServiceMaker)

    def __init__(self, name, module, description, tapname):
        self.name = name
        self.module = module
        self.description = description
        self.tapname = tapname


    def options():
        def get(self):
            return namedAny(self.module).Options
        return get,
    options = property(*options())


    def makeService():
        def get(self):
            return namedAny(self.module).makeService
        return get,
    makeService = property(*makeService())



class IService(Interface):
    """
    A service.

    Run start-up and shut-down code at the appropriate times.

    @type name:            C{string}
    @ivar name:            The name of the service (or None)
    @type running:         C{boolean}
    @ivar running:         Whether the service is running.
    """

    def setName(name):
        """Set the name of the service.

        @type name: C{str}
        @raise RuntimeError: Raised if the service already has a parent.
        """

    def setServiceParent(parent):
        """Set the parent of the service.

        @type name: L{IServiceCollection}
        @raise RuntimeError: Raised if the service already has a parent
        or if the service has a name and the parent already has a child
        by that name.
        """

    def disownServiceParent():
        """Remove the parent of the service.

        @rtype: L{Deferred}
        @return: a deferred which is triggered when the service has
        finished shutting down. If shutting down is immediate,
        a value can be returned (usually, None).
        """

    def startService():
        """Start the service."""

    def stopService():
        """Stop the service.

        @rtype: L{Deferred}
        @return: a deferred which is triggered when the service has
        finished shutting down. If shutting down is immediate,
        a value can be returned (usually, None).
        """

    def privilegedStartService():
        """Do preparation work for starting the service.

        Here things which should be done before changing directory,
        root or shedding privileges are done."""


class Service:

    """
    Base class for services

    Most services should inherit from this class. It handles the
    book-keeping reponsibilities of starting and stopping, as well
    as not serializing this book-keeping information.
    """

    implements(IService)

    running = 0
    name = None
    parent = None

    def __getstate__(self):
        dict = self.__dict__.copy()
        if dict.has_key("running"):
            del dict['running']
        return dict

    def setName(self, name):
        if self.parent is not None:
            raise RuntimeError("cannot change name when parent exists")
        self.name = name

    def setServiceParent(self, parent):
        if self.parent is not None:
            self.disownServiceParent()
        parent = IServiceCollection(parent, parent)
        self.parent = parent
        self.parent.addService(self)

    def disownServiceParent(self):
        d = self.parent.removeService(self)
        self.parent = None
        return d

    def privilegedStartService(self):
        pass

    def startService(self):
        self.running = 1

    def stopService(self):
        self.running = 0



class IServiceCollection(Interface):

    """Collection of services.

    Contain several services, and manage their start-up/shut-down.
    Services can be accessed by name if they have a name, and it
    is always possible to iterate over them.
    """

    def getServiceNamed(name):
        """Get the child service with a given name.

        @type name: C{str}
        @rtype: L{IService}
        @raise KeyError: Raised if the service has no child with the
        given name.
        """

    def __iter__():
        """Get an iterator over all child services"""

    def addService(service):
         """Add a child service.

        @type service: L{IService}
        @raise RuntimeError: Raised if the service has a child with
        the given name.
        """

    def removeService(service):
        """Remove a child service.

        @type service: L{IService}
        @raise ValueError: Raised if the given service is not a child.
        @rtype: L{Deferred}
        @return: a deferred which is triggered when the service has
        finished shutting down. If shutting down is immediate,
        a value can be returned (usually, None).
        """



class MultiService(Service):

    """Straightforward Service Container

    Hold a collection of services, and manage them in a simplistic
    way. No service will wait for another, but this object itself
    will not finish shutting down until all of its child services
    will finish.
    """

    implements(IServiceCollection)

    def __init__(self):
        self.services = []
        self.namedServices = {}
        self.parent = None

    def privilegedStartService(self):
        Service.privilegedStartService(self)
        for service in self:
            service.privilegedStartService()

    def startService(self):
        Service.startService(self)
        for service in self:
            service.startService()

    def stopService(self):
        Service.stopService(self)
        l = []
        services = list(self)
        services.reverse()
        for service in services:
            l.append(defer.maybeDeferred(service.stopService))
        return defer.DeferredList(l)

    def getServiceNamed(self, name):
        return self.namedServices[name]

    def __iter__(self):
        return iter(self.services)

    def addService(self, service):
        if service.name is not None:
            if self.namedServices.has_key(service.name):
                raise RuntimeError("cannot have two services with same name"
                                   " '%s'" % service.name)
            self.namedServices[service.name] = service
        self.services.append(service)
        if self.running:
            # It may be too late for that, but we will do our best
            service.privilegedStartService()
            service.startService()

    def removeService(self, service):
        if service.name:
            del self.namedServices[service.name]
        self.services.remove(service)
        if self.running:
            # Returning this so as not to lose information from the
            # MultiService.stopService deferred.
            return service.stopService()
        else:
            return None



class IProcess(Interface):

    """Process running parameters

    Represents parameters for how processes should be run.

    @ivar processName: the name the process should have in ps (or None)
    @type processName: C{str}
    @ivar uid: the user-id the process should run under.
    @type uid: C{int}
    @ivar gid: the group-id the process should run under.
    @type gid: C{int}
    """


class Process:
    """Process running parameters

    Sets up uid/gid in the constructor, and has a default
    of C{None} as C{processName}.
    """
    implements(IProcess)
    processName = None

    def __init__(self, uid=None, gid=None):
        """Set uid and gid.

        @param uid: The user ID as whom to execute the process.  If
        this is None, no attempt will be made to change the UID.

        @param gid: The group ID as whom to execute the process.  If
        this is None, no attempt will be made to change the GID.
        """
        self.uid = uid
        self.gid = gid


def Application(name, uid=None, gid=None):
    """Return a compound class.

    Return an object supporting the L{IService}, L{IServiceCollection},
    L{IProcess} and L{sob.IPersistable} interfaces, with the given
    parameters. Always access the return value by explicit casting to
    one of the interfaces.
    """
    ret = components.Componentized()
    for comp in (MultiService(), sob.Persistent(ret, name), Process(uid, gid)):
        ret.addComponent(comp, ignoreClass=1)
    IService(ret).setName(name)
    return ret



def loadApplication(filename, kind, passphrase=None):
    """Load Application from a given file.

    The serialization format it was saved in should be given as
    C{kind}, and is one of 'pickle', 'source', 'xml' or 'python'. If
    C{passphrase} is given, the application was encrypted with the
    given passphrase.

    @type filename: C{str}
    @type kind: C{str}
    @type passphrase: C{str}
    """
    if kind == 'python':
        application = sob.loadValueFromFile(filename, 'application', passphrase)
    else:
        application = sob.load(filename, kind, passphrase)
    return application


__all__ = ['IServiceMaker', 'IService', 'Service', 
           'IServiceCollection', 'MultiService',
           'IProcess', 'Process', 'Application', 'loadApplication']

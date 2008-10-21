# -*- test-case-name: twisted.test.test_amp -*-
# Copyright (c) 2005 Divmod, Inc.
# Copyright (c) 2007 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
This module implements AMP, the Asynchronous Messaging Protocol.

AMP is a protocol for sending multiple asynchronous request/response pairs over
the same connection.  Requests and responses are both collections of key/value
pairs.

AMP is a very simple protocol which is not an application.  This module is a
"protocol construction kit" of sorts; it attempts to be the simplest wire-level
implementation of Deferreds.  AMP provides the following base-level features:

    - Asynchronous request/response handling (hence the name)

    - Requests and responses are both key/value pairs

    - Binary transfer of all data: all data is length-prefixed.  Your
      application will never need to worry about quoting.

    - Command dispatching (like HTTP Verbs): the protocol is extensible, and
      multiple AMP sub-protocols can be grouped together easily.

The protocol implementation also provides a few additional features which are
not part of the core wire protocol, but are nevertheless very useful:

    - Tight TLS integration, with an included StartTLS command.

    - Handshaking to other protocols: because AMP has well-defined message
      boundaries and maintains all incoming and outgoing requests for you, you
      can start a connection over AMP and then switch to another protocol.
      This makes it ideal for firewall-traversal applications where you may
      have only one forwarded port but multiple applications that want to use
      it.

Using AMP with Twisted is simple.  Each message is a command, with a response.
You begin by defining a command type.  Commands specify their input and output
in terms of the types that they expect to see in the request and response
key-value pairs.  Here's an example of a command that adds two integers, 'a'
and 'b'::

    class Sum(amp.Command):
        arguments = [('a', amp.Integer()),
                     ('b', amp.Integer())]
        response = [('total', amp.Integer())]

Once you have specified a command, you need to make it part of a protocol, and
define a responder for it.  Here's a 'JustSum' protocol that includes a
responder for our 'Sum' command::

    class JustSum(amp.AMP):
        def sum(self, a, b):
            total = a + b
            print 'Did a sum: %d + %d = %d' % (a, b, total)
            return {'total': total}
        Sum.responder(sum)

Later, when you want to actually do a sum, the following expression will return
a Deferred which will fire with the result::

    ClientCreator(reactor, amp.AMP).connectTCP(...).addCallback(
        lambda p: p.callRemote(Sum, a=13, b=81)).addCallback(
            lambda result: result['total'])

You can also define the propagation of specific errors in AMP.  For example,
for the slightly more complicated case of division, we might have to deal with
division by zero::

    class Divide(amp.Command):
        arguments = [('numerator', amp.Integer()),
                     ('denominator', amp.Integer())]
        response = [('result', amp.Float())]
        errors = {ZeroDivisionError: 'ZERO_DIVISION'}

The 'errors' mapping here tells AMP that if a responder to Divide emits a
L{ZeroDivisionError}, then the other side should be informed that an error of
the type 'ZERO_DIVISION' has occurred.  Writing a responder which takes
advantage of this is very simple - just raise your exception normally::

    class JustDivide(amp.AMP):
        def divide(self, numerator, denominator):
            result = numerator / denominator
            print 'Divided: %d / %d = %d' % (numerator, denominator, total)
            return {'result': result}
        Divide.responder(divide)

On the client side, the errors mapping will be used to determine what the
'ZERO_DIVISION' error means, and translated into an asynchronous exception,
which can be handled normally as any L{Deferred} would be::

    def trapZero(result):
        result.trap(ZeroDivisionError)
        print "Divided by zero: returning INF"
        return 1e1000
    ClientCreator(reactor, amp.AMP).connectTCP(...).addCallback(
        lambda p: p.callRemote(Divide, numerator=1234,
                               denominator=0)
        ).addErrback(trapZero)

For a complete, runnable example of both of these commands, see the files in
the Twisted repository::

    doc/core/examples/ampserver.py
    doc/core/examples/ampclient.py

On the wire, AMP is a protocol which uses 2-byte lengths to prefix keys and
values, and empty keys to separate messages::

    <2-byte length><key><2-byte length><value>
    <2-byte length><key><2-byte length><value>
    ...
    <2-byte length><key><2-byte length><value>
    <NUL><NUL>                  # Empty Key == End of Message

And so on.  Because it's tedious to refer to lengths and NULs constantly, the
documentation will refer to packets as if they were newline delimited, like
so::

    C: _command: sum
    C: _ask: ef639e5c892ccb54
    C: a: 13
    C: b: 81

    S: _answer: ef639e5c892ccb54
    S: total: 94

Notes:

Values are limited to the maximum encodable size in a 16-bit length, 65535
bytes.

Keys are limited to the maximum encodable size in a 8-bit length, 255 bytes.
Note that we still use 2-byte lengths to encode keys.  This small redundancy
has several features:

    - If an implementation becomes confused and starts emitting corrupt data,
      or gets keys confused with values, many common errors will be
      signalled immediately instead of delivering obviously corrupt packets.

    - A single NUL will separate every key, and a double NUL separates
      messages.  This provides some redundancy when debugging traffic dumps.

    - NULs will be present at regular intervals along the protocol, providing
      some padding for otherwise braindead C implementations of the protocol,
      so that <stdio.h> string functions will see the NUL and stop.

    - This makes it possible to run an AMP server on a port also used by a
      plain-text protocol, and easily distinguish between non-AMP clients (like
      web browsers) which issue non-NUL as the first byte, and AMP clients,
      which always issue NUL as the first byte.

"""

__metaclass__ = type

import types, warnings

from cStringIO import StringIO
from struct import pack

from zope.interface import Interface, implements

from twisted.python.reflect import accumulateClassDict
from twisted.python.failure import Failure
from twisted.python import log, filepath

from twisted.internet.main import CONNECTION_LOST
from twisted.internet.error import PeerVerifyError, ConnectionLost
from twisted.internet.defer import Deferred, maybeDeferred, fail
from twisted.protocols.basic import Int16StringReceiver, StatefulStringProtocol

from twisted.internet._sslverify import problemsFromTransport

# I'd like this to use the exposed public API, but for some reason, when it was
# moved, these names were not exposed by internet.ssl.

from twisted.internet.ssl import CertificateOptions, Certificate, DN, KeyPair

ASK = '_ask'
ANSWER = '_answer'
COMMAND = '_command'
ERROR = '_error'
ERROR_CODE = '_error_code'
ERROR_DESCRIPTION = '_error_description'
UNKNOWN_ERROR_CODE = 'UNKNOWN'
UNHANDLED_ERROR_CODE = 'UNHANDLED'

MAX_KEY_LENGTH = 0xff
MAX_VALUE_LENGTH = 0xffff


class IBoxSender(Interface):
    """
    A transport which can send L{AmpBox} objects.
    """

    def sendBox(box):
        """
        Send an L{AmpBox}.

        @raise ProtocolSwitched: if the underlying protocol has been
        switched.

        @raise ConnectionLost: if the underlying connection has already been
        lost.
        """

    def unhandledError(failure):
        """
        An unhandled error occurred in response to a box.  Log it
        appropriately.

        @param failure: a L{Failure} describing the error that occurred.
        """



class IBoxReceiver(Interface):
    """
    An application object which can receive L{AmpBox} objects and dispatch them
    appropriately.
    """

    def startReceivingBoxes(boxSender):
        """
        The L{ampBoxReceived} method will start being called; boxes may be
        responded to by responding to the given L{IBoxSender}.

        @param boxSender: an L{IBoxSender} provider.
        """


    def ampBoxReceived(box):
        """
        A box was received from the transport; dispatch it appropriately.
        """


    def stopReceivingBoxes(reason):
        """
        No further boxes will be received on this connection.

        @type reason: L{Failure}
        """



class IResponderLocator(Interface):
    """
    An application object which can look up appropriate responder methods for
    AMP commands.
    """

    def locateResponder(self, name):
        """
        Locate a responder method appropriate for the named command.

        @param name: the wire-level name (commandName) of the AMP command to be
        responded to.

        @return: a 1-argument callable that takes an L{AmpBox} with argument
        values for the given command, and returns an L{AmpBox} containing
        argument values for the named command, or a L{Deferred} that fires the
        same.
        """



class AmpError(Exception):
    """
    Base class of all Amp-related exceptions.
    """



class ProtocolSwitched(Exception):
    """
    Connections which have been switched to other protocols can no longer
    accept traffic at the AMP level.  This is raised when you try to send it.
    """



class OnlyOneTLS(AmpError):
    """
    This is an implementation limitation; TLS may only be started once per
    connection.
    """



class NoEmptyBoxes(AmpError):
    """
    You can't have empty boxes on the connection.  This is raised when you
    receive or attempt to send one.
    """



class InvalidSignature(AmpError):
    """
    You didn't pass all the required arguments.
    """



class TooLong(AmpError):
    """
    One of the protocol's length limitations was violated.

    @ivar isKey: true if the string being encoded in a key position, false if
    it was in a value position.

    @ivar isLocal: Was the string encoded locally, or received too long from
    the network?  (It's only physically possible to encode "too long" values on
    the network for keys.)

    @ivar value: The string that was too long.

    @ivar keyName: If the string being encoded was in a value position, what
    key was it being encoded for?
    """

    def __init__(self, isKey, isLocal, value, keyName=None):
        AmpError.__init__(self)
        self.isKey = isKey
        self.isLocal = isLocal
        self.value = value
        self.keyName = keyName


    def __repr__(self):
        hdr = self.isKey and "key" or "value"
        if not self.isKey:
            hdr += ' ' + repr(self.keyName)
        lcl = self.isLocal and "local" or "remote"
        return "%s %s too long: %d" % (lcl, hdr, len(self.value))



class BadLocalReturn(AmpError):
    """
    A bad value was returned from a local command; we were unable to coerce it.
    """
    def __init__(self, message, enclosed):
        AmpError.__init__(self)
        self.message = message
        self.enclosed = enclosed


    def __repr__(self):
        return self.message + " " + self.enclosed.getBriefTraceback()

    __str__ = __repr__



class RemoteAmpError(AmpError):
    """
    This error indicates that something went wrong on the remote end of the
    connection, and the error was serialized and transmitted to you.
    """
    def __init__(self, errorCode, description, fatal=False, local=None):
        """Create a remote error with an error code and description.

        @param errorCode: the AMP error code of this error.

        @param description: some text to show to the user.

        @param fatal: a boolean, true if this error should terminate the
        connection.

        @param local: a local Failure, if one exists.
        """
        if local:
            localwhat = ' (local)'
            othertb = local.getBriefTraceback()
        else:
            localwhat = ''
            othertb = ''
        Exception.__init__(self, "Code<%s>%s: %s%s" % (
                errorCode, localwhat,
                description, othertb))
        self.local = local
        self.errorCode = errorCode
        self.description = description
        self.fatal = fatal



class UnknownRemoteError(RemoteAmpError):
    """
    This means that an error whose type we can't identify was raised from the
    other side.
    """
    def __init__(self, description):
        errorCode = UNKNOWN_ERROR_CODE
        RemoteAmpError.__init__(self, errorCode, description)



class MalformedAmpBox(AmpError):
    """
    This error indicates that the wire-level protocol was malformed.
    """



class UnhandledCommand(AmpError):
    """
    A command received via amp could not be dispatched.
    """



class IncompatibleVersions(AmpError):
    """
    It was impossible to negotiate a compatible version of the protocol with
    the other end of the connection.
    """


PROTOCOL_ERRORS = {UNHANDLED_ERROR_CODE: UnhandledCommand}

class AmpBox(dict):
    """
    I am a packet in the AMP protocol, much like a regular str:str dictionary.
    """
    __slots__ = []              # be like a regular dictionary, don't magically
                                # acquire a __dict__...


    def copy(self):
        """
        Return another AmpBox just like me.
        """
        newBox = self.__class__()
        newBox.update(self)
        return newBox


    def serialize(self):
        """
        Convert me into a wire-encoded string.

        @return: a str encoded according to the rules described in the module
        docstring.
        """
        i = self.items()
        i.sort()
        L = []
        w = L.append
        for k, v in i:
            if len(k) > MAX_KEY_LENGTH:
                raise TooLong(True, True, k, None)
            if len(v) > MAX_VALUE_LENGTH:
                raise TooLong(False, True, v, k)
            for kv in k, v:
                w(pack("!H", len(kv)))
                w(kv)
        w(pack("!H", 0))
        return ''.join(L)


    def _sendTo(self, proto):
        """
        Serialize and send this box to a Amp instance.  By the time it is being
        sent, several keys are required.  I must have exactly ONE of::

            _ask
            _answer
            _error

        If the '_ask' key is set, then the '_command' key must also be
        set.

        @param proto: an AMP instance.
        """
        proto.sendBox(self)

    def __repr__(self):
        return 'AmpBox(%s)' % (dict.__repr__(self),)

# amp.Box => AmpBox

Box = AmpBox

class QuitBox(AmpBox):
    """
    I am an AmpBox that, upon being sent, terminates the connection.
    """
    __slots__ = []


    def __repr__(self):
        return 'QuitBox(**%s)' % (super(QuitBox, self).__repr__(),)


    def _sendTo(self, proto):
        """
        Immediately call loseConnection after sending.
        """
        super(QuitBox, self)._sendTo(proto)
        proto.transport.loseConnection()



class _SwitchBox(AmpBox):
    """
    Implementation detail of ProtocolSwitchCommand: I am a AmpBox which sets
    up state for the protocol to switch.
    """

    # DON'T set __slots__ here; we do have an attribute.

    def __init__(self, innerProto, **kw):
        """
        Create a _SwitchBox with the protocol to switch to after being sent.

        @param innerProto: the protocol instance to switch to.
        @type innerProto: an IProtocol provider.
        """
        super(_SwitchBox, self).__init__(**kw)
        self.innerProto = innerProto


    def __repr__(self):
        return '_SwitchBox(%r, **%s)' % (self.innerProto,
                                         dict.__repr__(self),)


    def _sendTo(self, proto):
        """
        Send me; I am the last box on the connection.  All further traffic will be
        over the new protocol.
        """
        super(_SwitchBox, self)._sendTo(proto)
        proto._lockForSwitch()
        proto._switchTo(self.innerProto)



class BoxDispatcher:
    """
    A L{BoxDispatcher} dispatches '_ask', '_answer', and '_error' L{AmpBox}es,
    both incoming and outgoing, to their appropriate destinations.

    Outgoing commands are converted into L{Deferred}s and outgoing boxes, and
    associated tracking state to fire those L{Deferred} when '_answer' boxes
    come back.  Incoming '_answer' and '_error' boxes are converted into
    callbacks and errbacks on those L{Deferred}s, respectively.

    Incoming '_ask' boxes are converted into method calls on a supplied method
    locator.

    @ivar _outstandingRequests: a dictionary mapping request IDs to
    L{Deferred}s which were returned for those requests.

    @ivar locator: an object with a L{locateResponder} method that locates a
    responder function that takes a Box and returns a result (either a Box or a
    Deferred which fires one).

    @ivar boxSender: an object which can send boxes, via the L{_sendBox}
    method, such as an L{AMP} instance.
    @type boxSender: L{IBoxSender}
    """

    implements(IBoxReceiver)

    _failAllReason = None
    _outstandingRequests = None
    _counter = 0L
    boxSender = None

    def __init__(self, locator):
        self._outstandingRequests = {}
        self.locator = locator


    def startReceivingBoxes(self, boxSender):
        """
        The given boxSender is going to start calling boxReceived on this
        L{BoxDispatcher}.

        @param boxSender: The L{IBoxSender} to send command responses to.
        """
        self.boxSender = boxSender


    def stopReceivingBoxes(self, reason):
        """
        No further boxes will be received here.  Terminate all currently
        oustanding command deferreds with the given reason.
        """
        self.failAllOutgoing(reason)


    def failAllOutgoing(self, reason):
        """
        Call the errback on all outstanding requests awaiting responses.

        @param reason: the Failure instance to pass to those errbacks.
        """
        self._failAllReason = reason
        OR = self._outstandingRequests.items()
        self._outstandingRequests = None # we can never send another request
        for key, value in OR:
            value.errback(reason)


    def _nextTag(self):
        """
        Generate protocol-local serial numbers for _ask keys.

        @return: a string that has not yet been used on this connection.
        """
        self._counter += 1
        return '%x' % (self._counter,)


    def _sendBoxCommand(self, command, box, requiresAnswer=True):
        """
        Send a command across the wire with the given C{amp.Box}.

        Mutate the given box to give it any additional keys (_command, _ask)
        required for the command and request/response machinery, then send it.

        If requiresAnswer is True, returns a C{Deferred} which fires when a
        response is received. The C{Deferred} is fired with an C{amp.Box} on
        success, or with an C{amp.RemoteAmpError} if an error is received.

        If the Deferred fails and the error is not handled by the caller of
        this method, the failure will be logged and the connection dropped.

        @param command: a str, the name of the command to issue.

        @param box: an AmpBox with the arguments for the command.

        @param requiresAnswer: a boolean.  Defaults to True.  If True, return a
        Deferred which will fire when the other side responds to this command.
        If False, return None and do not ask the other side for acknowledgement.

        @return: a Deferred which fires the AmpBox that holds the response to
        this command, or None, as specified by requiresAnswer.

        @raise ProtocolSwitched: if the protocol has been switched.
        """
        if self._failAllReason is not None:
            return fail(self._failAllReason)
        box[COMMAND] = command
        tag = self._nextTag()
        if requiresAnswer:
            box[ASK] = tag
        box._sendTo(self.boxSender)
        if requiresAnswer:
            result = self._outstandingRequests[tag] = Deferred()
        else:
            result = None
        return result


    def callRemoteString(self, command, requiresAnswer=True, **kw):
        """
        This is a low-level API, designed only for optimizing simple messages
        for which the overhead of parsing is too great.

        @param command: a str naming the command.

        @param kw: arguments to the amp box.

        @param requiresAnswer: a boolean.  Defaults to True.  If True, return a
        Deferred which will fire when the other side responds to this command.
        If False, return None and do not ask the other side for acknowledgement.

        @return: a Deferred which fires the AmpBox that holds the response to
        this command, or None, as specified by requiresAnswer.
        """
        box = Box(kw)
        return self._sendBoxCommand(command, box)


    def callRemote(self, commandType, *a, **kw):
        """
        This is the primary high-level API for sending messages via AMP.  Invoke it
        with a command and appropriate arguments to send a message to this
        connection's peer.

        @param commandType: a subclass of Command.
        @type commandType: L{type}

        @param a: Positional (special) parameters taken by the command.
        Positional parameters will typically not be sent over the wire.  The
        only command included with AMP which uses positional parameters is
        L{ProtocolSwitchCommand}, which takes the protocol that will be
        switched to as its first argument.

        @param kw: Keyword arguments taken by the command.  These are the
        arguments declared in the command's 'arguments' attribute.  They will
        be encoded and sent to the peer as arguments for the L{commandType}.

        @return: If L{commandType} has a C{requiresAnswer} attribute set to
        L{False}, then return L{None}.  Otherwise, return a L{Deferred} which
        fires with a dictionary of objects representing the result of this
        call.  Additionally, this L{Deferred} may fail with an exception
        representing a connection failure, with L{UnknownRemoteError} if the
        other end of the connection fails for an unknown reason, or with any
        error specified as a key in L{commandType}'s C{errors} dictionary.
        """

        # XXX this takes command subclasses and not command objects on purpose.
        # There's really no reason to have all this back-and-forth between
        # command objects and the protocol, and the extra object being created
        # (the Command instance) is pointless.  Command is kind of like
        # Interface, and should be more like it.

        # In other words, the fact that commandType is instantiated here is an
        # implementation detail.  Don't rely on it.

        co = commandType(*a, **kw)
        return co._doCommand(self)


    def unhandledError(self, failure):
        """
        This is a terminal callback called after application code has had a
        chance to quash any errors.
        """
        return self.boxSender.unhandledError(failure)


    def _answerReceived(self, box):
        """
        An AMP box was received that answered a command previously sent with
        L{callRemote}.

        @param box: an AmpBox with a value for its L{ANSWER} key.
        """
        question = self._outstandingRequests.pop(box[ANSWER])
        question.addErrback(self.unhandledError)
        question.callback(box)


    def _errorReceived(self, box):
        """
        An AMP box was received that answered a command previously sent with
        L{callRemote}, with an error.

        @param box: an L{AmpBox} with a value for its L{ERROR}, L{ERROR_CODE},
        and L{ERROR_DESCRIPTION} keys.
        """
        question = self._outstandingRequests.pop(box[ERROR])
        question.addErrback(self.unhandledError)
        errorCode = box[ERROR_CODE]
        description = box[ERROR_DESCRIPTION]
        if errorCode in PROTOCOL_ERRORS:
            exc = PROTOCOL_ERRORS[errorCode](errorCode, description)
        else:
            exc = RemoteAmpError(errorCode, description)
        question.errback(Failure(exc))


    def _commandReceived(self, box):
        """
        @param box: an L{AmpBox} with a value for its L{COMMAND} and L{ASK}
        keys.
        """
        cmd = box[COMMAND]
        def formatAnswer(answerBox):
            answerBox[ANSWER] = box[ASK]
            return answerBox
        def formatError(error):
            if error.check(RemoteAmpError):
                code = error.value.errorCode
                desc = error.value.description
                if error.value.fatal:
                    errorBox = QuitBox()
                else:
                    errorBox = AmpBox()
            else:
                errorBox = QuitBox()
                log.err(error) # here is where server-side logging happens
                               # if the error isn't handled
                code = UNKNOWN_ERROR_CODE
                desc = "Unknown Error"
            errorBox[ERROR] = box[ASK]
            errorBox[ERROR_DESCRIPTION] = desc
            errorBox[ERROR_CODE] = code
            return errorBox
        deferred = self.dispatchCommand(box)
        if ASK in box:
            deferred.addCallbacks(formatAnswer, formatError)
            deferred.addCallback(self._safeEmit)
        deferred.addErrback(self.unhandledError)


    def ampBoxReceived(self, box):
        """
        An AmpBox was received, representing a command, or an answer to a
        previously issued command (either successful or erroneous).  Respond to
        it according to its contents.

        @param box: an AmpBox

        @raise NoEmptyBoxes: when a box is received that does not contain an
        '_answer', '_command' / '_ask', or '_error' key; i.e. one which does not
        fit into the command / response protocol defined by AMP.
        """
        if ANSWER in box:
            self._answerReceived(box)
        elif ERROR in box:
            self._errorReceived(box)
        elif COMMAND in box:
            self._commandReceived(box)
        else:
            raise NoEmptyBoxes(box)


    def _safeEmit(self, aBox):
        """
        Emit a box, ignoring L{ProtocolSwitched} and L{ConnectionLost} errors
        which cannot be usefully handled.
        """
        try:
            aBox._sendTo(self.boxSender)
        except (ProtocolSwitched, ConnectionLost):
            pass


    def dispatchCommand(self, box):
        """
        A box with a _command key was received.

        Dispatch it to a local handler call it.

        @param proto: an AMP instance.
        @param box: an AmpBox to be dispatched.
        """
        cmd = box[COMMAND]
        responder = self.locator.locateResponder(cmd)
        if responder is None:
            return fail(RemoteAmpError(
                    UNHANDLED_ERROR_CODE,
                    "Unhandled Command: %r" % (cmd,),
                    False,
                    local=Failure(UnhandledCommand())))
        return maybeDeferred(responder, box)



class CommandLocator:
    """
    A L{CommandLocator} is a collection of responders to AMP L{Command}s, with
    the help of the L{Command.responder} decorator.
    """

    class __metaclass__(type):
        """
        This metaclass keeps track of all of the Command.responder-decorated
        methods defined since the last CommandLocator subclass was defined.  It
        assumes (usually correctly, but unfortunately not necessarily so) that
        those commands responders were all declared as methods of the class
        being defined.  Note that this list can be incorrect if users use the
        Command.responder decorator outside the context of a CommandLocator
        class declaration.

        The Command.responder decorator explicitly cooperates with this
        metaclass.
        """

        _currentClassCommands = []
        def __new__(cls, name, bases, attrs):
            commands = cls._currentClassCommands[:]
            cls._currentClassCommands[:] = []
            cd = attrs['_commandDispatch'] = {}
            for base in bases:
                cls._grabFromBase(cd, base)
            for commandClass, responderFunc in commands:
                cd[commandClass.commandName] = (commandClass, responderFunc)
            subcls = type.__new__(cls, name, bases, attrs)
            if (bases and (
                    subcls.lookupFunction != CommandLocator.lookupFunction)):
                def locateResponder(self, name):
                    warnings.warn(
                        "Override locateResponder, not lookupFunction.",
                        category=PendingDeprecationWarning,
                        stacklevel=2)
                    return self.lookupFunction(name)
                subcls.locateResponder = locateResponder
            return subcls

        def _grabFromBase(cls, cd, base):
            if hasattr(base, "_commandDispatch"):
                cd.update(base._commandDispatch)
                for subbase in base.__bases__:
                    cls._grabFromBase(cd, subbase)
        _grabFromBase = classmethod(_grabFromBase)

    implements(IResponderLocator)


    def _wrapWithSerialization(self, aCallable, command):
        """
        Wrap aCallable with its command's argument de-serialization
        and result serialization logic.

        @param aCallable: a callable with a 'command' attribute, designed to be
        called with keyword arguments.

        @param command: the command class whose serialization to use.

        @return: a 1-arg callable which, when invoked with an AmpBox, will
        deserialize the argument list and invoke appropriate user code for the
        callable's command, returning a Deferred which fires with the result or
        fails with an error.
        """
        def doit(box):
            kw = command.parseArguments(box, self)
            def checkKnownErrors(error):
                key = error.trap(*command.allErrors)
                code = command.allErrors[key]
                desc = str(error.value)
                return Failure(RemoteAmpError(
                        code, desc, key in command.fatalErrors, local=error))
            def makeResponseFor(objects):
                try:
                    return command.makeResponse(objects, self)
                except:
                    # let's helpfully log this.
                    originalFailure = Failure()
                    raise BadLocalReturn(
                        "%r returned %r and %r could not serialize it" % (
                            aCallable,
                            objects,
                            command),
                        originalFailure)
            return maybeDeferred(aCallable, **kw).addCallback(
                makeResponseFor).addErrback(
                checkKnownErrors)
        return doit


    def lookupFunction(self, name):
        """
        Deprecated synonym for L{locateResponder}
        """
        if self.__class__.lookupFunction != CommandLocator.lookupFunction:
            return CommandLocator.locateResponder(self, name)
        else:
            warnings.warn("Call locateResponder, not lookupFunction.",
                          category=PendingDeprecationWarning,
                          stacklevel=2)
        return self.locateResponder(name)


    def locateResponder(self, name):
        """
        Locate a callable to invoke when executing the named command.

        @param name: the normalized name (from the wire) of the command.

        @return: a 1-argument function that takes a Box and returns a box or a
        Deferred which fires a Box, for handling the command identified by the
        given name, or None, if no appropriate responder can be found.
        """
        # Try to find a high-level method to invoke, and if we can't find one,
        # fall back to a low-level one.
        cd = self._commandDispatch
        if name in cd:
            commandClass, responderFunc = cd[name]
            responderMethod = types.MethodType(
                responderFunc, self, self.__class__)
            return self._wrapWithSerialization(responderMethod, commandClass)



class SimpleStringLocator(object):
    """
    Implement the L{locateResponder} method to do simple, string-based
    dispatch.
    """

    implements(IResponderLocator)

    baseDispatchPrefix = 'amp_'

    def locateResponder(self, name):
        """
        Locate a callable to invoke when executing the named command.

        @return: a function with the name C{"amp_" + name} on L{self}, or None
        if no such function exists.  This function will then be called with the
        L{AmpBox} itself as an argument.

        @param name: the normalized name (from the wire) of the command.
        """
        fName = self.baseDispatchPrefix + (name.upper())
        return getattr(self, fName, None)



PYTHON_KEYWORDS = [
    'and', 'del', 'for', 'is', 'raise', 'assert', 'elif', 'from', 'lambda',
    'return', 'break', 'else', 'global', 'not', 'try', 'class', 'except',
    'if', 'or', 'while', 'continue', 'exec', 'import', 'pass', 'yield',
    'def', 'finally', 'in', 'print']



def _wireNameToPythonIdentifier(key):
    """
    (Private) Normalize an argument name from the wire for use with Python
    code.  If the return value is going to be a python keyword it will be
    capitalized.  If it contains any dashes they will be replaced with
    underscores.

    The rationale behind this method is that AMP should be an inherently
    multi-language protocol, so message keys may contain all manner of bizarre
    bytes.  This is not a complete solution; there are still forms of arguments
    that this implementation will be unable to parse.  However, Python
    identifiers share a huge raft of properties with identifiers from many
    other languages, so this is a 'good enough' effort for now.  We deal
    explicitly with dashes because that is the most likely departure: Lisps
    commonly use dashes to separate method names, so protocols initially
    implemented in a lisp amp dialect may use dashes in argument or command
    names.

    @param key: a str, looking something like 'foo-bar-baz' or 'from'

    @return: a str which is a valid python identifier, looking something like
    'foo_bar_baz' or 'From'.
    """
    lkey = key.replace("-", "_")
    if lkey in PYTHON_KEYWORDS:
        return lkey.title()
    return lkey



class Argument:
    """
    Base-class of all objects that take values from Amp packets and convert
    them into objects for Python functions.
    """
    optional = False


    def __init__(self, optional=False):
        """
        Create an Argument.

        @param optional: a boolean indicating whether this argument can be
        omitted in the protocol.
        """
        self.optional = optional


    def retrieve(self, d, name, proto):
        """
        Retrieve the given key from the given dictionary, removing it if found.

        @param d: a dictionary.

        @param name: a key in L{d}.

        @param proto: an instance of an AMP.

        @raise KeyError: if I am not optional and no value was found.

        @return: d[name].
        """
        if self.optional:
            value = d.get(name)
            if value is not None:
                del d[name]
        else:
            value = d.pop(name)
        return value


    def fromBox(self, name, strings, objects, proto):
        """
        Populate an 'out' dictionary with mapping names to Python values
        decoded from an 'in' AmpBox mapping strings to string values.

        @param name: the argument name to retrieve
        @type name: str

        @param strings: The AmpBox to read string(s) from, a mapping of
        argument names to string values.
        @type strings: AmpBox

        @param objects: The dictionary to write object(s) to, a mapping of
        names to Python objects.
        @type objects: dict

        @param proto: an AMP instance.
        """
        st = self.retrieve(strings, name, proto)
        nk = _wireNameToPythonIdentifier(name)
        if self.optional and st is None:
            objects[nk] = None
        else:
            objects[nk] = self.fromStringProto(st, proto)


    def toBox(self, name, strings, objects, proto):
        """
        Populate an 'out' AmpBox with strings encoded from an 'in' dictionary
        mapping names to Python values.

        @param name: the argument name to retrieve
        @type name: str

        @param strings: The AmpBox to write string(s) to, a mapping of
        argument names to string values.
        @type strings: AmpBox

        @param objects: The dictionary to read object(s) from, a mapping of
        names to Python objects.

        @type objects: dict

        @param proto: the protocol we are converting for.
        @type proto: AMP
        """
        obj = self.retrieve(objects, _wireNameToPythonIdentifier(name), proto)
        if self.optional and obj is None:
            # strings[name] = None
            pass
        else:
            strings[name] = self.toStringProto(obj, proto)


    def fromStringProto(self, inString, proto):
        """
        Convert a string to a Python value.

        @param inString: the string to convert.

        @param proto: the protocol we are converting for.
        @type proto: AMP

        @return: a Python object.
        """
        return self.fromString(inString)


    def toStringProto(self, inObject, proto):
        """
        Convert a Python object to a string.

        @param inObject: the object to convert.

        @param proto: the protocol we are converting for.
        @type proto: AMP
        """
        return self.toString(inObject)


    def fromString(self, inString):
        """
        Convert a string to a Python object.  Subclasses must implement this.

        @param inString: the string to convert.
        @type inString: str

        @return: the decoded value from inString
        """


    def toString(self, inObject):
        """
        Convert a Python object into a string for passing over the network.

        @param inObject: an object of the type that this Argument is intended
        to deal with.

        @return: the wire encoding of inObject
        @rtype: str
        """



class Integer(Argument):
    """
    Convert to and from 'int'.
    """
    fromString = int
    def toString(self, inObject):
        return str(int(inObject))



class String(Argument):
    """
    Don't do any conversion at all; just pass through 'str'.
    """
    def toString(self, inObject):
        return inObject


    def fromString(self, inString):
        return inString



class Float(Argument):
    """
    Encode floating-point values on the wire as their repr.
    """
    fromString = float
    toString = repr



class Boolean(Argument):
    """
    Encode True or False as "True" or "False" on the wire.
    """
    def fromString(self, inString):
        if inString == 'True':
            return True
        elif inString == 'False':
            return False
        else:
            raise TypeError("Bad boolean value: %r" % (inString,))


    def toString(self, inObject):
        if inObject:
            return 'True'
        else:
            return 'False'



class Unicode(String):
    """
    Encode a unicode string on the wire as UTF-8.
    """

    def toString(self, inObject):
        # assert isinstance(inObject, unicode)
        return String.toString(self, inObject.encode('utf-8'))


    def fromString(self, inString):
        # assert isinstance(inString, str)
        return String.fromString(self, inString).decode('utf-8')



class Path(Unicode):
    """
    Encode and decode L{filepath.FilePath} instances as paths on the wire.

    This is really intended for use with subprocess communication tools:
    exchanging pathnames on different machines over a network is not generally
    meaningful, but neither is it disallowed; you can use this to communicate
    about NFS paths, for example.
    """
    def fromString(self, inString):
        return filepath.FilePath(Unicode.fromString(self, inString))


    def toString(self, inObject):
        return Unicode.toString(self, inObject.path)



class AmpList(Argument):
    """
    Convert a list of dictionaries into a list of AMP boxes on the wire.

    For example, if you want to pass::

        [{'a': 7, 'b': u'hello'}, {'a': 9, 'b': u'goodbye'}]

    You might use an AmpList like this in your arguments or response list::

        AmpList([('a', Integer()),
                 ('b', Unicode())])
    """
    def __init__(self, subargs):
        """
        Create an AmpList.

        @param subargs: a list of 2-tuples of ('name', argument) describing the
        schema of the dictionaries in the sequence of amp boxes.
        """
        self.subargs = subargs


    def fromStringProto(self, inString, proto):
        boxes = parseString(inString)
        values = [_stringsToObjects(box, self.subargs, proto)
                  for box in boxes]
        return values


    def toStringProto(self, inObject, proto):
        return ''.join([_objectsToStrings(
                    objects, self.subargs, Box(), proto
                    ).serialize() for objects in inObject])

class Command:
    """
    Subclass me to specify an AMP Command.

    @cvar arguments: A list of 2-tuples of (name, Argument-subclass-instance),
    specifying the names and values of the parameters which are required for
    this command.

    @cvar response: A list like L{arguments}, but instead used for the return
    value.

    @cvar errors: A mapping of subclasses of L{Exception} to wire-protocol tags
    for errors represented as L{str}s.  Responders which raise keys from this
    dictionary will have the error translated to the corresponding tag on the
    wire.  Invokers which receive Deferreds from invoking this command with
    L{AMP.callRemote} will potentially receive Failures with keys from this
    mapping as their value.  This mapping is inherited; if you declare a
    command which handles C{FooError} as 'FOO_ERROR', then subclass it and
    specify C{BarError} as 'BAR_ERROR', responders to the subclass may raise
    either C{FooError} or C{BarError}, and invokers must be able to deal with
    either of those exceptions.

    @cvar fatalErrors: like 'errors', but errors in this list will always
    terminate the connection, despite being of a recognizable error type.

    @cvar commandType: The type of Box used to issue commands; useful only for
    protocol-modifying behavior like startTLS or protocol switching.  Defaults
    to a plain vanilla L{Box}.

    @cvar responseType: The type of Box used to respond to this command; only
    useful for protocol-modifying behavior like startTLS or protocol switching.
    Defaults to a plain vanilla L{Box}.

    @ivar requiresAnswer: a boolean; defaults to True.  Set it to False on your
    subclass if you want callRemote to return None.  Note: this is a hint only
    to the client side of the protocol.  The return-type of a command responder
    method must always be a dictionary adhering to the contract specified by
    L{response}, because clients are always free to request a response if they
    want one.
    """

    class __metaclass__(type):
        """
        Metaclass hack to establish reverse-mappings for 'errors' and
        'fatalErrors' as class vars.
        """
        def __new__(cls, name, bases, attrs):
            re = attrs['reverseErrors'] = {}
            er = attrs['allErrors'] = {}
            if 'commandName' not in attrs:
                attrs['commandName'] = name
            newtype = type.__new__(cls, name, bases, attrs)
            errors = {}
            fatalErrors = {}
            accumulateClassDict(newtype, 'errors', errors)
            accumulateClassDict(newtype, 'fatalErrors', fatalErrors)
            for v, k in errors.iteritems():
                re[k] = v
                er[v] = k
            for v, k in fatalErrors.iteritems():
                re[k] = v
                er[v] = k
            return newtype

    arguments = []
    response = []
    extra = []
    errors = {}
    fatalErrors = {}

    commandType = Box
    responseType = Box

    requiresAnswer = True


    def __init__(self, **kw):
        """
        Create an instance of this command with specified values for its
        parameters.

        @param kw: a dict containing an appropriate value for each name
        specified in the L{arguments} attribute of my class.

        @raise InvalidSignature: if you forgot any required arguments.
        """
        self.structured = kw
        givenArgs = kw.keys()
        forgotten = []
        for name, arg in self.arguments:
            pythonName = _wireNameToPythonIdentifier(name)
            if pythonName not in givenArgs and not arg.optional:
                forgotten.append(pythonName)
        if forgotten:
            raise InvalidSignature("forgot %s for %s" % (
                    ', '.join(forgotten), self.commandName))
        forgotten = []


    def makeResponse(cls, objects, proto):
        """
        Serialize a mapping of arguments using this L{Command}'s
        response schema.

        @param objects: a dict with keys matching the names specified in
        self.response, having values of the types that the Argument objects in
        self.response can format.

        @param proto: an L{AMP}.

        @return: an L{AmpBox}.
        """
        return _objectsToStrings(objects, cls.response, cls.responseType(),
                                 proto)
    makeResponse = classmethod(makeResponse)


    def makeArguments(cls, objects, proto):
        """
        Serialize a mapping of arguments using this L{Command}'s
        argument schema.

        @param objects: a dict with keys similar to the names specified in
        self.arguments, having values of the types that the Argument objects in
        self.arguments can parse.

        @param proto: an L{AMP}.

        @return: An instance of this L{Command}'s C{commandType}.
        """
        return _objectsToStrings(objects, cls.arguments, cls.commandType(),
                                 proto)
    makeArguments = classmethod(makeArguments)


    def parseResponse(cls, box, protocol):
        """
        Parse a mapping of serialized arguments using this
        L{Command}'s response schema.

        @param box: A mapping of response-argument names to the
        serialized forms of those arguments.
        @param protocol: The L{AMP} protocol.

        @return: A mapping of response-argument names to the parsed
        forms.
        """
        return _stringsToObjects(box, cls.response, protocol)
    parseResponse = classmethod(parseResponse)


    def parseArguments(cls, box, protocol):
        """
        Parse a mapping of serialized arguments using this
        L{Command}'s argument schema.

        @param box: A mapping of argument names to the seralized forms
        of those arguments.
        @param protocol: The L{AMP} protocol.

        @return: A mapping of argument names to the parsed forms.
        """
        return _stringsToObjects(box, cls.arguments, protocol)
    parseArguments = classmethod(parseArguments)


    def responder(cls, methodfunc):
        """
        Declare a method to be a responder for a particular command.

        This is a decorator.

        Use like so::

            class MyCommand(Command):
                arguments = [('a', ...), ('b', ...)]

            class MyProto(AMP):
                def myFunMethod(self, a, b):
                    ...
                MyCommand.responder(myFunMethod)

        Notes: Although decorator syntax is not used within Twisted, this
        function returns its argument and is therefore safe to use with
        decorator syntax.

        This is not thread safe.  Don't declare AMP subclasses in other
        threads.  Don't declare responders outside the scope of AMP subclasses;
        the behavior is undefined.

        @param methodfunc: A function which will later become a method, which
        has a keyword signature compatible with this command's L{argument} list
        and returns a dictionary with a set of keys compatible with this
        command's L{response} list.

        @return: the methodfunc parameter.
        """
        CommandLocator._currentClassCommands.append((cls, methodfunc))
        return methodfunc
    responder = classmethod(responder)


    # Our only instance method
    def _doCommand(self, proto):
        """
        Encode and send this Command to the given protocol.

        @param proto: an AMP, representing the connection to send to.

        @return: a Deferred which will fire or error appropriately when the
        other side responds to the command (or error if the connection is lost
        before it is responded to).
        """

        def _massageError(error):
            error.trap(RemoteAmpError)
            rje = error.value
            errorType = self.reverseErrors.get(rje.errorCode,
                                               UnknownRemoteError)
            return Failure(errorType(rje.description))

        d = proto._sendBoxCommand(self.commandName,
                                  self.makeArguments(self.structured, proto),
                                  self.requiresAnswer)

        if self.requiresAnswer:
            d.addCallback(self.parseResponse, proto)
            d.addErrback(_massageError)

        return d



class _NoCertificate:
    """
    This is for peers which don't want to use a local certificate.  Used by
    AMP because AMP's internal language is all about certificates and this
    duck-types in the appropriate place; this API isn't really stable though,
    so it's not exposed anywhere public.

    For clients, it will use ephemeral DH keys, or whatever the default is for
    certificate-less clients in OpenSSL.  For servers, it will generate a
    temporary self-signed certificate with garbage values in the DN and use
    that.
    """

    def __init__(self, client):
        """
        Create a _NoCertificate which either is or isn't for the client side of
        the connection.

        @param client: True if we are a client and should truly have no
        certificate and be anonymous, False if we are a server and actually
        have to generate a temporary certificate.

        @type client: bool
        """
        self.client = client


    def options(self, *authorities):
        """
        Behaves like L{twisted.internet.ssl.PrivateCertificate.options}().
        """
        if not self.client:
            # do some crud with sslverify to generate a temporary self-signed
            # certificate.  This is SLOOOWWWWW so it is only in the absolute
            # worst, most naive case.

            # We have to do this because OpenSSL will not let both the server
            # and client be anonymous.
            sharedDN = DN(CN='TEMPORARY CERTIFICATE')
            key = KeyPair.generate()
            cr = key.certificateRequest(sharedDN)
            sscrd = key.signCertificateRequest(sharedDN, cr, lambda dn: True, 1)
            cert = key.newCertificate(sscrd)
            return cert.options(*authorities)
        options = dict()
        if authorities:
            options.update(dict(verify=True,
                                requireCertificate=True,
                                caCerts=[auth.original for auth in authorities]))
        occo = CertificateOptions(**options)
        return occo



class _TLSBox(AmpBox):
    """
    I am an AmpBox that, upon being sent, initiates a TLS connection.
    """
    __slots__ = []

    def _keyprop(k, default):
        return property(lambda self: self.get(k, default))


    # These properties are described in startTLS
    certificate = _keyprop('tls_localCertificate', _NoCertificate(False))
    verify = _keyprop('tls_verifyAuthorities', None)

    def _sendTo(self, proto):
        """
        Send my encoded value to the protocol, then initiate TLS.
        """
        ab = AmpBox(self)
        for k in ['tls_localCertificate',
                  'tls_verifyAuthorities']:
            ab.pop(k, None)
        ab._sendTo(proto)
        proto._startTLS(self.certificate, self.verify)



class _LocalArgument(String):
    """
    Local arguments are never actually relayed across the wire.  This is just a
    shim so that StartTLS can pretend to have some arguments: if arguments
    acquire documentation properties, replace this with something nicer later.
    """

    def fromBox(self, name, strings, objects, proto):
        pass



class StartTLS(Command):
    """
    Use, or subclass, me to implement a command that starts TLS.

    Callers of StartTLS may pass several special arguments, which affect the
    TLS negotiation:

        - tls_localCertificate: This is a
        twisted.internet.ssl.PrivateCertificate which will be used to secure
        the side of the connection it is returned on.

        - tls_verifyAuthorities: This is a list of
        twisted.internet.ssl.Certificate objects that will be used as the
        certificate authorities to verify our peer's certificate.

    Each of those special parameters may also be present as a key in the
    response dictionary.
    """

    arguments = [("tls_localCertificate", _LocalArgument(optional=True)),
                 ("tls_verifyAuthorities", _LocalArgument(optional=True))]

    response = [("tls_localCertificate", _LocalArgument(optional=True)),
                ("tls_verifyAuthorities", _LocalArgument(optional=True))]

    responseType = _TLSBox

    def __init__(self, **kw):
        """
        Create a StartTLS command.  (This is private.  Use AMP.callRemote.)

        @param tls_localCertificate: the PrivateCertificate object to use to
        secure the connection.  If it's None, or unspecified, an ephemeral DH
        key is used instead.

        @param tls_verifyAuthorities: a list of Certificate objects which
        represent root certificates to verify our peer with.
        """
        self.certificate = kw.pop('tls_localCertificate', _NoCertificate(True))
        self.authorities = kw.pop('tls_verifyAuthorities', None)
        Command.__init__(self, **kw)


    def _doCommand(self, proto):
        """
        When a StartTLS command is sent, prepare to start TLS, but don't actually
        do it; wait for the acknowledgement, then initiate the TLS handshake.
        """
        d = Command._doCommand(self, proto)
        proto._prepareTLS(self.certificate, self.authorities)
        # XXX before we get back to user code we are going to start TLS...
        def actuallystart(response):
            proto._startTLS(self.certificate, self.authorities)
            return response
        d.addCallback(actuallystart)
        return d



class ProtocolSwitchCommand(Command):
    """
    Use this command to switch from something Amp-derived to a different
    protocol mid-connection.  This can be useful to use amp as the
    connection-startup negotiation phase.  Since TLS is a different layer
    entirely, you can use Amp to negotiate the security parameters of your
    connection, then switch to a different protocol, and the connection will
    remain secured.
    """

    def __init__(self, _protoToSwitchToFactory, **kw):
        """
        Create a ProtocolSwitchCommand.

        @param _protoToSwitchToFactory: a ProtocolFactory which will generate
        the Protocol to switch to.

        @param kw: Keyword arguments, encoded and handled normally as
        L{Command} would.
        """

        self.protoToSwitchToFactory = _protoToSwitchToFactory
        super(ProtocolSwitchCommand, self).__init__(**kw)


    def makeResponse(cls, innerProto, proto):
        return _SwitchBox(innerProto)
    makeResponse = classmethod(makeResponse)


    def _doCommand(self, proto):
        """
        When we emit a ProtocolSwitchCommand, lock the protocol, but don't actually
        switch to the new protocol unless an acknowledgement is received.  If
        an error is received, switch back.
        """
        d = super(ProtocolSwitchCommand, self)._doCommand(proto)
        proto._lockForSwitch()
        def switchNow(ign):
            innerProto = self.protoToSwitchToFactory.buildProtocol(
                proto.transport.getPeer())
            proto._switchTo(innerProto, self.protoToSwitchToFactory)
            return ign
        def handle(ign):
            proto._unlockFromSwitch()
            self.protoToSwitchToFactory.clientConnectionFailed(
                None, Failure(CONNECTION_LOST))
            return ign
        return d.addCallbacks(switchNow, handle)



class BinaryBoxProtocol(StatefulStringProtocol, Int16StringReceiver):
    """
    A protocol for receving L{Box}es - key/value pairs - via length-prefixed
    strings.  A box is composed of:

        - any number of key-value pairs, described by:
            - a 2-byte network-endian packed key length (of which the first
              byte must be null, and the second must be non-null: i.e. the
              value of the length must be 1-255)
            - a key, comprised of that many bytes
            - a 2-byte network-endian unsigned value length (up to the maximum
              of 65535)
            - a value, comprised of that many bytes
        - 2 null bytes

    In other words, an even number of strings prefixed with packed unsigned
    16-bit integers, and then a 0-length string to indicate the end of the box.

    This protocol also implements 2 extra private bits of functionality related
    to the byte boundaries between messages; it can start TLS between two given
    boxes or switch to an entirely different protocol.  However, due to some
    tricky elements of the implementation, the public interface to this
    functionality is L{ProtocolSwitchCommand} and L{StartTLS}.

    @ivar boxReceiver: an L{IBoxReceiver} provider, whose L{ampBoxReceived}
    method will be invoked for each L{Box} that is received.
    """

    implements(IBoxSender)

    _justStartedTLS = False
    _startingTLSBuffer = None
    _locked = False
    _currentKey = None
    _currentBox = None

    hostCertificate = None
    noPeerCertificate = False   # for tests
    innerProtocol = None
    innerProtocolClientFactory = None

    _sslVerifyProblems = ()
    # ^ Later this will become a mutable list - we can't get the handle during
    # connection shutdown thanks to the fact that Twisted destroys the socket
    # on our transport before notifying us of a lost connection (which I guess
    # is reasonable - the socket is dead by then) See a few lines below in
    # startTLS for details.  --glyph


    def __init__(self, boxReceiver):
        self.boxReceiver = boxReceiver


    def _switchTo(self, newProto, clientFactory=None):
        """
        Switch this BinaryBoxProtocol's transport to a new protocol.  You need
        to do this 'simultaneously' on both ends of a connection; the easiest
        way to do this is to use a subclass of ProtocolSwitchCommand.

        @param newProto: the new protocol instance to switch to.

        @param clientFactory: the ClientFactory to send the
        L{clientConnectionLost} notification to.
        """
        # All the data that Int16Receiver has not yet dealt with belongs to our
        # new protocol: luckily it's keeping that in a handy (although
        # ostensibly internal) variable for us:
        newProtoData = self.recvd
        # We're quite possibly in the middle of a 'dataReceived' loop in
        # Int16StringReceiver: let's make sure that the next iteration, the
        # loop will break and not attempt to look at something that isn't a
        # length prefix.
        self.recvd = ''
        # Finally, do the actual work of setting up the protocol and delivering
        # its first chunk of data, if one is available.
        self.innerProtocol = newProto
        self.innerProtocolClientFactory = clientFactory
        newProto.makeConnection(self.transport)
        newProto.dataReceived(newProtoData)


    def sendBox(self, box):
        """
        Send a amp.Box to my peer.

        Note: transport.write is never called outside of this method.

        @param box: an AmpBox.

        @raise ProtocolSwitched: if the protocol has previously been switched.

        @raise ConnectionLost: if the connection has previously been lost.
        """
        if self._locked:
            raise ProtocolSwitched(
                "This connection has switched: no AMP traffic allowed.")
        if self.transport is None:
            raise ConnectionLost()
        if self._startingTLSBuffer is not None:
            self._startingTLSBuffer.append(box)
        else:
            self.transport.write(box.serialize())


    def makeConnection(self, transport):
        """
        Notify L{boxReceiver} that it is about to receive boxes from this
        protocol by invoking L{startReceivingBoxes}.
        """
        self.boxReceiver.startReceivingBoxes(self)
        Int16StringReceiver.makeConnection(self, transport)


    def dataReceived(self, data):
        """
        Either parse incoming data as L{AmpBox}es or relay it to our nested
        protocol.
        """
        if self._justStartedTLS:
            self._justStartedTLS = False
        # If we already have an inner protocol, then we don't deliver data to
        # the protocol parser any more; we just hand it off.
        if self.innerProtocol is not None:
            self.innerProtocol.dataReceived(data)
            return
        return Int16StringReceiver.dataReceived(self, data)


    def connectionLost(self, reason):
        """
        The connection was lost; notify any nested protocol.
        """
        if self.innerProtocol is not None:
            self.innerProtocol.connectionLost(reason)
            if self.innerProtocolClientFactory is not None:
                self.innerProtocolClientFactory.clientConnectionLost(None, reason)
        # XXX this may be a slight oversimplification, but I believe that if
        # there are pending SSL errors, they _are_ the reason that the
        # connection was lost.  a totally correct implementation of this would
        # set up a simple state machine to track whether any bytes were
        # received after startTLS was called.  --glyph
        problems = self._sslVerifyProblems
        if problems:
            failReason = Failure(problems[0])
        elif self._justStartedTLS:
            # We just started TLS and haven't received any data.  This means
            # the other connection didn't like our cert (although they may not
            # have told us why - later Twisted should make 'reason' into a TLS
            # error.)
            failReason = PeerVerifyError(
                "Peer rejected our certificate for an unknown reason.")
        else:
            failReason = reason
        self.boxReceiver.stopReceivingBoxes(failReason)



    def proto_init(self, string):
        """
        String received in the 'init' state.
        """
        self._currentBox = AmpBox()
        return self.proto_key(string)


    def proto_key(self, string):
        """
        String received in the 'key' state.  If the key is empty, a complete
        box has been received.
        """
        if string:
            self._currentKey = string
            return 'value'
        else:
            self.boxReceiver.ampBoxReceived(self._currentBox)
            self._currentBox = None
            return 'init'


    def proto_value(self, string):
        """
        String received in the 'value' state.
        """
        self._currentBox[self._currentKey] = string
        self._currentKey = None
        return 'key'


    def _lockForSwitch(self):
        """
        Lock this binary protocol so that no further boxes may be sent.  This
        is used when sending a request to switch underlying protocols.  You
        probably want to subclass ProtocolSwitchCommand rather than calling
        this directly.
        """
        self._locked = True


    def _unlockFromSwitch(self):
        """
        Unlock this locked binary protocol so that further boxes may be sent
        again.  This is used after an attempt to switch protocols has failed
        for some reason.
        """
        if self.innerProtocol is not None:
            raise ProtocolSwitched("Protocol already switched.  Cannot unlock.")
        self._locked = False


    def _prepareTLS(self, certificate, verifyAuthorities):
        """
        Used by StartTLSCommand to put us into the state where we don't
        actually send things that get sent, instead we buffer them.  see
        L{_sendBox}.
        """
        self._startingTLSBuffer = []
        if self.hostCertificate is not None:
            raise OnlyOneTLS(
                "Previously authenticated connection between %s and %s "
                "is trying to re-establish as %s" % (
                    self.hostCertificate,
                    self.peerCertificate,
                    (certificate, verifyAuthorities)))


    def _startTLS(self, certificate, verifyAuthorities):
        """
        Used by TLSBox to initiate the SSL handshake.

        @param certificate: a L{twisted.internet.ssl.PrivateCertificate} for
        use locally.

        @param verifyAuthorities: L{twisted.internet.ssl.Certificate} instances
        representing certificate authorities which will verify our peer.
        """
        self.hostCertificate = certificate
        self._justStartedTLS = True
        if verifyAuthorities is None:
            verifyAuthorities = ()
        self.transport.startTLS(certificate.options(*verifyAuthorities))
        # Remember that mutable list that we were just talking about?  Here
        # it is.  sslverify.py takes care of populating this list as
        # necessary. --glyph
        self._sslVerifyProblems = problemsFromTransport(self.transport)
        stlsb = self._startingTLSBuffer
        if stlsb is not None:
            self._startingTLSBuffer = None
            for box in stlsb:
                self.sendBox(box)


    def _getPeerCertificate(self):
        if self.noPeerCertificate:
            return None
        return Certificate.peerFromTransport(self.transport)
    peerCertificate = property(_getPeerCertificate)


    def unhandledError(self, failure):
        """
        The buck stops here.  This error was completely unhandled, time to
        terminate the connection.
        """
        log.msg("Amp server or network failure "
                "unhandled by client application:")
        log.err(failure)
        log.msg(
            "Dropping connection!  "
            "To avoid, add errbacks to ALL remote commands!")
        if self.transport is not None:
            self.transport.loseConnection()


    def _defaultStartTLSResponder(self):
        """
        The default TLS responder doesn't specify any certificate or anything.

        From a security perspective, it's little better than a plain-text
        connection - but it is still a *bit* better, so it's included for
        convenience.

        You probably want to override this by providing your own StartTLS.responder.
        """
        return {}
    StartTLS.responder(_defaultStartTLSResponder)



class AMP(BinaryBoxProtocol, BoxDispatcher,
          CommandLocator, SimpleStringLocator):
    """
    This protocol is an AMP connection.  See the module docstring for protocol
    details.
    """

    _ampInitialized = False

    def __init__(self, boxReceiver=None, locator=None):
        # For backwards compatibility.  When AMP did not separate parsing logic
        # (L{BinaryBoxProtocol}), request-response logic (L{BoxDispatcher}) and
        # command routing (L{CommandLocator}), it did not have a constructor.
        # Now it does, so old subclasses might have defined their own that did
        # not upcall.  If this flag isn't set, we'll call the constructor in
        # makeConnection before anything actually happens.
        self._ampInitialized = True
        if boxReceiver is None:
            boxReceiver = self
        if locator is None:
            locator = self
        boxSender = self
        BoxDispatcher.__init__(self, locator)
        BinaryBoxProtocol.__init__(self, boxReceiver)


    def locateResponder(self, name):
        """
        Unify the implementations of L{CommandLocator} and
        L{SimpleStringLocator} to perform both kinds of dispatch, preferring
        L{CommandLocator}.
        """
        firstResponder = CommandLocator.locateResponder(self, name)
        if firstResponder is not None:
            return firstResponder
        secondResponder = SimpleStringLocator.locateResponder(self, name)
        return secondResponder


    def __repr__(self):
        """
        A verbose string representation which gives us information about this
        AMP connection.
        """
        return '<%s %s at 0x%x>' % (
            self.__class__.__name__,
            self.innerProtocol, id(self))


    def makeConnection(self, transport):
        """
        Emit a helpful log message when the connection is made.
        """
        if not self._ampInitialized:
            # See comment in the constructor re: backward compatibility.  I
            # should probably emit a deprecation warning here.
            AMP.__init__(self)
        # Save these so we can emit a similar log message in L{connectionLost}.
        self._transportPeer = transport.getPeer()
        self._transportHost = transport.getHost()
        log.msg("%s connection established (HOST:%s PEER:%s)" % (
                self.__class__.__name__,
                self._transportHost,
                self._transportPeer))
        BinaryBoxProtocol.makeConnection(self, transport)


    def connectionLost(self, reason):
        """
        Emit a helpful log message when the connection is lost.
        """
        log.msg("%s connection lost (HOST:%s PEER:%s)" %
                (self.__class__.__name__,
                 self._transportHost,
                 self._transportPeer))
        BinaryBoxProtocol.connectionLost(self, reason)
        self.transport = None



class _ParserHelper:
    """
    A box receiver which records all boxes received.
    """
    def __init__(self):
        self.boxes = []


    def getPeer(self):
        return 'string'


    def getHost(self):
        return 'string'

    disconnecting = False


    def startReceivingBoxes(self, sender):
        """
        No initialization is required.
        """


    def ampBoxReceived(self, box):
        self.boxes.append(box)


    # Synchronous helpers
    def parse(cls, fileObj):
        """
        Parse some amp data stored in a file.

        @param fileObj: a file-like object.

        @return: a list of AmpBoxes encoded in the given file.
        """
        parserHelper = cls()
        bbp = BinaryBoxProtocol(boxReceiver=parserHelper)
        bbp.makeConnection(parserHelper)
        bbp.dataReceived(fileObj.read())
        return parserHelper.boxes
    parse = classmethod(parse)


    def parseString(cls, data):
        """
        Parse some amp data stored in a string.

        @param data: a str holding some amp-encoded data.

        @return: a list of AmpBoxes encoded in the given string.
        """
        return cls.parse(StringIO(data))
    parseString = classmethod(parseString)



parse = _ParserHelper.parse
parseString = _ParserHelper.parseString

def _stringsToObjects(strings, arglist, proto):
    """
    Convert an AmpBox to a dictionary of python objects, converting through a
    given arglist.

    @param strings: an AmpBox (or dict of strings)

    @param arglist: a list of 2-tuples of strings and Argument objects, as
    described in L{Command.arguments}.

    @param proto: an L{AMP} instance.

    @return: the converted dictionary mapping names to argument objects.
    """
    objects = {}
    myStrings = strings.copy()
    for argname, argparser in arglist:
        argparser.fromBox(argname, myStrings, objects, proto)
    return objects



def _objectsToStrings(objects, arglist, strings, proto):
    """
    Convert a dictionary of python objects to an AmpBox, converting through a
    given arglist.

    @param objects: a dict mapping names to python objects

    @param arglist: a list of 2-tuples of strings and Argument objects, as
    described in L{Command.arguments}.

    @param strings: [OUT PARAMETER] An object providing the L{dict}
    interface which will be populated with serialized data.

    @param proto: an L{AMP} instance.

    @return: The converted dictionary mapping names to encoded argument
    strings (identical to C{strings}).
    """
    myObjects = {}
    for (k, v) in objects.items():
        myObjects[k] = v

    for argname, argparser in arglist:
        argparser.toBox(argname, strings, myObjects, proto)
    return strings



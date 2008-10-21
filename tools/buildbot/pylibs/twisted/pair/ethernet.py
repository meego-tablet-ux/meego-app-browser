# -*- test-case-name: twisted.pair.test.test_ethernet -*-
# Copyright (c) 2001-2004 Twisted Matrix Laboratories.
# See LICENSE for details.

#


"""Support for working directly with ethernet frames"""

import struct


from twisted.internet import protocol
from twisted.pair import raw
from zope.interface import implements, Interface


class IEthernetProtocol(Interface):
    """An interface for protocols that handle Ethernet frames"""
    def addProto():
        """Add an IRawPacketProtocol protocol"""

    def datagramReceived():
        """An Ethernet frame has been received"""

class EthernetHeader:
    def __init__(self, data):

        (self.dest, self.source, self.proto) \
                    = struct.unpack("!6s6sH", data[:6+6+2])

class EthernetProtocol(protocol.AbstractDatagramProtocol):

    implements(IEthernetProtocol)
    
    def __init__(self):
        self.etherProtos = {}

    def addProto(self, num, proto):
        proto = raw.IRawPacketProtocol(proto)
        if num < 0:
            raise TypeError, 'Added protocol must be positive or zero'
        if num >= 2**16:
            raise TypeError, 'Added protocol must fit in 16 bits'
        if num not in self.etherProtos:
            self.etherProtos[num] = []
        self.etherProtos[num].append(proto)

    def datagramReceived(self, data, partial=0):
        header = EthernetHeader(data[:14])
        for proto in self.etherProtos.get(header.proto, ()):
            proto.datagramReceived(data=data[14:],
                                   partial=partial,
                                   dest=header.dest,
                                   source=header.source,
                                   protocol=header.proto)

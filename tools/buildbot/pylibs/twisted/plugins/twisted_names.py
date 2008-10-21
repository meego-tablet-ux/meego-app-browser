# Copyright (c) 2001-2008 Twisted Matrix Laboratories.
# See LICENSE for details.

from twisted.application.service import ServiceMaker

TwistedNames = ServiceMaker(
    "Twisted DNS Server",
    "twisted.names.tap",
    "A domain name server.",
    "dns")

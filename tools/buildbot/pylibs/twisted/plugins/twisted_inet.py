# Copyright (c) 2001-2008 Twisted Matrix Laboratories.
# See LICENSE for details.

from twisted.application.service import ServiceMaker

TwistedINETD = ServiceMaker(
    "Twisted INETD Server",
    "twisted.runner.inetdtap",
    "An inetd(8) replacement.",
    "inetd")

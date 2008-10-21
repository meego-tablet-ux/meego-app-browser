# Copyright (c) 2001-2008 Twisted Matrix Laboratories.
# See LICENSE for details.

from twisted.application.service import ServiceMaker

TwistedFTP = ServiceMaker(
    "Twisted FTP",
    "twisted.tap.ftp",
    "An FTP server.",
    "ftp")

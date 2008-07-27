
# Copyright (c) 2001-2004 Twisted Matrix Laboratories.
# See LICENSE for details.


from twisted.python import components
from zope import interface

def foo():
    return 2

class X:
    def __init__(self, x):
        self.x = x

    def do(self):
        #print 'X',self.x,'doing!'
        pass


class XComponent(components.Componentized):
    pass

class IX(components.Interface):
    pass

class XA(components.Adapter):
    interface.implements(IX)

    def method(self):
        # Kick start :(
        pass

components.registerAdapter(XA, X, IX)

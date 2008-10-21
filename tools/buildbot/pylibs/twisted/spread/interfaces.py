"""
Twisted Spread Interfaces.

This module is unused so far. It's also undecided whether this module
will remain monolithic.
"""

from zope.interface import Interface

class IJellyable(Interface):
    def jellyFor(jellier):
        """
        Jelly myself for jellier.
        """

class IUnjellyable(Interface):
    def unjellyFor(jellier, jellyList):
        """
        Unjelly myself for the jellier.

        @param jellier: A stateful object which exists for the lifetime of a
        single call to L{unjelly}.

        @param jellyList: The C{list} which represents the jellied state of the
        object to be unjellied.

        @return: The object which results from unjellying.
        """

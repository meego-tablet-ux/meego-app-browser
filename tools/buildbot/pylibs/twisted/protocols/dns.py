from twisted.python import util

util.moduleMovedForSplit('twisted.protocols.dns', 'twisted.names.dns',
                         'DNS protocol support', 'Names',
                         'http://twistedmatrix.com/projects/names',
                         globals())

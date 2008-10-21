# Copyright (c) 2001-2008 Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Tests for L{twisted.python.zipstream}
"""
import sys
import random
import md5
import zipfile

from twisted.python import zipstream, filepath
from twisted.trial import unittest

class FileEntryMixin:
    """
    File entry classes should behave as file-like objects
    """
    def getFileEntry(self, contents):
        """
        Return an appropriate zip file entry
        """
        filename = self.mktemp()
        z = zipfile.ZipFile(filename, 'w', self.compression)
        z.writestr('content', contents)
        z.close()
        z = zipstream.ChunkingZipFile(filename, 'r')
        return z.readfile('content')


    def test_isatty(self):
        """
        zip files should not be ttys, so isatty() should be false
        """
        self.assertEquals(self.getFileEntry('').isatty(), False)


    def test_closed(self):
        """
        The C{closed} attribute should reflect whether C{close()} has been
        called.
        """
        fileEntry = self.getFileEntry('')
        self.assertEquals(fileEntry.closed, False)
        fileEntry.close()
        self.assertEquals(fileEntry.closed, True)


    def test_readline(self):
        """
        C{readline()} should mirror L{file.readline} and return up to a single
        deliminter.
        """
        fileEntry = self.getFileEntry('hoho\nho')
        self.assertEquals(fileEntry.readline(), 'hoho\n')
        self.assertEquals(fileEntry.readline(), 'ho')
        self.assertEquals(fileEntry.readline(), '')


    def test_next(self):
        """
        Zip file entries should implement the iterator protocol as files do.
        """
        fileEntry = self.getFileEntry('ho\nhoho')
        self.assertEquals(fileEntry.next(), 'ho\n')
        self.assertEquals(fileEntry.next(), 'hoho')
        self.assertRaises(StopIteration, fileEntry.next)


    def test_readlines(self):
        """
        C{readlines()} should return a list of all the lines.
        """
        fileEntry = self.getFileEntry('ho\nho\nho')
        self.assertEquals(fileEntry.readlines(), ['ho\n', 'ho\n', 'ho'])


    def test_iteration(self):
        """
        C{__iter__()} and C{xreadlines()} should return C{self}.
        """
        fileEntry = self.getFileEntry('')
        self.assertIdentical(iter(fileEntry), fileEntry)
        self.assertIdentical(fileEntry.xreadlines(), fileEntry)


    def test_readWhole(self):
        """
        C{.read()} should read the entire file.
        """
        contents = "Hello, world!"
        entry = self.getFileEntry(contents)
        self.assertEquals(entry.read(), contents)


    def test_readPartial(self):
        """
        C{.read(num)} should read num bytes from the file.
        """
        contents = "0123456789"
        entry = self.getFileEntry(contents)
        one = entry.read(4)
        two = entry.read(200)
        self.assertEquals(one, "0123")
        self.assertEquals(two, "456789")


    def test_tell(self):
        """
        C{.tell()} should return the number of bytes that have been read so
        far.
        """
        contents = "x" * 100
        entry = self.getFileEntry(contents)
        entry.read(2)
        self.assertEquals(entry.tell(), 2)
        entry.read(4)
        self.assertEquals(entry.tell(), 6)



class DeflatedZipFileEntryTest(FileEntryMixin, unittest.TestCase):
    """
    DeflatedZipFileEntry should be file-like
    """
    compression = zipfile.ZIP_DEFLATED



class ZipFileEntryTest(FileEntryMixin, unittest.TestCase):
   """
   ZipFileEntry should be file-like
   """
   compression = zipfile.ZIP_STORED



class ZipstreamTest(unittest.TestCase):
    """
    Tests for twisted.python.zipstream
    """
    def setUp(self):
        """
        Creates junk data that can be compressed and a test directory for any
        files that will be created
        """
        self.testdir = filepath.FilePath(self.mktemp())
        self.testdir.makedirs()
        self.unzipdir = self.testdir.child('unzipped')
        self.unzipdir.makedirs()


    def makeZipFile(self, contents, directory=''):
        """
        Makes a zip file archive containing len(contents) files.  Contents
        should be a list of strings, each string being the content of one file.
        """
        zpfilename = self.testdir.child('zipfile.zip').path
        zpfile = zipfile.ZipFile(zpfilename, 'w')
        for i, content in enumerate(contents):
            filename = str(i)
            if directory:
                filename = directory + "/" + filename
            zpfile.writestr(filename, content)
        zpfile.close()
        return zpfilename


    def test_countEntries(self):
        """
        Make sure the deprecated L{countZipFileEntries} returns the correct
        number of entries for a zip file.
        """
        name = self.makeZipFile(["one", "two", "three", "four", "five"])
        result = self.assertWarns(DeprecationWarning,
                                  "countZipFileEntries is deprecated.",
                                  __file__, lambda :
                                      zipstream.countZipFileEntries(name))
        self.assertEquals(result, 5)


    def test_invalidMode(self):
        """
        A ChunkingZipFile opened in write-mode should not allow .readfile(),
        and raise a RuntimeError instead.
        """
        czf = zipstream.ChunkingZipFile(self.mktemp(), "w")
        self.assertRaises(RuntimeError, czf.readfile, "something")


    def test_closedArchive(self):
        """
        A closed ChunkingZipFile should raise a L{RuntimeError} when
        .readfile() is invoked.
        """
        czf = zipstream.ChunkingZipFile(self.makeZipFile(["something"]), "r")
        czf.close()
        self.assertRaises(RuntimeError, czf.readfile, "something")


    def test_invalidHeader(self):
        """
        A zipfile entry with the wrong magic number should raise BadZipfile for
        readfile(), but that should not affect other files in the archive.
        """
        fn = self.makeZipFile(["test contents",
                               "more contents"])
        zf = zipfile.ZipFile(fn, "r")
        zeroOffset = zf.getinfo("0").header_offset
        zf.close()
        # Zero out just the one header.
        scribble = file(fn, "r+b")
        scribble.seek(zeroOffset, 0)
        scribble.write(chr(0) * 4)
        scribble.close()
        czf = zipstream.ChunkingZipFile(fn)
        self.assertRaises(zipfile.BadZipfile, czf.readfile, "0")
        self.assertEquals(czf.readfile("1").read(), "more contents")


    def test_filenameMismatch(self):
        """
        A zipfile entry with a different filename than is found in the central
        directory should raise BadZipfile.
        """
        fn = self.makeZipFile(["test contents",
                               "more contents"])
        zf = zipfile.ZipFile(fn, "r")
        info = zf.getinfo("0")
        info.filename = "not zero"
        zf.close()
        scribble = file(fn, "r+b")
        scribble.seek(info.header_offset, 0)
        scribble.write(info.FileHeader())
        scribble.close()

        czf = zipstream.ChunkingZipFile(fn)
        self.assertRaises(zipfile.BadZipfile, czf.readfile, "0")
        self.assertEquals(czf.readfile("1").read(), "more contents")


    if sys.version_info < (2, 5):
        # In python 2.4 and earlier, consistency between the directory and the
        # file header are verified at archive-opening time.  In python 2.5
        # (and, presumably, later) it is readzipfile's responsibility.
        message = "Consistency-checking only necessary in 2.5."
        test_invalidHeader.skip = message
        test_filenameMismatch.skip = message



    def test_unsupportedCompression(self):
        """
        A zipfile which describes an unsupported compression mechanism should
        raise BadZipfile.
        """
        fn = self.mktemp()
        zf = zipfile.ZipFile(fn, "w")
        zi = zipfile.ZipInfo("0")
        zf.writestr(zi, "some data")
        # Mangle its compression type in the central directory; can't do this
        # before the writestr call or zipfile will (correctly) tell us not to
        # pass bad compression types :)
        zi.compress_type = 1234
        zf.close()

        czf = zipstream.ChunkingZipFile(fn)
        self.assertRaises(zipfile.BadZipfile, czf.readfile, "0")


    def test_extraData(self):
        """
        readfile() should skip over 'extra' data present in the zip metadata.
        """
        fn = self.mktemp()
        zf = zipfile.ZipFile(fn, 'w')
        zi = zipfile.ZipInfo("0")
        zi.extra = "hello, extra"
        zf.writestr(zi, "the real data")
        zf.close()
        czf = zipstream.ChunkingZipFile(fn)
        self.assertEquals(czf.readfile("0").read(), "the real data")


    def test_unzipIter(self):
        """
        L{twisted.python.zipstream.unzipIter} should unzip a file for each
        iteration and yield the number of files left to unzip after that
        iteration
        """
        numfiles = 10
        contents = ['This is test file %d!' % i for i in range(numfiles)]
        zpfilename = self.makeZipFile(contents)
        uziter = zipstream.unzipIter(zpfilename, self.unzipdir.path)
        for i in range(numfiles):
            self.assertEquals(len(list(self.unzipdir.children())), i)
            self.assertEquals(uziter.next(), numfiles - i - 1)
        self.assertEquals(len(list(self.unzipdir.children())), numfiles)

        for child in self.unzipdir.children():
            num = int(child.basename())
            self.assertEquals(child.open().read(), contents[num])


    def test_unzip(self):
        """
        L{twisted.python.zipstream.unzip} should extract all files from a zip
        archive
        """
        numfiles = 3
        zpfilename = self.makeZipFile([''] * numfiles)
        zipstream.unzip(zpfilename, self.unzipdir.path)
        self.assertEquals(len(list(self.unzipdir.children())), numfiles)


    def test_overwrite(self):
        """
        L{twisted.python.zipstream.unzip} and
        L{twisted.python.zipstream.unzipIter} shouldn't overwrite files unless
        the 'overwrite' flag is passed
        """
        testfile = self.unzipdir.child('0')
        zpfilename = self.makeZipFile(['OVERWRITTEN'])

        testfile.setContent('NOT OVERWRITTEN')
        zipstream.unzip(zpfilename, self.unzipdir.path)
        self.assertEquals(testfile.open().read(), 'NOT OVERWRITTEN')
        zipstream.unzip(zpfilename, self.unzipdir.path, overwrite=True)
        self.assertEquals(testfile.open().read(), 'OVERWRITTEN')

        testfile.setContent('NOT OVERWRITTEN')
        uziter = zipstream.unzipIter(zpfilename, self.unzipdir.path)
        uziter.next()
        self.assertEquals(testfile.open().read(), 'NOT OVERWRITTEN')
        uziter = zipstream.unzipIter(zpfilename, self.unzipdir.path,
                                     overwrite=True)
        uziter.next()
        self.assertEquals(testfile.open().read(), 'OVERWRITTEN')


    # XXX these tests are kind of gross and old, but I think unzipIterChunky is
    # kind of a gross function anyway.  We should really write an abstract
    # copyTo/moveTo that operates on FilePath and make sure ZipPath can support
    # it, then just deprecate / remove this stuff.
    def _unzipIterChunkyTest(self, compression, chunksize, lower, upper):
        """
        unzipIterChunky should unzip the given number of bytes per iteration.
        """
        junk = ' '.join([str(random.random()) for n in xrange(1000)])
        junkmd5 = md5.new(junk).hexdigest()

        tempdir = filepath.FilePath(self.mktemp())
        tempdir.makedirs()
        zfpath = tempdir.child('bigfile.zip').path
        self._makebigfile(zfpath, compression, junk)
        uziter = zipstream.unzipIterChunky(zfpath, tempdir.path,
                                           chunksize=chunksize)
        r = uziter.next()
        # test that the number of chunks is in the right ballpark;
        # this could theoretically be any number but statistically it
        # should always be in this range
        approx = lower < r < upper
        self.failUnless(approx)
        for r in uziter:
            pass
        self.assertEqual(r, 0)
        newmd5 = md5.new(
            tempdir.child("zipstreamjunk").open().read()).hexdigest()
        self.assertEqual(newmd5, junkmd5)

    def test_unzipIterChunkyStored(self):
        """
        unzipIterChunky should unzip the given number of bytes per iteration on
        a stored archive.
        """
        self._unzipIterChunkyTest(zipfile.ZIP_STORED, 500, 35, 45)


    def test_chunkyDeflated(self):
        """
        unzipIterChunky should unzip the given number of bytes per iteration on
        a deflated archive.
        """
        self._unzipIterChunkyTest(zipfile.ZIP_DEFLATED, 972, 23, 27)


    def _makebigfile(self, filename, compression, junk):
        """
        Create a zip file with the given file name and compression scheme.
        """
        zf = zipfile.ZipFile(filename, 'w', compression)
        for i in range(10):
            fn = 'zipstream%d' % i
            zf.writestr(fn, "")
        zf.writestr('zipstreamjunk', junk)
        zf.close()

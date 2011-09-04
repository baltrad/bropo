'''
Created on Sep 1, 2011

@author: anders
'''
import unittest
import string
import _raveio
import _fmiimage
import _rave

class FmiImageTest(unittest.TestCase):
  PVOL_TESTFILE="fixtures/pvol_seang_20090501T120000Z.h5"

  def setUp(self):
    pass

  def tearDown(self):
    pass

  def test_offset(self):
    a = _fmiimage.new()
    self.assertAlmostEqual(0.0, a.offset, 4)
    a.offset = 2.5
    self.assertAlmostEqual(2.5, a.offset, 4)

  def test_gain(self):
    a = _fmiimage.new()
    self.assertAlmostEqual(1.0, a.gain, 4)
    a.gain = 2.5
    self.assertAlmostEqual(2.5, a.gain, 4)

  def testAttributes(self):
    a = _fmiimage.new()
    names = a.getAttributeNames()
    self.assertEquals(0, len(names))
    a.addAttribute("how/slask", 10)
    self.assertEquals(10, a.getAttribute("how/slask"))
    names = a.getAttributeNames()
    self.assertEquals(1, len(names))
    self.assertTrue("how/slask" in names)

  def testFromRave_scan(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    scan = a.object.getScan(0)
    b = _fmiimage.fromRave(scan)
    self.assertNotEqual(-1, string.find(`type(b)`, "FmiImageCore"))
    self.assertAlmostEqual(scan.getParameter("DBZH").offset, b.offset, 4)
    self.assertAlmostEqual(scan.getParameter("DBZH").gain, b.gain, 4)

  def testFromRave_withQuantity_scan(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRave(a.object.getScan(0), "DBZH")
    self.assertNotEqual(-1, string.find(`type(b)`, "FmiImageCore"))

  def testFromRave_volume(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRave(a.object)
    self.assertNotEqual(-1, string.find(`type(b)`, "FmiImageCore"))

  def testFromRave_withQuantity_volume(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRave(a.object, "DBZH")
    self.assertNotEqual(-1, string.find(`type(b)`, "FmiImageCore"))

  def testFromRaveVolume(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRaveVolume(a.object, 0)
    self.assertNotEqual(-1, string.find(`type(b)`, "FmiImageCore"))

  def testFromRaveVolume_withQuantity(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRaveVolume(a.object, 0, "DBZH")
    self.assertNotEqual(-1, string.find(`type(b)`, "FmiImageCore"))

  def testToPolarScan(self):
    a = _raveio.open(self.PVOL_TESTFILE).object
    b = _fmiimage.fromRave(a.getScan(0), "DBZH")
    c = b.toPolarScan("DBZH")
    self.assertAlmostEqual(a.getScan(0).elangle, c.elangle, 4)
    self.assertTrue(c.hasParameter("DBZH"))

  def testToRaveField(self):
    a = _raveio.open(self.PVOL_TESTFILE).object
    b = _fmiimage.fromRave(a.getScan(0), "DBZH")
    c = b.toRaveField()
    self.assertNotEqual(-1, string.find(`type(c)`, "RaveFieldCore"))
    
if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()
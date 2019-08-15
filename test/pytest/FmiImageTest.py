'''
Created on Sep 1, 2011

@author: anders
'''
import unittest
import string
import _raveio
import _fmiimage
import _rave
import numpy

class FmiImageTest(unittest.TestCase):
  PVOL_TESTFILE="fixtures/pvol_seang_20090501T120000Z.h5"
  SCAN16_TESTFILE="fixtures/sebaa-scan_0_5_20190613.h5"
  
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

  def test_undetect(self):
    a = _fmiimage.new()
    self.assertAlmostEqual(0.0, a.undetect, 4)
    a.undetect = 20.0
    self.assertAlmostEqual(20.0, a.undetect, 4)

  def test_nodata(self):
    a = _fmiimage.new()
    self.assertAlmostEqual(255.0, a.nodata, 4)
    a.nodata = 1.0
    self.assertAlmostEqual(1, a.nodata, 4)

  def testAttributes(self):
    a = _fmiimage.new()
    names = a.getAttributeNames()
    self.assertEqual(0, len(names))
    a.addAttribute("how/slask", 10)
    self.assertEqual(10, a.getAttribute("how/slask"))
    names = a.getAttributeNames()
    self.assertEqual(1, len(names))
    self.assertTrue("how/slask" in names)

  def testSetValue(self):
    a = _fmiimage.new(10,10)
    a.setValue(1,1,1)
    a.setOriginalValue(1,1,2.0)
    self.assertEqual(1, a.getValue(1,1))
    self.assertAlmostEqual(2.0, a.getOriginalValue(1,1), 4)

  def testFromRave_scan(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    scan = a.object.getScan(0)
    b = _fmiimage.fromRave(scan)
    self.assertNotEqual(-1, str(type(b)).find("FmiImageCore"))
    self.assertAlmostEqual(scan.getParameter("DBZH").offset, b.offset, 4)
    self.assertAlmostEqual(scan.getParameter("DBZH").gain, b.gain, 4)
    self.assertAlmostEqual(scan.getParameter("DBZH").nodata, b.nodata, 4)
    self.assertAlmostEqual(scan.getParameter("DBZH").undetect, b.undetect, 4)

  def testFromRave_scan_different_nodataUndetect(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    scan = a.object.getScan(0)
    p = scan.getParameter("DBZH")
    p.offset = 1.0
    p.gain = 2.0
    p.nodata = 3.0
    p.undetect = 4.0
    b = _fmiimage.fromRave(scan)
    self.assertNotEqual(-1, str(type(b)).find("FmiImageCore"))
    self.assertAlmostEqual(1.0, b.offset, 4)
    self.assertAlmostEqual(2.0, b.gain, 4)
    self.assertAlmostEqual(3.0, b.nodata, 4)
    self.assertAlmostEqual(4.0, b.undetect, 4)

  def testFromRave_withQuantity_scan(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRave(a.object.getScan(0), "DBZH")
    self.assertNotEqual(-1, str(type(b)).find("FmiImageCore"))

  def testFromRave_volume(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRave(a.object)
    self.assertNotEqual(-1, str(type(b)).find("FmiImageCore"))

  def testFromRave_withQuantity_volume(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRave(a.object, "DBZH")
    self.assertNotEqual(-1, str(type(b)).find("FmiImageCore"))

  def testFromRaveVolume(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRaveVolume(a.object, 0)
    self.assertNotEqual(-1, str(type(b)).find("FmiImageCore"))

  def testFromRaveVolume_withQuantity(self):
    a = _raveio.open(self.PVOL_TESTFILE)
    b = _fmiimage.fromRaveVolume(a.object, 0, "DBZH")
    self.assertNotEqual(-1, str(type(b)).find("FmiImageCore"))

  def testToPolarScan(self):
    a = _raveio.open(self.PVOL_TESTFILE).object
    b = _fmiimage.fromRave(a.getScan(0), "DBZH")
    c = b.toPolarScan("DBZH")
    self.assertAlmostEqual(a.getScan(0).elangle, c.elangle, 4)
    self.assertTrue(c.hasParameter("DBZH"))

  def testToPolarScan_sametype(self):
    a = _raveio.open(self.SCAN16_TESTFILE).object
    b = _fmiimage.fromRave(a, "DBZH")
    b.setValue(1,1,10)
    b.setOriginalValue(1,1,20)
    c = b.toPolarScan("DBZH")
    self.assertAlmostEqual(a.elangle, c.elangle, 4)
    self.assertTrue(c.hasParameter("DBZH"))
    self.assertEqual(numpy.int16, c.getParameter("DBZH").getData().dtype)
    self.assertAlmostEqual(20.0, c.getParameter("DBZH").getValue(1,1)[1], 4)

  def testToPolarScan_datatype1(self):
    a = _raveio.open(self.SCAN16_TESTFILE).object
    b = _fmiimage.fromRave(a, "DBZH")
    b.setValue(1,1,10)
    b.setOriginalValue(1,1,20)
    c = b.toPolarScan("DBZH", 1)
    self.assertAlmostEqual(a.elangle, c.elangle, 4)
    self.assertTrue(c.hasParameter("DBZH"))
    self.assertEqual(numpy.uint8, c.getParameter("DBZH").getData().dtype)
    self.assertAlmostEqual(10.0, c.getParameter("DBZH").getValue(1,1)[1], 4)

  def testToPolarScan_datatype2(self):
    a = _raveio.open(self.SCAN16_TESTFILE).object
    b = _fmiimage.fromRave(a, "DBZH")
    b.setValue(1,1,10)
    b.setOriginalValue(1,1,20)
    c = b.toPolarScan("DBZH", 2)
    self.assertAlmostEqual(a.elangle, c.elangle, 4)
    self.assertTrue(c.hasParameter("DBZH"))
    self.assertEqual(numpy.uint8, c.getParameter("DBZH").getData().dtype)
    # Expected result is (20.0*0.001- (-32))/0.5
    self.assertAlmostEqual(64.0, c.getParameter("DBZH").getValue(1,1)[1], 2)

  def testToRaveField(self):
    a = _raveio.open(self.PVOL_TESTFILE).object
    b = _fmiimage.fromRave(a.getScan(0), "DBZH")
    c = b.toRaveField()
    self.assertNotEqual(-1, str(type(c)).find("RaveFieldCore"))

  def testToRaveField_sametype(self):
    a = _raveio.open(self.SCAN16_TESTFILE).object
    b = _fmiimage.fromRave(a, "DBZH")
    b.setValue(1,1,10)
    b.setOriginalValue(1,1,20)
    c = b.toRaveField()
    self.assertNotEqual(-1, str(type(c)).find("RaveFieldCore"))
    self.assertEqual(numpy.int16, c.getData().dtype)
    self.assertAlmostEqual(20.0, c.getValue(1,1)[1], 4)

  def testToRaveField_datatype1(self):
    a = _raveio.open(self.SCAN16_TESTFILE).object
    b = _fmiimage.fromRave(a, "DBZH")
    b.setValue(1,1,10)
    b.setOriginalValue(1,1,20)
    c = b.toRaveField(1)
    self.assertNotEqual(-1, str(type(c)).find("RaveFieldCore"))
    self.assertEqual(numpy.uint8, c.getData().dtype)
    self.assertAlmostEqual(10.0, c.getValue(1,1)[1], 4)

  def testToRaveField_datatype2(self):
    a = _raveio.open(self.SCAN16_TESTFILE).object
    b = _fmiimage.fromRave(a, "DBZH")
    b.setValue(1,1,10)
    b.setOriginalValue(1,1,20)
    c = b.toRaveField(2)
    self.assertNotEqual(-1, str(type(c)).find("RaveFieldCore"))
    self.assertEqual(numpy.uint8, c.getData().dtype)
    self.assertAlmostEqual(20.0, c.getValue(1,1)[1], 4)

if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()
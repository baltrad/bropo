'''
Created on Sep 1, 2011

@author: anders
'''
import unittest

import _fmiimage
import _raveio
import _ropogenerator
import os, string, sys
import _rave
import _polarscanparam
import _polarscan
import _ravefield
import ropo_realtime
import numpy

class PyRopoGeneratorTest(unittest.TestCase):
  PVOL_TESTFILE="fixtures/pvol_seang_20090501T120000Z.h5"
  SCAN16_TESTFILE="fixtures/sebaa-scan_0_5_20190613.h5"
  PVOL_RIX_TESTFILE="fixtures/pvol_rix.h5"
  
  TEMPORARY_FILE="ropotest_file1.h5"
  TEMPORARY_FILE2="ropotest_file2.h5"
  TEMPORARY_FILE3="ropotest_file3.h5"
  
  def setUp(self):
    if os.path.isfile(self.TEMPORARY_FILE):
      os.unlink(self.TEMPORARY_FILE)
    if os.path.isfile(self.TEMPORARY_FILE2):
      os.unlink(self.TEMPORARY_FILE2)

  def tearDown(self):
    if os.path.isfile(self.TEMPORARY_FILE):
      os.unlink(self.TEMPORARY_FILE)
    if os.path.isfile(self.TEMPORARY_FILE2):
     os.unlink(self.TEMPORARY_FILE2)  

  def testNew(self):
    a = _ropogenerator.new()
    self.assertNotEqual(-1, str(type(a)).find("RopoGeneratorCore"))
    self.assertTrue(None==a.getImage())

  def testNew_withImage(self):
    image = _fmiimage.fromRave(_raveio.open(self.PVOL_TESTFILE).object.getScan(0), "DBZH")
    a = _ropogenerator.new(image)
    self.assertNotEqual(-1, str(type(a)).find("RopoGeneratorCore"))
    self.assertTrue(image==a.getImage())
  
  def testSetImage(self):
    volume = _raveio.open(self.PVOL_TESTFILE).object
    image = _fmiimage.fromRave(volume.getScan(0), "DBZH")
    image2 = _fmiimage.fromRave(volume.getScan(1), "DBZH")
    a = _ropogenerator.new()
    a.setImage(image)
    self.assertTrue(image==a.getImage())
    a.setImage(image2)
    self.assertTrue(image2==a.getImage())

  def testThreshold(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.threshold(80)

  def testSpeck(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(-20, 5)

  def testSpeck_differentUndetect_restore(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    a.getParameter("DBZH").undetect = 254.0
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    c=b.speck(-20, 5).restore(50).toPolarScan().getParameter("DBZH")
    self.assertAlmostEqual(254.0, c.undetect, 4)

  def testSpeckNormOld(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speckNormOld(-20, 5, 16)

  def testEmitter(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.emitter(-10, 4)

  def testEmitter2(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.emitter2(-10, 4, 2)
    
  def testClutter(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.clutter(-5, 5)

  def testClutter2(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.clutter(-5, 60)

  def testSoftcut(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.softcut(-10, 250, 100)

  def testBiomet(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.biomet(-10, 5, 5000, 1)

  def testShip(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.ship(15, 8)

  def testSun(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.sun(-10, 32, 3)

  def testSun2(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.sun2(-10, 32, 3, 45, 2)

  def testRestore2(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.sun2(-10, 32, 3, 45, 2).restore2(50)

  def testChaining_speckEmitterEmitter2Clutter(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(-20, 5).emitter(-10,4).emitter2(-10,4,2).clutter(-5,5)

  def testClassify(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(-20, 5).emitter(3,6).classify()
    
    self.assertEqual("fi.fmi.ropo.detector.classification", b.classification.getAttribute("how/task"))
    self.assertTrue(b.classification.getAttribute("how/task_args").find("SPECK:") >= 0)
    self.assertTrue(b.classification.getAttribute("how/task_args").find("EMITTER:") >= 0)

    self.assertEqual("fi.fmi.ropo.detector.classification_marker", b.markers.getAttribute("how/task"))
    self.assertTrue(b.markers.getAttribute("how/task_args").find("SPECK:") >= 0)
    self.assertTrue(b.markers.getAttribute("how/task_args").find("EMITTER:") >= 0)

  def testClassify_reclassification(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(-20, 5).emitter(3,6).classify()
    
    self.assertEqual("fi.fmi.ropo.detector.classification", b.classification.getAttribute("how/task"))
    self.assertTrue(b.classification.getAttribute("how/task_args").find("SPECK:") >= 0)
    self.assertTrue(b.classification.getAttribute("how/task_args").find("EMITTER:") >= 0)

    self.assertEqual("fi.fmi.ropo.detector.classification_marker", b.markers.getAttribute("how/task"))
    self.assertTrue(b.markers.getAttribute("how/task_args").find("SPECK:") >= 0)
    self.assertTrue(b.markers.getAttribute("how/task_args").find("EMITTER:") >= 0)

    b.clutter(-5, 5).classify()
    self.assertEqual("fi.fmi.ropo.detector.classification", b.classification.getAttribute("how/task"))
    self.assertTrue(b.classification.getAttribute("how/task_args").find("SPECK:") >= 0)
    self.assertTrue(b.classification.getAttribute("how/task_args").find("EMITTER:") >= 0)
    self.assertTrue(b.classification.getAttribute("how/task_args").find("CLUTTER:") >= 0)

  def testDeclassify(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(-20, 5).emitter(3,6).classify().declassify()
    
    self.assertEqual(0, b.getProbabilityFieldCount())
    self.assertEqual("fi.fmi.ropo.detector.classification", b.classification.getAttribute("how/task"))
    self.assertTrue(b.classification.getAttribute("how/task_args").find("SPECK:") == -1)
    self.assertTrue(b.classification.getAttribute("how/task_args").find("EMITTER:") == -1)

    self.assertEqual("fi.fmi.ropo.detector.classification_marker", b.markers.getAttribute("how/task"))
    self.assertTrue(b.markers.getAttribute("how/task_args").find("SPECK:") == -1)
    self.assertTrue(b.markers.getAttribute("how/task_args").find("EMITTER:") == -1)

  def testGenerator_generateClassification(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))

    b.speck(-20, 5)
    b.emitter(3, 6)
    b.classify()
    c = b.classification

    output = _raveio.new()
    output.object = c.toPolarScan("DBZH")
    output.filename = self.TEMPORARY_FILE
    output.save()
 
  def testGeneratorSequence_generateClassification(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))

    c = b.speck(-20, 5).emitter(3, 6).classify().classification

    output = _raveio.new()
    output.object = c.toPolarScan("DBZH")
    output.filename = self.TEMPORARY_FILE
    output.save()

  def testGeneratorSequence_fullExample(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))

    b.speck(-20, 5).speckNormOld(-20,5,16).emitter(-20, 4).emitter2(-10,4,2)

    classification = b.classify().classification.toRaveField()
    restored = b.restore(50).toRaveField()
    
    a.addQualityField(classification)
    a.addQualityField(restored)
    
    output = _raveio.new()
    output.object = a
    output.filename = self.TEMPORARY_FILE

    self.exportFile(output);

  def testRestoreSelf(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    oldimg = b.getImage()
    b.speck(-20, 5).restoreSelf(50)
    result = b.getImage()
    self.assertTrue(result != oldimg)
    self.assertEqual("fi.fmi.ropo.restore", result.getAttribute("how/task"))
    self.assertTrue(result.getAttribute("how/task_args").find("SPECK:") >= 0)

  def testGetProbabilityFieldCount(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))

    self.assertEqual(0, b.getProbabilityFieldCount())
    b.speck(-20, 5).emitter(-20, 4)
    self.assertEqual(2, b.getProbabilityFieldCount())
    c = b.getProbabilityField(0)
    d = b.getProbabilityField(1)
    self.assertTrue(c.getAttribute("how/task_args").find("SPECK:") >= 0)
    self.assertTrue(c.getAttribute("how/task_args").find("EMITTER:") == -1)
    self.assertTrue(d.getAttribute("how/task_args").find("EMITTER:") >= 0)
    self.assertTrue(d.getAttribute("how/task_args").find("SPECK:") == -1)

  def testPadding(self):
      import numpy

      scan = _polarscan.new()
      dbzh = _polarscanparam.new()
      dbzh.quantity = "DBZH"
      l = []
      for i in range(360):
          l.append([i,i,i])

      data = numpy.array(l).astype(numpy.uint16)
      dbzh.setData(data)
      scan.addParameter(dbzh)
      dbzh.quantity = "TH"
      scan.addParameter(dbzh)

      options = ropo_realtime.options()
      options.emitter2 = (-10, 3, 3)  # default

      newscan, gates = ropo_realtime.PadNrays(scan, options)
      newdata = newscan.getParameter("DBZH").getData()

      # Test data wrapping       
      self.assertEqual(newdata[:4].tolist(), data[356:,].tolist())
      self.assertEqual(newdata[364:,].tolist(), data[:4,].tolist())

      classification = _ravefield.new()
      classification.setData(newdata)    # bogus probability of anomaly array
      unwrapped, classification = ropo_realtime.UnpadNrays(newscan, classification, gates)

      # Test data unwrapping
      self.assertEqual(unwrapped.getParameter("DBZH").getData().tolist(), 
                        data.tolist())

  def testSpeck_16bit(self):
    a = _raveio.open(self.SCAN16_TESTFILE).object
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    c = b.speck(-30,12).restore(108).toRaveField().getData()
    self.assertEqual(numpy.int16, c.dtype)

  def testChainCompare_8bit_and_16bit_Restore(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    result8bit = b.speckNormOld(-20,24,8).emitter2(-30,3,3).softcut(5,170,180).ship(20,8).speck(-30,12).restore(108).toPolarScan().getParameter("DBZH").getData()
    
    # Adjust the 8 bit data to be 16 bit instead
    p = a.getParameter("DBZH")
    d = p.getData().astype(numpy.int16)
    p.setData(d)
    p.undetect = 0.0
    p.nodata = 255.0
    a.addParameter(p)
    
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    result16bit = b.speckNormOld(-20,24,8).emitter2(-30,3,3).softcut(5,170,180).ship(20,8).speck(-30,12).restore(108).toPolarScan().getParameter("DBZH").getData()
    
    self.assertEqual(numpy.int16, result16bit.dtype) # Result should be same type as when created
    
    result16bit=result16bit.astype(numpy.uint8)
    
    self.assertTrue(numpy.array_equal(result8bit.astype(numpy.int16),result16bit.astype(numpy.int16)))

  def testChainCompare_8bit_and_16bit_Restore2(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    result8bit = b.speckNormOld(-20,24,8).emitter2(-30,3,3).softcut(5,170,180).ship(20,8).speck(-30,12).restore2(108).toPolarScan().getParameter("DBZH").getData()
    
    # Adjust the 8 bit data to be 16 bit instead
    p = a.getParameter("DBZH")
    d = p.getData().astype(numpy.int16)
    p.setData(d)
    p.undetect = 0.0
    p.nodata = 255.0
    a.addParameter(p)
    
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    result16bit = b.speckNormOld(-20,24,8).emitter2(-30,3,3).softcut(5,170,180).ship(20,8).speck(-30,12).restore2(108).toPolarScan().getParameter("DBZH").getData()
    
    self.assertEqual(numpy.int16, result16bit.dtype) # Result should be same type as when created
    
    #result16bit=result16bit.astype(numpy.uint8)
    
    # Averaging might be different since the procedure is different for determining mean. atol means that avg different should be max 3
    # Verifications is done as absolute(a - b) <= (atol + rtol * absolute(b)) which means that this verifies that
    # abs(8bit - 16bit) is < 3
    self.assertTrue(numpy.allclose(result8bit.astype(numpy.int16), result16bit, atol=3.0, rtol=0.0)) # If not having values in short range, there might be wrap around. E.g. -1 => 255 which is out of range 3

  def testChainCompare_8bit_and_32bit_Restore(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    result8bit = b.speckNormOld(-20,24,8).emitter2(-30,3,3).softcut(5,170,180).ship(20,8).speck(-30,12).restore(108).toPolarScan().getParameter("DBZH").getData()
    
    # Adjust the 8 bit data to be 16 bit instead
    p = a.getParameter("DBZH")
    d = p.getData().astype(numpy.int32)
    p.setData(d)
    p.undetect = 0.0
    p.nodata = 255.0
    a.addParameter(p)
    
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    result32bit = b.speckNormOld(-20,24,8).emitter2(-30,3,3).softcut(5,170,180).ship(20,8).speck(-30,12).restore(108).toPolarScan().getParameter("DBZH").getData()
    
    self.assertEqual(numpy.int32, result32bit.dtype) # Result should be same type as when created
    
    result32bit=result32bit.astype(numpy.uint8)
    
    self.assertTrue(numpy.array_equal(result8bit.astype(numpy.int32),result32bit.astype(numpy.int32)))
  
  def testCompare_8bit_and_16bit_Classification(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    result8bit = b.speckNormOld(-20,24,8).classify().classification.toRaveField().getData()
    
    self.assertEqual(numpy.uint8, result8bit.dtype)
    
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    # Adjust the 8 bit data to be 16 bit instead
    p = a.getParameter("DBZH")
    p.setData(p.getData().astype(numpy.int16))
    c = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))

    result16bit = c.speckNormOld(-20,24,8).classify().classification.toRaveField().getData()
    self.assertEqual(numpy.uint8, result16bit.dtype)

    self.assertTrue(numpy.array_equal(result8bit,result16bit))
  
  # Simple way to ensure that a file is exported properly
  #
  def exportFile(self, object):
    if os.path.isfile(object.filename):
      os.unlink(object.filename)
    object.save()

    
if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()
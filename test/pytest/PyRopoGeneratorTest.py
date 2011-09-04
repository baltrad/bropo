'''
Created on Sep 1, 2011

@author: anders
'''
import unittest

import _fmiimage
import _raveio
import _ropogenerator
import os, string
import _rave
import _polarscanparam

class PyRopoGeneratorTest(unittest.TestCase):
  PVOL_TESTFILE="fixtures/pvol_seang_20090501T120000Z.h5"
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
    self.assertNotEqual(-1, string.find(`type(a)`, "RopoGeneratorCore"))
    self.assertTrue(None==a.getImage())

  def testNew_withImage(self):
    image = _fmiimage.fromRave(_raveio.open(self.PVOL_TESTFILE).object.getScan(0), "DBZH")
    a = _ropogenerator.new(image)
    self.assertNotEqual(-1, string.find(`type(a)`, "RopoGeneratorCore"))
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
  
  def testSpeck(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(-20, 5);
    
    output = _raveio.new()
    output.object = b.getImage().toPolarScan("DBZH")
    output.filename = self.TEMPORARY_FILE
    output.save()

  def testSpeckNormOld(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speckNormOld(-20, 5, 16);
    
    output = _raveio.new()
    output.object = b.getImage().toPolarScan("DBZH")
    output.filename = self.TEMPORARY_FILE
    output.save()

  def testEmitter(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(3, 6);
    
    output = _raveio.new()
    output.object = b.getImage().toPolarScan("DBZH")
    output.filename = self.TEMPORARY_FILE
    output.save()

  def testEmitter2(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(3, 6);
    
    output = _raveio.new()
    output.object = b.getImage().toPolarScan("DBZH")
    output.filename = self.TEMPORARY_FILE
    output.save()

  def testSpeckAndEmitter(self):
    a = _raveio.open(self.PVOL_RIX_TESTFILE).object.getScan(0)
    b = _ropogenerator.new(_fmiimage.fromRave(a, "DBZH"))
    b.speck(-20, 5).emitter(3,6)
    
    output = _raveio.new()
    output.object = b.getImage().toPolarScan("DBZH")
    output.filename = self.TEMPORARY_FILE
    output.save()

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
    output.filename = self.TEMPORARY_FILE3

    self.exportFile(output);

  # Simple way to ensure that a file is exported properly
  #
  def exportFile(self, object):
    if os.path.isfile(object.filename):
      os.unlink(object.filename)
    object.save()
    
if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()
'''
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of beamb.

beamb is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

beamb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with beamb.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/

BeamBlockage tests

@file
@author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
@date 2012-01-03
'''
import unittest

import _raveio
import _beamblockage
import ropo_quality_plugin
import rave_quality_plugin
import os, string
import _rave
import numpy

class ropo_quality_plugin_test(unittest.TestCase):
  SCAN_FIXTURE = "fixtures/scan_sevil_20100702T113200Z.h5"
  VOLUME_FIXTURE = "fixtures/pvol_rix.h5"
  
  def setUp(self):
    self.classUnderTest = ropo_quality_plugin.ropo_quality_plugin()
  
  def tearDown(self):
    self.classUnderTest = None

  def test_getQualityFields(self):
    result = self.classUnderTest.getQualityFields()
    self.assertEquals(1, len(result))
    self.assertEquals("fi.fmi.ropo.detector.classification", result[0])
  
  def test_process_scan(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object

    result = self.classUnderTest.process(scan)
    
    self.assertTrue(result == scan) # Return original scan with added field
    self.assertTrue(result.findQualityFieldByHowTask("fi.fmi.ropo.detector.classification") != None)

  def test_process_scan_reprocess_true(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object

    field1 = self.classUnderTest.process(scan, reprocess_quality_flag=True).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification")
    field2 = self.classUnderTest.process(scan, reprocess_quality_flag=True).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification")
    
    self.assertTrue(field1 != None)
    self.assertTrue(field2 != None)
    self.assertTrue(field1 != field2)

  def test_process_scan_reprocess_false(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object

    field1 = self.classUnderTest.process(scan, reprocess_quality_flag=False).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification")
    field2 = self.classUnderTest.process(scan, reprocess_quality_flag=False).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification")
    
    self.assertTrue(field1 != None)
    self.assertTrue(field2 != None)
    self.assertTrue(field1 == field2)

  def test_process_volume(self):
    volume = _raveio.open(self.VOLUME_FIXTURE).object

    result = self.classUnderTest.process(volume)
    
    for i in range(result.getNumberOfScans()):
      scan = result.getScan(i)
      self.assertTrue(scan.getQualityFieldByHowTask("fi.fmi.ropo.detector.classification") != None)

  def test_process_volume_reprocess_true(self):
    volume = _raveio.open(self.VOLUME_FIXTURE).object

    result = self.classUnderTest.process(volume)
    
    fields = []
    for i in range(result.getNumberOfScans()):
      fields.append(result.getScan(i).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification"))

    result = self.classUnderTest.process(volume, reprocess_quality_flag=True)
    
    fields2 = []
    for i in range(result.getNumberOfScans()):
      fields2.append(result.getScan(i).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification"))
    
    self.assertTrue(len(fields) != 0)
    self.assertEquals(len(fields), len(fields2))
    for i in range(len(fields)):
      self.assertTrue(fields[i] != fields2[i])

  def test_process_volume_reprocess_false(self):
    volume = _raveio.open(self.VOLUME_FIXTURE).object

    result = self.classUnderTest.process(volume, reprocess_quality_flag=False)
    
    fields = []
    for i in range(result.getNumberOfScans()):
      fields.append(result.getScan(i).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification"))

    result = self.classUnderTest.process(volume, reprocess_quality_flag=False)
    
    fields2 = []
    for i in range(result.getNumberOfScans()):
      fields2.append(result.getScan(i).getQualityFieldByHowTask("fi.fmi.ropo.detector.classification"))
    
    self.assertTrue(len(fields) != 0)
    self.assertEquals(len(fields), len(fields2))
    for i in range(len(fields)):
      self.assertTrue(fields[i] == fields2[i])


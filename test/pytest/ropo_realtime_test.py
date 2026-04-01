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
@date 2026-04-01
'''
import unittest

import _raveio
from attr import field
import ropo_realtime

from rave_quality_plugin import QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY, QUALITY_CONTROL_MODE_ANALYZE

class ropo_realtime_test(unittest.TestCase):
  SCAN_FIXTURE = "fixtures/scan_sevil_20100702T113200Z.h5"
  VOLUME_FIXTURE = "fixtures/pvol_rix.h5"
  
  def setUp(self):
    pass
  
  def tearDown(self):
    pass

  def test_PadNrays_emitter2_1(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.emitter2 = [-10, 2, 3]
    result, gates = ropo_realtime.PadNrays(scan, opts)

    self.assertEqual(4, gates)
    self.assertEqual(scan.nrays + 4 * 2, result.nrays)

  def test_PadNrays_emitter2_2(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.emitter2 = [-10, 2, 4]
    result, gates = ropo_realtime.PadNrays(scan, opts)

    self.assertEqual(5, gates)
    self.assertEqual(scan.nrays + 5 * 2, result.nrays)

  def test_UnpadNrays_emitter2_1(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.emitter2 = [-10, 2, 3]
    result, gates = ropo_realtime.PadNrays(scan, opts)

    classification = result.getParameter("DBZH").toField()

    restored, classification = ropo_realtime.UnpadNrays(result, classification, gates)
    self.assertEqual(scan.nrays, restored.nrays)
    self.assertEqual(scan.nrays, classification.getData().shape[0])

  def test_UnpadNrays_emitter2_2(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.emitter2 = [-10, 2, 4]
    result, gates = ropo_realtime.PadNrays(scan, opts)

    classification = result.getParameter("DBZH").toField()

    restored, classification = ropo_realtime.UnpadNrays(result, classification, gates)
    self.assertEqual(scan.nrays, restored.nrays)
    self.assertEqual(scan.nrays, classification.getData().shape[0])

  def test_PadNrays_padwith_1(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.padwidth = "3"
    result, gates = ropo_realtime.PadNrays(scan, opts)

    self.assertEqual(4, gates)
    self.assertEqual(scan.nrays + 4 * 2, result.nrays)

  def test_PadNrays_padwidth_2(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.padwidth = "4"
    result, gates = ropo_realtime.PadNrays(scan, opts)

    self.assertEqual(5, gates)
    self.assertEqual(scan.nrays + 5 * 2, result.nrays)

  def test_UnpadNrays_padwidth_1(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.padwidth = "3"
    result, gates = ropo_realtime.PadNrays(scan, opts)

    classification = result.getParameter("DBZH").toField()

    restored, classification = ropo_realtime.UnpadNrays(result, classification, gates)
    self.assertEqual(scan.nrays, restored.nrays)
    self.assertEqual(scan.nrays, classification.getData().shape[0])

  def test_UnpadNrays_padwidth_2(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    opts.padwidth = "4"
    result, gates = ropo_realtime.PadNrays(scan, opts)

    classification = result.getParameter("DBZH").toField()

    restored, classification = ropo_realtime.UnpadNrays(result, classification, gates)
    self.assertEqual(scan.nrays, restored.nrays)
    self.assertEqual(scan.nrays, classification.getData().shape[0])

  def test_PadNrays_default(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    result, gates = ropo_realtime.PadNrays(scan, opts)

    self.assertEqual(4, gates)
    self.assertEqual(scan.nrays + 4 * 2, result.nrays)

  def test_UnpadNrays_default(self):
    scan = _raveio.open(self.SCAN_FIXTURE).object
    opts = ropo_realtime.options()
    result, gates = ropo_realtime.PadNrays(scan, opts)

    classification = result.getParameter("DBZH").toField()

    restored, classification = ropo_realtime.UnpadNrays(result, classification, gates)
    self.assertEqual(scan.nrays, restored.nrays)
    self.assertEqual(scan.nrays, classification.getData().shape[0])

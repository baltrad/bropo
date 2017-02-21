#!/usr/bin/env python
'''
Copyright (C) 2011- Swedish Meteorological and Hydrological Institute (SMHI)

This file is part of the bRopo extension to RAVE.

RAVE is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RAVE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with RAVE.  If not, see <http://www.gnu.org/licenses/>.
'''
## FMI's ROPO Anomaly Detection and Removal ...
## ... as a BALTRAD tool. This is the real-time script.

## @file
## @author Daniel Michelson, SMHI
## @date 2011-09-06

from Proj import rd
from rave_defines import UTF8
import _fmiimage
import _polarscan
import _polarvolume
import _raveio
import _ropogenerator
import odim_source
import os
import time
import copy
import xml.etree.ElementTree as ET
from rave_quality_plugin import QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY
from rave_quality_plugin import QUALITY_CONTROL_MODE_ANALYZE

## Contains site-specific argument settings 
CONFIG_FILE = os.path.join(os.path.join(os.path.split(os.path.split(_ropogenerator.__file__)[0])[0],
                                        'config'), 'ropo_options.xml')

# Guideline command-line arguments when creating this functionality
# --parameters=DBZH --threshold=<see below> --restore-fill=True --restore-thresh=108 
# --softcut=5,170,180 --speckNormOld=-20,24,8 --emitter2=-10,3,2 --ship=20,8 --speck=-30,12

## Dictionary containing static reflectivity thresholds, one for each month of the year.
# To be used when initially thresholding reflectivity data.
# Add and use entries as you please.
THRESHOLDS = {"COLD"      : (-6, -4, -2, 0, 2, 4, 6, 4, 2, 0, -2, -4),
              "VERY_COLD" : (-12, -10, -6, -4, 0, 4, 6, 4, -4, -8, -10, -12),
              "TEMPERATE" : (0, 2, 4, 6, 8, 10, 10, 8, 6, 4, 2, 0),
              "FLAT-10"   : (-10, -10, -10, -10, -10, -10, -10, -10, -10, -10, -10, -10),
              "FLAT-24"   : (-24, -24, -24, -24, -24, -24, -24, -24, -24, -24, -24, -24),
              "FLAT-30"   : (-30, -30, -30, -30, -30, -30, -30, -30, -30, -30, -30, -30),
              "NONE"      : None
              }
THRESHOLDS["DEFAULT"] = THRESHOLDS["FLAT-24"]

initialized = 0

ARGS = {}  # Empty dictionary to be filled with site-specific options/arguments

## Initializes the ARGS dictionary by reading content from XML file
def init():
    global initialized
    if initialized: return
    
    C = ET.parse(CONFIG_FILE)
    OPTIONS = C.getroot()
    
    for site in list(OPTIONS):
        opts = options()
        
        for k in site.attrib.keys():
            if   k == "parameters": opts.params = site.attrib[k]
            elif k == "threshold": opts.threshold = site.attrib[k]
            elif k == "highest-elev": opts.elev = float(site.attrib[k])
            elif k == "restore": opts.restore = eval(site.attrib[k])
            elif k == "restore-fill": opts.restore2 = eval(site.attrib[k])
            elif k == "restore-thresh": opts.restore_thresh = eval(site.attrib[k])
            elif k == "softcut": opts.softcut = eval(site.attrib[k])
            elif k == "speckNormOld": opts.speckNormOld = eval(site.attrib[k])
            elif k == "emitter2": opts.emitter2 = eval(site.attrib[k])
            elif k == "ship": opts.ship = eval(site.attrib[k])
            elif k == "speck": opts.speck = eval(site.attrib[k])

        ARGS[site.tag] = opts                
    initialized = 1


## Class used to organize options and argument values to ropo.
# Tries to be similar to "options" created by the \ref optparse module.
# TODO: needs to be modified for each new ropo detector or argument that is added.
# This version uses only those options/arguments that are actually used. 
class options:
    def __init__(self):
        self.params = None
        self.threshold = None
        self.elev = None
        self.restore = None
        self.restore2 = None
        self.restore_thresh = None
        self.softcut = None
        self.speckNormOld = None
        self.emitter2 = None
        self.ship = None
        self.speck = None


## Based on the /what/source attribute, find site-specific options/arguments
# @param inobj input SCAN or PVOL object
# @return options object. If the look-up fails, then default options are returned
def get_options(inobj):
    odim_source.CheckSource(inobj)
    S = odim_source.ODIM_Source(inobj.source)
    try:
        return copy.deepcopy(ARGS[S.nod])
    except KeyError:
        return copy.deepcopy(ARGS["default"])


## Copies the top-level 'what' attributes from one object to another.
# Fixes /what/source in the process, if it doesn't contain the NOD identifier.
# @param ino input PVOL or SCAN object
# @param outo output PVOL or SCAN object
def copy_topwhat(ino, outo):
    outo.beamwidth = ino.beamwidth
    outo.date = ino.date
    outo.time = ino.time
    outo.height = ino.height
    outo.latitude = ino.latitude
    outo.longitude = ino.longitude
    outo.source = ino.source


## TODO: activate parameters list. This first version uses only the default, assumes DBZH.
#  TODO: separation of probability fields.
# @param scan input SCAN object
# @param options variable-length object containing argument names and values
# @returns scan object containing detected and removed anomalies
def process_scan(scan, options, quality_control_mode=QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY):
    newscan, gates = PadNrays(scan, options)

    image = _fmiimage.fromRave(newscan, options.params)        
    param = newscan.getParameter(options.params)
    rg = _ropogenerator.new(image)
    if options.threshold:
        raw_thresh = int((options.threshold - param.offset) / param.gain)
        rg.threshold(raw_thresh)
    if options.speck:
        a, b = options.speck
        rg.speck(a, b)
    if scan.elangle * rd < options.elev:
        if options.speckNormOld:
            a, b, c = options.speckNormOld
            rg.speckNormOld(a, b, c)
        if options.softcut:
            a, b, c = options.softcut
            rg.softcut(a, b, c)
        if options.ship:
            a, b = options.ship
            rg.ship(a, b)
        if options.emitter2:
            a, b, c = options.emitter2
            rg.emitter2(a, b, c)

    classification = rg.classify().classification.toRaveField()
    if options.restore:
        restored = rg.restore(int(options.restore_thresh)).toPolarScan()
    elif options.restore2:
        restored = rg.restore2(int(options.restore_thresh)).toPolarScan()
        
    restored, classification = UnpadNrays(restored, classification, gates)
    dbzh = scan.getParameter("DBZH")
    if quality_control_mode != QUALITY_CONTROL_MODE_ANALYZE:
      dbzh.setData(restored.getParameter("DBZH").getData())
      scan.addParameter(dbzh)
    scan.addOrReplaceQualityField(classification)
    
    return scan


## Loops through a volume and processes scans using \ref process_scan
# @param pvol input PVOL object
# @param options variable-length object containing argument names and values
# @return PVOL object containing detected and removed anomalies
def process_pvol(pvol, options, quality_control_mode=QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY):
    import _polarvolume

    out = _polarvolume.new()
    copy_topwhat(pvol, out)
    
    # Get month and use it to determine dBZ threshold, recalling that
    # options.threshold contains the name of the look-up. This is 
    # over-written with the looked-up threshold itself. 
    month = int(pvol.date[4:6]) - 1
    options.threshold = THRESHOLDS[options.threshold][month]

    for a in pvol.getAttributeNames():  # Copy also 'how' attributes at top level of volume, if any
        out.addAttribute(a, pvol.getAttribute(a))

    for s in range(pvol.getNumberOfScans()):
        scan = pvol.getScan(s)
        scan = process_scan(scan, options, quality_control_mode)
        out.addScan(scan)

    return out


## Generate - does the real work
# @param inobj SCAN or PVOL object
# @param reprocess_quality_flag: Specifies if the quality flag should be reprocessed or not.
# @return SCAN or PVOL object, with anomalies hopefully identified and removed
def generate(inobj, reprocess_quality_flag=True, quality_control_mode=QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY):
    if _polarscan.isPolarScan(inobj) == False and _polarvolume.isPolarVolume(inobj) == False:
      raise IOError, "Input file must be either polar scan or volume."

    if reprocess_quality_flag == False:
      if _polarscan.isPolarScan(inobj) and inobj.findQualityFieldByHowTask("fi.fmi.ropo.detector.classification"):
        return inobj
      elif _polarvolume.isPolarVolume(inobj):
        allprocessed = True
        for i in range(inobj.getNumberOfScans()):
          scan = inobj.getScan(i)
          if not scan.findQualityFieldByHowTask("fi.fmi.ropo.detector.classification"):
            allprocessed = False
            break
        if allprocessed:
          return inobj

    options = get_options(inobj)  # Gets options/arguments for this radar. Fixes /what/source if required.
    if _polarvolume.isPolarVolume(inobj):
      ret = process_pvol(inobj, options, quality_control_mode)
    elif _polarscan.isPolarScan(inobj):
      month = int(inobj.date[4:6]) - 1
      options.threshold = THRESHOLDS[options.threshold][month]
      ret = process_scan(inobj, options, quality_control_mode)
      copy_topwhat(inobj, ret)

    return ret


## Internal function to wrap rays near 360-0 degrees. This addresses a design
# flaw in ropo's original C code that leads to data being removed in this sector.
# NOTE that this fix is only being made available at the Python level.
# @param scan input scan object
# @param \ref options instance containing options and their values.
# @return tuple containing scan object and int gates,
# which is the number of gates the scan should be wrapped with.
def PadNrays(scan, options):
    from numpy import vstack

    width = float(options.emitter2[2])
    gatew = 360.0 / scan.nrays
    gates = (width / gatew) # / 2  # May as well pad with good margin
    if (gates - 1) > 0.0:
        gates = int(gates + 1)
    else:
        gates = int(gates)

    newscan = _polarscan.new()

    dbzh = scan.getParameter("DBZH").clone()
    data = dbzh.getData()
    toprays = data[0:gates, ]
    botrays = data[scan.nrays - gates:, ]
    data = vstack((botrays, data, toprays))
    dbzh.setData(data)
    dbzh.quantity = "DBZH"
    newscan.addParameter(dbzh)
    newscan.elangle = scan.elangle
    return newscan, gates


## Internal function to unwrap a scan from overlapping rays.
# @param scan input scan object
# @param classification input RaveField object containing probability of anomaly
# @param gates int number of gates the scan has been wrapped with.
# @return tuple containing scan object and classification, both unpadded
def UnpadNrays(scan, classification, gates):
    dbzh = scan.getParameter("DBZH")
    data = dbzh.getData()
    data = data[gates:scan.nrays - gates, ]
    dbzh.setData(data)

    qdata = classification.getData()
    qdata = qdata[gates:scan.nrays - gates, ]
    classification.setData(qdata)

    scan.removeParameter("DBZH")
    scan.addParameter(dbzh)
    return scan, classification


## Initialize
init()


if __name__ == "__main__":
    pass

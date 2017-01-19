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
##
# A quality plugin for enabling the bRopo support 

## 
# @file
# @author Anders Henja, SMHI
# @date 2011-11-07

from rave_quality_plugin import rave_quality_plugin
from rave_quality_plugin import QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY
import _polarvolume

class ropo_quality_plugin(rave_quality_plugin):
  ##
  # Default constructor
  def __init__(self):
    super(ropo_quality_plugin, self).__init__()
  
  ##
  # @return a list containing the string se.smhi.detector.poo
  def getQualityFields(self):
    return ["fi.fmi.ropo.detector.classification"]
  
  ##
  # @param obj: A rave object that should be processed.
  # @param reprocess_quality_flag: Specifies if the quality flag should be reprocessed or not. 
  # @param arguments: Not used
  # @return: The modified object if this quality plugin has performed changes 
  # to the object.
  def process(self, obj, reprocess_quality_flag=True, quality_control_mode=QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY, arguments=None):
    try:
      import ropo_realtime
      obj = ropo_realtime.generate(obj, reprocess_quality_flag, quality_control_mode)
    except:
      pass
    return obj

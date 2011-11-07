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
# A composite quality plugin for enabling the bRopo support 

## 
# @file
# @author Anders Henja, SMHI
# @date 2011-11-07

from rave_composite_quality_plugin import rave_composite_quality_plugin
import _polarvolume

class ropo_pgf_composite_quality_plugin(rave_composite_quality_plugin):
  ##
  # Default constructor
  def __init__(self):
    super(ropo_pgf_composite_quality_plugin, self).__init__()
  
  ##
  # @return a list containing the string se.smhi.detector.poo
  def getQualityFields(self):
    return ["fi.fmi.ropo.detector.classification"]
  
  ##
  # @param obj: A rave object that should be processed.
  # @return: The modified object if this quality plugin has performed changes 
  # to the object.
  def process(self, obj):
    try:
      import ropo_realtime
      obj = ropo_realtime.generate(obj)
    except:
      pass
    return obj

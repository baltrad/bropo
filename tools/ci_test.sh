#!/bin/sh
############################################################
# Description: Script that should be executed from a continous
# integration runner. It is nessecary to point out the proper
# paths to RAVE since this test sequence should be run whenever
# RAVE is changed.
#
# Author(s):   Anders Henja
#
# Copyright:   Swedish Meteorological and Hydrological Institute, 2011
#
# History:  2011-08-20 Created by Anders Henja
############################################################
SCRFILE=`python -c "import os;print os.path.abspath(\"$0\")"`
SCRIPTPATH=`dirname "$SCRFILE"`

"$SCRIPTPATH/run_python_script.sh" "${SCRIPTPATH}/../test/pytest/RopoXmlTestSuite.py" "${SCRIPTPATH}/../test/pytest"
exit $?

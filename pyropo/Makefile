###########################################################################
# Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,
#
# This file is part of bRopo.
#
# bRopo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# bRopo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with RAVE.  If not, see <http://www.gnu.org/licenses/>.
# ------------------------------------------------------------------------
# 
# bRopo make file
# @file
# @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
# @date 2011-08-26
###########################################################################
-include ../def.mk

# Drain specific c flags
#
CFLAGS= -I../ropo -I. $(PNG_INCLUDE_DIR) $(RAVE_MODULE_PYCFLAGS)

# Linker flags
#
LDFLAGS= -L../ropo -L. $(PNG_LIB_DIR) $(RAVE_MODULE_LDFLAGS)

# We need to ensure that raves extra components are managed
LIBRARIES= -lbropo $(RAVE_MODULE_PYLIBRARIES) -lpng

# --------------------------------------------------------------------
# Fixed definitions

FMIIMAGE_SOURCE= pyfmiimage.c
FMIIMAGE_OBJECTS= $(FMIIMAGE_SOURCE:.c=.o)
FMIIMAGE_TARGET= _fmiimage.so

ROPO_SOURCE= pyropogenerator.c
ROPO_OBJECTS= $(ROPO_SOURCE:.c=.o)
ROPO_TARGET= _ropogenerator.so

MAKECDEPEND=$(CC) -MM $(CFLAGS) -MT '$(@D)/$(@F)' -o $(DF).d $<

DEPDIR=.dep
DF=$(DEPDIR)/$(*F)

# Ensures that the .dep directory exists
.PHONY=$(DEPDIR)
$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

# And the rest of the make file targets
#
.PHONY=all
all:		$(FMIIMAGE_TARGET) $(ROPO_TARGET)

$(FMIIMAGE_TARGET): $(DEPDIR) $(FMIIMAGE_OBJECTS) ../ropo/libbropo.so
	$(LDSHARED) -o $@ $(FMIIMAGE_OBJECTS) $(LDFLAGS) $(LIBRARIES)
	
$(ROPO_TARGET): $(DEPDIR) $(ROPO_OBJECTS) ../ropo/libbropo.so
	$(LDSHARED) -o $@ $(ROPO_OBJECTS) $(LDFLAGS) $(LIBRARIES)

.PHONY=install
install:
	@mkdir -p ${DESTDIR}${prefix}/share/bropo/pyropo
	@cp -v -f $(FMIIMAGE_TARGET) ${DESTDIR}${prefix}/share/bropo/pyropo
	@cp -v -f $(ROPO_TARGET) ${DESTDIR}${prefix}/share/bropo/pyropo
	@cp -v -f *.py ${DESTDIR}${prefix}/share/bropo/pyropo
	@-echo "$(prefix)/share/bropo/pyropo" > "${DESTDIR}$(SITEPACK_PYTHON)/pyropo.pth"

.PHONY=clean
clean:
	@\rm -f *.o core *~
	@\rm -fr $(DEPDIR)

.PHONY=distclean		 
distclean:	clean
	@\rm -f $(FMIIMAGE_TARGET)
	@\rm -f $(ROPO_TARGET)

# --------------------------------------------------------------------
# Rules

# Contains dependency generation as well, so if you are not using
# gcc, comment out everything until the $(CC) statement.
%.o : %.c
	@$(MAKECDEPEND); \
	cp $(DF).d $(DF).P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(DF).d >> $(DF).P; \
	\rm -f $(DF).d
	$(CC) -c $(CFLAGS) $< -o $@

# NOTE! This ensures that the dependencies are setup at the right time so this should not be moved
-include $(SOURCES:%.c=$(DEPDIR)/%.P)

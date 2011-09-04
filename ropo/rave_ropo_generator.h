/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of bRack.

bRack is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

bRack is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with bRack.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/

/**
 * rave object wrapper for the ropo generator
 * This object DOES NOT support \ref #RAVE_OBJECT_CLONE.
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-09-02
 */
#ifndef RAVE_ROPO_GENERATOR_H
#define RAVE_ROPO_GENERATOR_H

#include "rave_fmi_image.h"
#include "rave_object.h"

/**
 * Defines a RaveRopoGenerator
 */
typedef struct _RaveRopoGenerator_t RaveRopoGenerator_t;

/**
 * Type definition to use when creating a rave object.
 */
extern RaveCoreObjectType RaveRopoGenerator_TYPE;

/**
 * Sets the rave fmi image to use during this generation. This will
 * clear all previous calculations.
 * @param[in] self - self
 * @param[in] image - the image to perform operations on
 */
void RaveRopoGenerator_setImage(RaveRopoGenerator_t* self, RaveFmiImage_t* image);

/**
 * Returns the fmi image associated with this generator
 * @param[in] self - self
 * @return the fmi image
 */
RaveFmiImage_t* RaveRopoGenerator_getImage(RaveRopoGenerator_t* self);

/**
 * Threshold by min dBz, detect specks < A
 * Example: -20dBz     5pix
 *
 * @param[in] self - self
 * @param[in] minDbz - min dbz
 * @param[in] maxA - max A
 * @return 1 on success otherwise 0
 */
int RaveRopoGenerator_speck(RaveRopoGenerator_t* self, int minDbz, int maxA);

/**
 * Threshold by min dBz, then detect specks, size A_max_range <=> size N*A A.
 * Example:  -20dBz   5pix  16
 *
 * @param[in] self - self
 * @param[in] minDbz - min dbz
 * @param[in] maxA - max A
 * @param[in] maxN - max N
 * @return 1 on success otherwise 0
 */
int RaveRopoGenerator_speckNormOld(RaveRopoGenerator_t* self, int minDbz, int maxA, int maxN);

/**
 * Filter unity-width emitter lines.
 * Example: -10dbz 4
 * @param[in] self - self
 * @param[in] minDbz - min dbz
 * @param[in] length - length
 * @return 1 on success otherwise 0
 */
int RaveRopoGenerator_emitter(RaveRopoGenerator_t* self, int minDbz, int length);

/**
 * Filter emitter lines.
 * Example: -10dbz, 4 bins, 2
 *
 * @param[in] self - self
 * @param[in] minDbz - min dbz
 * @param[in] length - the bin length
 * @param[in] width - the width
 * @return 1 on success otherwise 0
 */
int RaveRopoGenerator_emitter2(RaveRopoGenerator_t* self, int minDbz, int length, int width);

/**
 * Updates the classifications with the currently kept probability fields
 * @param[in] self - self
 * @return 1 on success otherwise 0
 */
int RaveRopoGenerator_classify(RaveRopoGenerator_t* self);

/**
 * Creates a restored image according to the classification table.
 * @param[in] self - self
 * @param[in] threshold - the probability threshold
 * @return the restored image.
 */
RaveFmiImage_t* RaveRopoGenerator_restore(RaveRopoGenerator_t* self, int threshold);

/**
 * Restores self. Basically the same as \ref RaveRopoGenerator_restore followed
 * by a \ref RaveRopoGenerator_setImage but the probability fields aren't
 * removed.
 * @param[in] self - self
 * @param[in] threshold - the probability threshold
 * @return 1 on success otherwise 0
 */
int RaveRopoGenerator_restoreSelf(RaveRopoGenerator_t* self, int threshold);

/**
 * Returns the a classification probability field.
 * @param[in] self - self
 * @return the classification field that is determined from all run detectors.
 */
RaveFmiImage_t* RaveRopoGenerator_getClassification(RaveRopoGenerator_t* self);

/**
 * Returns the a field containing information what detector has
 * contributed to the probability field.
 * @param[in] self - self
 * @return the markers field
 */
RaveFmiImage_t* RaveRopoGenerator_getMarkers(RaveRopoGenerator_t* self);


/**
 * Creates a ropo generator with the provided fmi image.
 * @param[in] image - the image this generator should work on
 * @return a ropo generator on success otherwise NULL
 */
RaveRopoGenerator_t* RaveRopoGenerator_new(RaveFmiImage_t* image);

#endif /* RAVE_ROPO_GENERATOR_H */

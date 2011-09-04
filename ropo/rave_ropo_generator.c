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
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fmi_util.h>
#include <fmi_image.h>
#include <fmi_image_filter.h>
#include <fmi_image_filter_line.h>
#include <fmi_image_histogram.h>
#include <fmi_image_filter_speck.h>
#include <fmi_image_filter_morpho.h>
#include <fmi_image_restore.h>
#include <fmi_meteosat.h>
#include <fmi_radar_image.h>

#include "rave_ropo_generator.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include "raveobject_list.h"

/**
 * Represents a rave fmi image
 */
struct _RaveRopoGenerator_t {
  RAVE_OBJECT_HEAD       /** Always on top */
  RaveFmiImage_t* image; /**< the fmi image */
  RaveObjectList_t* probabilities; /**< a list of probabilities */
  RaveFmiImage_t* classification; /**< the classification field */
  RaveFmiImage_t* markers; /**< the markers identifying what type of detector indicating probability */
};

/*@{ Private functions */
static const char RopoGenerator_CLEAR_STR[] = "CLEAR:";
static const char RopoGenerator_CUTOFF_STR[] = "CUTOFF:";
static const char RopoGenerator_BIOMET_STR[] = "BIOMET:";
static const char RopoGenerator_SHIP_STR[] = "SHIP:";
static const char RopoGenerator_SUN_STR[] = "SUN:";
static const char RopoGenerator_EMITTER_STR[] = "EMITTER:";
static const char RopoGenerator_EMITTER2_STR[] = "EMITTER2:";
static const char RopoGenerator_CLUTTER_STR[] = "CLUTTER:";
static const char RopoGenerator_CLUTTER2_STR[] = "CLUTTER2:";
static const char RopoGenerator_SPECK_STR[] = "SPECK:";
static const char RopoGenerator_SPECKNORMOLD_STR[] = "SPECKNORMOLD:";
static const char RopoGenerator_DOPPLER_STR[] = "DOPPLER:";
static const char RopoGenerator_GROUND_STR[] = "GROUND:";
static const char RopoGenerator_METEOSAT_STR[] = "METEOSAT:";
static const char RopoGenerator_THRESH1_STR[] = "THRESH1:";
static const char RopoGenerator_EMITTER3_STR[] = "EMITTER3:";
static const char RopoGenerator_DATA_MIN_STR[] = "DATA_MIN:";
static const char RopoGenerator_DATA_MAX_STR[] = "DATA_MAX:";
static const char RopoGenerator_NO_DATA_STR[] = "NO_DATA:";

struct RopoGenerator_PgmCodeMapping {
  FmiRadarPGMCode type;  /**< the pgm code */
  const char* str;       /**< the string representation */
};

/**
 * The product type mapping table.
 */
static const struct RopoGenerator_PgmCodeMapping PGM_CODE_MAPPING[] =
{
  {CLEAR,  RopoGenerator_CLEAR_STR},
  {CUTOFF,  RopoGenerator_CUTOFF_STR},
  {BIOMET,  RopoGenerator_BIOMET_STR},
  {SHIP,  RopoGenerator_SHIP_STR},
  {SUN,  RopoGenerator_SUN_STR},
  {EMITTER,  RopoGenerator_EMITTER_STR},
  {EMITTER2,  RopoGenerator_EMITTER2_STR},
  {CLUTTER,  RopoGenerator_CLUTTER_STR},
  {CLUTTER2,  RopoGenerator_CLUTTER2_STR},
  {SPECK,  RopoGenerator_SPECK_STR},
  {SPECK, RopoGenerator_SPECKNORMOLD_STR},
  {DOPPLER,  RopoGenerator_DOPPLER_STR},
  {METEOSAT,  RopoGenerator_METEOSAT_STR},
  {THRESH1,  RopoGenerator_THRESH1_STR},
  {EMITTER3,  RopoGenerator_EMITTER3_STR},
  {DATA_MIN,  RopoGenerator_DATA_MIN_STR},
  {DATA_MAX,  RopoGenerator_DATA_MAX_STR},
  {NO_DATA,  RopoGenerator_NO_DATA_STR},
  {-1, NULL}
};

/**
 * Constructor
 */
static int RaveRopoGenerator_constructor(RaveCoreObject* obj)
{
  RaveRopoGenerator_t* this = (RaveRopoGenerator_t*)obj;
  this->image = NULL;
  this->classification = NULL;
  this->markers = NULL;
  this->probabilities = RAVE_OBJECT_NEW(&RaveObjectList_TYPE);

  if (this->probabilities == NULL) {
    goto error;
  }
  return 1;
error:
  RAVE_OBJECT_RELEASE(this->probabilities);
  return 0;
}

/**
 * Destructor
 */
static void RaveRopoGenerator_destructor(RaveCoreObject* obj)
{
  RaveRopoGenerator_t* src = (RaveRopoGenerator_t*)obj;
  RAVE_OBJECT_RELEASE(src->image);
  RAVE_OBJECT_RELEASE(src->probabilities);
  RAVE_OBJECT_RELEASE(src->classification);
  RAVE_OBJECT_RELEASE(src->markers);
}

/**
 * Adds how/task to the image.
 * @param[in] image - the image
 * @param[in] taskname - the name of the task executed
 * @return 1 on success otherwise 0
 */
static int RaveRopoGeneratorInternal_addTask(RaveFmiImage_t* image, const char* taskname)
{
  RaveAttribute_t* attribute = NULL;
  int result = 0;
  RAVE_ASSERT((image != NULL), "image == NULL");
  RAVE_ASSERT((taskname != NULL), "taskname == NULL");

  attribute = RaveAttributeHelp_createString("how/task", taskname);
  if (attribute == NULL || !RaveFmiImage_addAttribute(image, attribute)) {
    RAVE_CRITICAL0("Failed to add attribute to image");
    goto done;
  }
  result = 1;

done:
  RAVE_OBJECT_RELEASE(attribute);
  return result;
}

/**
 * Adds how/task to the image.
 * @param[in] image - the image
 * @param[in] format - the var args format
 * @param[in] ... - the arguments
 * @return 1 on success otherwise 0
 */
static int RaveRopoGeneratorInternal_addTaskArgs(RaveFmiImage_t* image, const char* fmt, ...)
{
  RaveAttribute_t* attribute = NULL;
  int result = 0;
  char fmtstring[1024];
  va_list ap;
  int n = 0;

  RAVE_ASSERT((image != NULL), "image == NULL");
  RAVE_ASSERT((fmt != NULL), "fmt == NULL");

  va_start(ap, fmt);
  n = vsnprintf(fmtstring, 1024, fmt, ap);
  va_end(ap);
  if (n < 0 || n >= 1024) {
    RAVE_ERROR0("Failed to generate name");
    goto done;
  }

  attribute = RaveAttributeHelp_createString("how/task_args", fmtstring);
  if (attribute == NULL || !RaveFmiImage_addAttribute(image, attribute)) {
    RAVE_CRITICAL0("Failed to add attribute to image");
    goto done;
  }
  result = 1;

done:
  RAVE_OBJECT_RELEASE(attribute);
  return result;
}

static int RaveRopoGeneratorInternal_addProbabilityTaskArgs(RaveFmiImage_t* image, RaveObjectList_t* probs, const char* fmt, ...)
{
  RaveFmiImage_t* field = NULL;
  RaveAttribute_t* attr = NULL;
  int sz = 0;
  int i = 0;
  int result = 0;
  char pstr[1024];
  char fmtstring[1024];
  va_list ap;
  int n = 0;

  RAVE_ASSERT((image != NULL), "image == NULL");
  RAVE_ASSERT((probs != NULL), "probs == NULL");
  RAVE_ASSERT((fmt != NULL), "fmt == NULL");

  va_start(ap, fmt);
  n = vsnprintf(fmtstring, 1024, fmt, ap);
  va_end(ap);
  if (n < 0 || n >= 1024) {
    RAVE_ERROR0("Failed to generate name");
    goto done;
  }

  memset(pstr, '\0', 1024);

  sz = RaveObjectList_size(probs);
  for (i = 0; i < sz; i++) {
    field = (RaveFmiImage_t*)RaveObjectList_get(probs, i);
    attr = RaveFmiImage_getAttribute(field, "how/task_args");
    if (attr != NULL) {
      char* attrstr = NULL;
      if (!RaveAttribute_getString(attr, &attrstr) || attrstr == NULL) {
        goto done;
      }
      if (strlen(pstr) > 0) {
        strcat(pstr, ";");
        strcat(pstr, attrstr);
      } else {
        strcpy(pstr, attrstr);
      }
    }
    RAVE_OBJECT_RELEASE(attr);
    RAVE_OBJECT_RELEASE(field);
  }

  if (strlen(fmtstring) > 0) {
    if (strlen(pstr) > 0) {
      strcat(pstr, ";");
      strcat(pstr, fmtstring);
    } else {
      strcpy(pstr, fmtstring);
    }
  }

  attr = RaveAttributeHelp_createString("how/task_args", pstr);
  if (attr == NULL || !RaveFmiImage_addAttribute(image, attr)) {
    RAVE_CRITICAL0("Failed to add attribute to image");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(field);
  RAVE_OBJECT_RELEASE(attr);
  return result;
}

/**
 * Gets the attribute how/task_args and atempts to determine a FmiRadarPGMCode from
 * the string.
 * @param[in] image - the image
 * @return the identified PGM code on success or CLEAR if nothing could be identified
 */
static FmiRadarPGMCode RaveRopoGeneratorInternal_getPgmCode(RaveFmiImage_t* image)
{
  RaveAttribute_t* taskargs = NULL;
  FmiRadarPGMCode result = CLEAR;
  RAVE_ASSERT((image != NULL), "image == NULL");

  taskargs = RaveFmiImage_getAttribute(image, "how/task_args");
  if (taskargs != NULL) {
    char* argument = NULL;
    int index = 0;
    if (!RaveAttribute_getString(taskargs, &argument)) {
      goto done;
    }
    if (argument == NULL) {
      goto done;
    }
    while (PGM_CODE_MAPPING[index].str != NULL) {
      if (strncmp(PGM_CODE_MAPPING[index].str, argument, strlen(PGM_CODE_MAPPING[index].str))==0) {
        result = PGM_CODE_MAPPING[index].type;
        goto done;
      }
      index++;
    }
  }
done:
  RAVE_OBJECT_RELEASE(taskargs);
  return result;
}

/**
 * Creates a valid 8 bit value in range 0-255 from the wanted value translated
 * with the offset and gain.
 * E.g. If data representation is from -20dBZ and up with 0.5 dBZ intervals you
 * probably have a offset -20 and gain 0.5. Which means that
 * 1 in the data is -19.5dBZ.
 * However, this function inverts these value. value should be actual value, e.g. -20
 * and then it is converted with the offset and gain by v = (value - offset)/gain.
 *
 * @param[in] value - the requested actual value, e.g. -20 (dBZ)
 * @param[in] image - image (from where offset and gain are fetched)
 * @return the converted value (0-255) on success and negative value on failure
 */
static int RaveRopoGeneratorInternal_valueToByteRange(int value, RaveFmiImage_t* image)
{
  int result = -1;
  double offset = 0.0;
  double gain = 0.0;

  RAVE_ASSERT((image != NULL), "image == NULL");

  offset = RaveFmiImage_getOffset(image);
  gain = RaveFmiImage_getGain(image);

  if (gain != 0.0) {
    result = (int)((value - offset)/gain);
  }
  if (result <= 0) {
    result = 0;
  } else if (result >= 255) {
    result = 255;
  }
  return result;
}

/*@} End of Private functions */

/*@{ Interface functions */
void RaveRopoGenerator_setImage(RaveRopoGenerator_t* self, RaveFmiImage_t* image)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((image != NULL), "image == NULL");

  RaveRopoGenerator_declassify(self);
  RAVE_OBJECT_RELEASE(self->image);
  self->image = RAVE_OBJECT_COPY(image);
}

RaveFmiImage_t* RaveRopoGenerator_getImage(RaveRopoGenerator_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RAVE_OBJECT_COPY(self->image);
}

int RaveRopoGenerator_speck(RaveRopoGenerator_t* self, int minDbz, int maxA)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling speck when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "SPECK: %d,%d",minDbz, maxA)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_specks(RaveFmiImage_getImage(self->image),
                RaveFmiImage_getImage(probability),
                RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image),
                histogram_area);
  semisigmoid_image(RaveFmiImage_getImage(probability), maxA);
  invert_image(RaveFmiImage_getImage(probability));
  translate_intensity(RaveFmiImage_getImage(probability), 255, 0);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_speckNormOld(RaveRopoGenerator_t* self, int minDbz, int maxA, int maxN)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling speckNormOld when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "SPECKNORMOLD: %d,%d,%d",minDbz, maxA, maxN)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_specks(RaveFmiImage_getImage(self->image),
		  	    RaveFmiImage_getImage(probability),
		  	    RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image),
		  	    histogram_area);
  distance_compensation_mul(RaveFmiImage_getImage(probability), maxN);
  semisigmoid_image(RaveFmiImage_getImage(probability),maxA);
  invert_image(RaveFmiImage_getImage(probability));
  translate_intensity(RaveFmiImage_getImage(probability),255,0);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_emitter(RaveRopoGenerator_t* self, int minDbz, int length)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling emitter when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "EMITTER: %d,%d",minDbz, length)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_emitters(RaveFmiImage_getImage(self->image),
                  RaveFmiImage_getImage(probability),
                  RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image), length);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_emitter2(RaveRopoGenerator_t* self, int minDbz, int length, int width)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling emitter2 when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "EMITTER2: %d,%d,%d",minDbz, length, width)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_emitters2(RaveFmiImage_getImage(self->image),
                   RaveFmiImage_getImage(probability),
                   RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image),
                   length,
                   width);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_clutter(RaveRopoGenerator_t* self, int minDbz, int maxCompactness)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling clutter when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "CLUTTER: %d,%d",minDbz, maxCompactness)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_specks(RaveFmiImage_getImage(self->image),
                RaveFmiImage_getImage(probability),
                RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image),
                histogram_compactness);
  semisigmoid_image(RaveFmiImage_getImage(probability),maxCompactness);
  invert_image(RaveFmiImage_getImage(probability));
  translate_intensity(RaveFmiImage_getImage(probability),255,0);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_clutter2(RaveRopoGenerator_t* self, int minDbz, int maxSmoothness)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling clutter2 when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "CLUTTER2: %d,%d",minDbz, maxSmoothness)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_specks(RaveFmiImage_getImage(self->image),
                RaveFmiImage_getImage(probability),
                RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image),
                histogram_smoothness);
  invert_image(RaveFmiImage_getImage(probability));
  translate_intensity(RaveFmiImage_getImage(probability),255,0);
  semisigmoid_image(RaveFmiImage_getImage(probability),255-maxSmoothness);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_softcut(RaveRopoGenerator_t* self, int maxDbz, int r, int r2)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling softcut when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "SOFTCUT: %d,%d,%d",maxDbz, r, r2)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_insect_band(RaveFmiImage_getImage(self->image),
                     RaveFmiImage_getImage(probability),
                     RaveRopoGeneratorInternal_valueToByteRange(maxDbz, self->image),
                     r, r2);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_biomet(RaveRopoGenerator_t* self, int maxDbz, int dbzDelta, int maxAlt, int altDelta)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling biomet when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "BIOMET: %d,%d,%d,%d",maxDbz, dbzDelta, maxAlt, altDelta)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_biomet(RaveFmiImage_getImage(self->image),
                RaveFmiImage_getImage(probability),
                RaveRopoGeneratorInternal_valueToByteRange(maxDbz, self->image),
                rel_dbz_to_byte(dbzDelta),
                maxAlt,
                altDelta);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_ship(RaveRopoGenerator_t* self, int minRelDbz, int minA)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling ship when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "SHIP: %d,%d",minRelDbz, minA)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_ships(RaveFmiImage_getImage(self->image),
               RaveFmiImage_getImage(probability),
               rel_dbz_to_byte(minRelDbz),
               minA);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_sun(RaveRopoGenerator_t* self, int minDbz, int minLength, int maxThickness)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling sun when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "SUN: %d,%d,%d", minDbz, minLength, maxThickness)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_sun(RaveFmiImage_getImage(self->image),
             RaveFmiImage_getImage(probability),
             RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image),
             maxThickness,
             minLength);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_sun2(RaveRopoGenerator_t* self, int minDbz, int minLength, int maxThickness, int azimuth, int elevation)
{
  RaveFmiImage_t* probability = NULL;

  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling sun when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector") ||
      !RaveRopoGeneratorInternal_addTaskArgs(probability, "SUN2: %d,%d,%d,%d,%d", minDbz, minLength, maxThickness,azimuth,elevation)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  detect_sun2(RaveFmiImage_getImage(self->image),
              RaveFmiImage_getImage(probability),
              RaveRopoGeneratorInternal_valueToByteRange(minDbz, self->image),
              maxThickness,
              minLength,
              azimuth,
              elevation);

  if (!RaveObjectList_add(self->probabilities, (RaveCoreObject*)probability)) {
    RAVE_ERROR0("Failed to add probability field to probabilities");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  return result;
}

int RaveRopoGenerator_classify(RaveRopoGenerator_t* self)
{
  RaveFmiImage_t* probability = NULL;
  RaveFmiImage_t* markers = NULL;
  FmiImage* fmiProbImage = NULL;
  FmiImage* fmiMarkersImage = NULL;
  int result = 0;
  int probCount = 0;
  int i = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->image == NULL) {
    RAVE_ERROR0("Calling speck when there is no image to operate on?");
    goto done;
  }

  probability = RAVE_OBJECT_CLONE(self->image);
  markers = RAVE_OBJECT_CLONE(self->image);
  if (probability == NULL || markers == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  RaveFmiImage_clear(probability, CLEAR);
  RaveFmiImage_clear(markers, CLEAR);

  probCount = RaveObjectList_size(self->probabilities);

  fmiProbImage = RaveFmiImage_getImage(probability);
  fmiMarkersImage = RaveFmiImage_getImage(markers);

  for (i = 0; i < probCount; i++) {
    int j = 0;
    RaveFmiImage_t* image = (RaveFmiImage_t*)RaveObjectList_get(self->probabilities, i);
    FmiImage* probImage = RaveFmiImage_getImage(image);
    FmiRadarPGMCode pgmCode = RaveRopoGeneratorInternal_getPgmCode(image);
    for (j = 0; j < fmiProbImage->volume; j++) {
      if (probImage->array[j] >= fmiProbImage->array[j]) {
        fmiMarkersImage->array[j]=pgmCode;
        fmiProbImage->array[j]=probImage->array[j];
      }
    }
    RAVE_OBJECT_RELEASE(image);
  }

  if (!RaveRopoGeneratorInternal_addTask(probability, "fi.fmi.ropo.detector.classification") ||
      !RaveRopoGeneratorInternal_addProbabilityTaskArgs(probability, self->probabilities, "")) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  if (!RaveRopoGeneratorInternal_addTask(markers, "fi.fmi.ropo.detector.classification_marker") ||
      !RaveRopoGeneratorInternal_addProbabilityTaskArgs(markers, self->probabilities, "")) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  RAVE_OBJECT_RELEASE(self->classification);
  RAVE_OBJECT_RELEASE(self->markers);
  self->classification = RAVE_OBJECT_COPY(probability);
  self->markers = RAVE_OBJECT_COPY(markers);

  result = 1;
done:
  RAVE_OBJECT_RELEASE(probability);
  RAVE_OBJECT_RELEASE(markers);
  return result;
}

void RaveRopoGenerator_declassify(RaveRopoGenerator_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_OBJECT_RELEASE(self->classification);
  RAVE_OBJECT_RELEASE(self->markers);
  RaveObjectList_clear(self->probabilities);
}

RaveFmiImage_t* RaveRopoGenerator_restore(RaveRopoGenerator_t* self, int threshold)
{
  RaveFmiImage_t* restored = NULL;
  RaveFmiImage_t* result = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (self->classification == NULL || self->markers == NULL) {
    RaveRopoGenerator_classify(self);
  }
  restored = RAVE_OBJECT_CLONE(self->image);
  if (restored == NULL) {
    RAVE_CRITICAL0("Failed to clone image");
    goto done;
  }

  RaveFmiImage_clear(restored, CLEAR);

  if (!RaveRopoGeneratorInternal_addTask(restored, "fi.fmi.ropo.restore") ||
      !RaveRopoGeneratorInternal_addProbabilityTaskArgs(restored, self->probabilities, "RESTORE_THRESH: %d",threshold)) {
    RAVE_CRITICAL0("Failed to add task arguments");
    goto done;
  }

  restore_image(RaveFmiImage_getImage(self->image),
                RaveFmiImage_getImage(restored),
                RaveFmiImage_getImage(self->classification),
                threshold);

  result = RAVE_OBJECT_COPY(restored);
done:
  RAVE_OBJECT_RELEASE(restored);
  return result;
}

int RaveRopoGenerator_restoreSelf(RaveRopoGenerator_t* self, int threshold)
{
  RaveFmiImage_t* restored = NULL;
  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  restored = RaveRopoGenerator_restore(self, threshold);

  if (restored == NULL) {
    RAVE_ERROR0("Failed to restore self");
    goto done;
  }

  RAVE_OBJECT_RELEASE(self->image);
  self->image = RAVE_OBJECT_COPY(restored);

  result = 1;
done:
  RAVE_OBJECT_RELEASE(restored);
  return result;
}

int RaveRopoGenerator_getProbabilityFieldCount(RaveRopoGenerator_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveObjectList_size(self->probabilities);
}

RaveFmiImage_t* RaveRopoGenerator_getProbabilityField(RaveRopoGenerator_t* self, int index)
{
  RaveCoreObject* object = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");

  object = RaveObjectList_get(self->probabilities, index);

  return (RaveFmiImage_t*)object;
}

RaveFmiImage_t* RaveRopoGenerator_getClassification(RaveRopoGenerator_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (self->classification == NULL || self->markers == NULL) {
    RaveRopoGenerator_classify(self);
  }
  return RAVE_OBJECT_COPY(self->classification);
}

RaveFmiImage_t* RaveRopoGenerator_getMarkers(RaveRopoGenerator_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (self->classification == NULL || self->markers == NULL) {
    RaveRopoGenerator_classify(self);
  }
  return RAVE_OBJECT_COPY(self->markers);
}

RaveRopoGenerator_t* RaveRopoGenerator_new(RaveFmiImage_t* image)
{
  RaveRopoGenerator_t* result = NULL;
  RAVE_ASSERT((image != NULL), "image == NULL");

  result = RAVE_OBJECT_NEW(&RaveRopoGenerator_TYPE);

  if (result == NULL) {
    RAVE_CRITICAL0("Failed to allocate memory for generator");
    return NULL;
  }
  result->image = RAVE_OBJECT_COPY(image);

  return result;
}

/*@} End of Interface functions */
RaveCoreObjectType RaveRopoGenerator_TYPE = {
    "RaveRopoGenerator",
    sizeof(RaveRopoGenerator_t),
    RaveRopoGenerator_constructor,
    RaveRopoGenerator_destructor,
    NULL /* No copy constructor */
};

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
 * rave object wrapper for the fmi_image as a volume.
 * This object DOES NOT support \ref #RAVE_OBJECT_CLONE.
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-08-31
 */
#include "rave_fmi_volume.h"
#include "rave_debug.h"
#include "rave_alloc.h"

/**
 * Represents a rave fmi image
 */
struct _RaveFmiVolume_t {
  RAVE_OBJECT_HEAD /** Always on top */
  FmiImage* image; /**< the fmi image */
  int sweepCount;  /**< the sweep count */
};

/*@{ Private functions */
/**
 * Constructor
 */
static int RaveFmiVolume_constructor(RaveCoreObject* obj)
{
  RaveFmiVolume_t* this = (RaveFmiVolume_t*)obj;
  this->image = NULL;
  this->sweepCount = 0;
  return 1;
}

/**
 * Releases all memory allocated in the fmi image.
 * @param[in] img - the rave fmi volume to reset
 */
static void RaveFmiVolumeInternal_resetImage(RaveFmiVolume_t* img)
{
  int i = 0;
  if (img->image != NULL) {
    for (i = 0; i < img->sweepCount; i++) {
      if (img->image[i].heights != NULL) {
        RAVE_FREE(img->image[i].heights);
      }
      if (img->image[i].array != NULL) {
        RAVE_FREE(img->image[i].array);
      }
    }
    RAVE_FREE(img->image);
  }
  img->image = NULL;
  img->sweepCount = 0;
}

/**
 * Destructor
 */
static void RaveFmiVolume_destructor(RaveCoreObject* obj)
{
  RaveFmiVolume_t* src = (RaveFmiVolume_t*)obj;
  RaveFmiVolumeInternal_resetImage(src);
}

/**
 * Counts the number of sweeps that a volume would result in.
 * @param[in] volume - the rave volume
 * @param[in] quantity - the quantity of the parameter (if NULL, default parameter will be used)
 */
static int RaveFmiVolumeInternal_getSweepCount(PolarVolume_t* volume, const char* quantity)
{
  int sweepCount = 0;
  int i = 0;
  int nrScans = 0;
  RAVE_ASSERT((volume != NULL), "volume == NULL");

  nrScans = PolarVolume_getNumberOfScans(volume);

  /**
   * Count number of scans with wanted quantity
   */
  for (i = 0; i < nrScans; i++) {
    PolarScan_t* scan = PolarVolume_getScan(volume, i);
    if (scan != NULL) {
      if (quantity == NULL || PolarScan_hasParameter(scan, quantity)) {
        sweepCount++;
      }
    }
    RAVE_OBJECT_RELEASE(scan);
  }

  return sweepCount;
}

/**
 * Converts a rave polar scan into a fmi image
 * @param[in] scan - the rave polar scan
 * @param[in] quantity - the quantity to use (if NULL, default parameter is assumed)
 * @param[in] image - the image to be filled
 * @return 1 on success otherwise 0
 */
static int RaveFmiVolumeInternal_scanToFmiImage(PolarScan_t* scan, const char* quantity, FmiImage* image)
{
  int i = 0, j = 0;
  int result = 0;
  PolarScanParam_t* param = NULL;

  RAVE_ASSERT((scan != NULL), "scan == NULL");
  RAVE_ASSERT((image != NULL), "image == NULL");

  image->width=PolarScan_getNbins(scan);
  image->height=PolarScan_getNrays(scan);
  image->bin_depth=PolarScan_getRscale(scan);
  image->elevation_angle=PolarScan_getElangle(scan) * 180.0 / M_PI; /* elangles in degrees for ropo */
  image->max_value=255;
  image->channels=1;

  initialize_image(image);

  if (quantity == NULL) {
    param = PolarScan_getParameter(scan, "DBZH");
  } else {
    param = PolarScan_getParameter(scan, quantity);
  }

  if (param == NULL) {
    RAVE_WARNING1("Failed to extract parameter %s from scan", quantity);
    goto done;
  }
  if (PolarScanParam_getDataType(param) != RaveDataType_CHAR &&
      PolarScanParam_getDataType(param) != RaveDataType_UCHAR) {
    RAVE_WARNING0("FmiImages can only support 8-bit data");
    goto done;
  }

  for (j = 0; j < image->height; j++) {
    for (i = 0; i < image->width; i++) {
      double value = 0.0;
      PolarScanParam_getValue(param, i, j, &value);
      put_pixel(image, i, j, 0, (Byte)(value)); // why + 0.5 ?
    }
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(param);
  return result;
}

/**
 * Converts a rave field into a fmi image
 * @param[in] field - the rave field
 * @param[in] image - the image to be filled
 * @return 1 on success otherwise 0
 */
static int RaveFmiVolumeInternal_fieldToFmiImage(RaveField_t* field, FmiImage* image)
{
  int i = 0, j = 0;
  int result = 0;

  RAVE_ASSERT((field != NULL), "field == NULL");
  RAVE_ASSERT((image != NULL), "image == NULL");

  image->width=RaveField_getXsize(field);
  image->height=RaveField_getYsize(field);
  image->channels=1;

  initialize_image(image);

  if (RaveField_getDataType(field) != RaveDataType_CHAR &&
      RaveField_getDataType(field) != RaveDataType_UCHAR) {
    RAVE_WARNING0("FmiImages can only support 8-bit data");
    goto done;
  }

  for (j = 0; j < image->height; j++) {
    for (i = 0; i < image->width; i++) {
      double value = 0.0;
      RaveField_getValue(field, i, j, &value);
      put_pixel(image, i, j, 0, (Byte)(value)); // why + 0.5 ?
    }
  }

  result = 1;
done:
  return result;
}

/**
 * Creates a polar scan from an fmi image.
 * @param[in] image - the fmi image
 * @param[in] quantity - the quantity of the parameter. If null, default is DBZH.
 * @return the polar scan on success otherwise NULL
 */
static PolarScan_t* RaveFmiVolumeInternal_fmiImageToScan(FmiImage* image, const char* quantity)
{
  PolarScan_t* scan = NULL;
  PolarScan_t* result = NULL;
  PolarScanParam_t* param = NULL;
  int ray = 0, bin = 0;
  RAVE_ASSERT((image != NULL), "image == NULL");

  scan = RAVE_OBJECT_NEW(&PolarScan_TYPE);
  param = RAVE_OBJECT_NEW(&PolarScanParam_TYPE);
  if (scan == NULL || param == NULL) {
    RAVE_CRITICAL0("Failed to allocate memory for polar scan");
    goto done;
  }

  PolarScanParam_setGain(param, 1.0);
  PolarScanParam_setOffset(param, 0.0);
  PolarScanParam_setNodata(param, 255.0);
  PolarScanParam_setUndetect(param, 0.0);
  if (quantity != NULL) {
    PolarScanParam_setQuantity(param, quantity);
  } else {
    PolarScanParam_setQuantity(param, "DBZH");
  }
  if (!PolarScanParam_createData(param, image->width, image->height, RaveDataType_UCHAR)) {
    RAVE_CRITICAL0("Failed to allocate memory for data");
    goto done;
  }

  for (ray = 0; ray < image->height; ray++) {
    for (bin = 0; bin < image->width; bin++) {
      PolarScanParam_setValue(param, bin, ray, (double)get_pixel(image, bin, ray, 0));
    }
  }
  if (!PolarScan_addParameter(scan, param)) {
    RAVE_CRITICAL0("Failed to add parameter to scan");
    goto done;
  }
  PolarScan_setRscale(scan, image->bin_depth);
  PolarScan_setElangle(scan, image->elevation_angle * M_PI / 180.0);
  PolarScan_setRstart(scan, 0.0);

  result = RAVE_OBJECT_COPY(scan);
done:
  RAVE_OBJECT_RELEASE(scan);
  RAVE_OBJECT_RELEASE(param);
  return result;
}

/*@} End of Private functions */

/*@{ Interface functions */
int RaveFmiVolume_initialize(RaveFmiVolume_t* self, int sweepCount)
{
  int result = 0;
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (sweepCount <= 0) {
    RAVE_ERROR0("You can not initialize a fmi image with sweepCount <= 0");
  } else {
    FmiImage* images = new_image(sweepCount);
    if (images != NULL) {
      RaveFmiVolumeInternal_resetImage(self);
      self->image = images;
      self->sweepCount = sweepCount;
      result = 1;
    }
  }
  return result;
}

int RaveFmiVolume_getSweepCount(RaveFmiVolume_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->sweepCount;
}

FmiImage* RaveFmiVolume_getImage(RaveFmiVolume_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->image;
}

FmiImage* RaveFmiVolume_getSweep(RaveFmiVolume_t* self, int sweep)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (sweep >= 0 && sweep < self->sweepCount) {
    return &self->image[sweep];
  }
  return NULL;
}

PolarVolume_t* RaveFmiVolume_toRave(RaveFmiVolume_t* self, const char* quantity)
{
  PolarVolume_t* result = NULL;
  PolarVolume_t* volume = NULL;
  PolarScan_t* scan = NULL;

  int i = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  volume = RAVE_OBJECT_NEW(&PolarVolume_TYPE);
  if (volume == NULL) {
    RAVE_CRITICAL0("Failed to create volume");
    goto done;
  }

  for (i = 0; i < self->sweepCount; i++) {
    scan = RaveFmiVolumeInternal_fmiImageToScan(&self->image[i], quantity);
    if (scan == NULL) {
      RAVE_CRITICAL0("Failed to convert image to scan");
      goto done;
    }
    if (!PolarVolume_addScan(volume, scan)) {
      RAVE_CRITICAL0("Failed to add scan to volume");
      goto done;
    }
    RAVE_OBJECT_RELEASE(scan);
  }

  result = RAVE_OBJECT_COPY(volume);
done:
  RAVE_OBJECT_RELEASE(scan);
  RAVE_OBJECT_RELEASE(volume);
  return result;
}

RaveFmiVolume_t* RaveFmiVolume_new(int sweepCount)
{
  RaveFmiVolume_t* result = RAVE_OBJECT_NEW(&RaveFmiVolume_TYPE);
  if (result != NULL) {
    if (!RaveFmiVolume_initialize(result, sweepCount)) {
      RAVE_OBJECT_RELEASE(result);
    }
  }
  return result;
}

RaveFmiVolume_t* RaveFmiVolume_fromPolarVolume(PolarVolume_t* volume, const char* quantity)
{
  int sweepCount = 0;
  int nrScans = 0;
  int i = 0;
  int sweepIndex = 0;
  RaveFmiVolume_t* image = NULL;
  RaveFmiVolume_t* result = NULL;

  RAVE_ASSERT((volume != NULL), "volume == NULL");

  PolarVolume_sortByElevations(volume, 1);
  nrScans = PolarVolume_getNumberOfScans(volume);
  sweepCount = RaveFmiVolumeInternal_getSweepCount(volume, quantity);

  if (sweepCount <= 0) {
    RAVE_WARNING0("Volume does not contain any wanted parameters");
    goto done;
  }

  image = RaveFmiVolume_new(sweepCount);
  if (image == NULL) {
    RAVE_CRITICAL0("Failed to create fmi image");
    goto done;
  }

  for (i = 0; i < nrScans; i++) {
    PolarScan_t* scan = PolarVolume_getScan(volume, i);
    if (quantity == NULL || PolarScan_hasParameter(scan, quantity)) {
      if (!RaveFmiVolumeInternal_scanToFmiImage(scan, quantity, &image->image[sweepIndex])) {
        RAVE_ERROR0("Failed to convert scan to fmi image");
        RAVE_OBJECT_RELEASE(scan);
        goto done;
      }
      sweepIndex++;
    }
    RAVE_OBJECT_RELEASE(scan);
  }

  result = RAVE_OBJECT_COPY(image);
done:
  RAVE_OBJECT_RELEASE(image);
  return result;
}

RaveFmiVolume_t* RaveFmiVolume_fromPolarScan(PolarScan_t* scan, const char* quantity)
{
  RaveFmiVolume_t* image = NULL;
  RaveFmiVolume_t* result = NULL;

  RAVE_ASSERT((scan != NULL), "scan == NULL");

  image = RaveFmiVolume_new(1);
  if (image == NULL) {
    RAVE_CRITICAL0("Failed to create fmi image");
    goto done;
  }

  if (quantity == NULL || PolarScan_hasParameter(scan, quantity)) {
    if (!RaveFmiVolumeInternal_scanToFmiImage(scan, quantity, &image->image[0])) {
      RAVE_ERROR0("Failed to convert scan to fmi image");
      goto done;
    }
  }

  result = RAVE_OBJECT_COPY(image);
done:
  RAVE_OBJECT_RELEASE(image);
  return result;
}

RaveFmiVolume_t* RaveFmiVolume_fromRaveField(RaveField_t* field)
{
  RaveFmiVolume_t* image = NULL;
  RaveFmiVolume_t* result = NULL;

  RAVE_ASSERT((field != NULL), "field == NULL");

  image = RaveFmiVolume_new(1);
  if (image == NULL) {
    RAVE_CRITICAL0("Failed to create fmi image");
    goto done;
  }

  if (!RaveFmiVolumeInternal_fieldToFmiImage(field, &image->image[0])) {
    RAVE_ERROR0("Failed to convert rave field to fmi image");
    goto done;
  }

  result = RAVE_OBJECT_COPY(image);
done:
  RAVE_OBJECT_RELEASE(image);
  return result;
}

RaveFmiVolume_t* RaveFmiVolume_fromRave(RaveCoreObject* object, const char* quantity)
{
  RAVE_ASSERT((object != NULL), "object == NULL");
  if (RAVE_OBJECT_CHECK_TYPE(object, &PolarVolume_TYPE)) {
    return RaveFmiVolume_fromPolarVolume((PolarVolume_t*)object, quantity);
  } else if (RAVE_OBJECT_CHECK_TYPE(object, &PolarScan_TYPE)) {
    return RaveFmiVolume_fromPolarScan((PolarScan_t*)object, quantity);
  } else if (RAVE_OBJECT_CHECK_TYPE(object, &RaveField_TYPE)) {
    return RaveFmiVolume_fromRaveField((RaveField_t*)object);
  } else {
    RAVE_ERROR1("RaveFmiVolume_fromRave does not support %s", object->roh_type->name);
  }
  return NULL;
}

/*@} End of Interface functions */
RaveCoreObjectType RaveFmiVolume_TYPE = {
    "RaveFmiVolume",
    sizeof(RaveFmiVolume_t),
    RaveFmiVolume_constructor,
    RaveFmiVolume_destructor,
    NULL /* No copy constructor */
};

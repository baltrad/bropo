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
 * rave object wrapper for the fmi_image.
 * This object supports \ref #RAVE_OBJECT_CLONE.
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-08-31
 */
#include "rave_fmi_image.h"
#include "raveobject_hashtable.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include <string.h>

/**
 * Represents a rave fmi image
 */
struct _RaveFmiImage_t {
  RAVE_OBJECT_HEAD /** Always on top */
  FmiImage* image; /**< the fmi image */
  RaveObjectHashTable_t* attrs; /**< attributes */
};

/*@{ Private functions */

/**
 * Releases all memory allocated in the fmi image.
 * @param[in] img - the rave fmi image to reset
 */
static void RaveFmiImageInternal_resetImage(RaveFmiImage_t* img)
{
  if (img->image != NULL) {
    if (img->image->heights != NULL) {
      free(img->image->heights);
    }
    if (img->image->array != NULL) {
      free(img->image->array);
    }
    free(img->image);
  }
  img->image = NULL;
}

/**
 * Constructor
 */
static int RaveFmiImage_constructor(RaveCoreObject* obj)
{
  RaveFmiImage_t* this = (RaveFmiImage_t*)obj;
  this->image = new_image(1);
  this->attrs = RAVE_OBJECT_NEW(&RaveObjectHashTable_TYPE);
  if (this->image == NULL || this->attrs == NULL) {
    goto error;
  }

  return 1;
error:
  RaveFmiImageInternal_resetImage(this);
  RAVE_OBJECT_RELEASE(this->attrs);
  return 0;
}

/**
 * Destructor
 */
static void RaveFmiImage_destructor(RaveCoreObject* obj)
{
  RaveFmiImage_t* src = (RaveFmiImage_t*)obj;
  RaveFmiImageInternal_resetImage(src);
  RAVE_OBJECT_RELEASE(src->attrs);
}

/**
 * Copy constructor
 */
/**
 * Copy constructor.
 */
static int RaveFmiImage_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  RaveFmiImage_t* this = (RaveFmiImage_t*)obj;
  RaveFmiImage_t* src = (RaveFmiImage_t*)srcobj;
  this->image = new_image(1);
  this->attrs = RAVE_OBJECT_CLONE(src->attrs);
  if (this->image == NULL || this->attrs == NULL) {
    goto error;
  }
  canonize_image(src->image, this->image);
  return 1;
error:
  RAVE_OBJECT_RELEASE(this->image);
  RAVE_OBJECT_RELEASE(this->attrs);
  return 0;
}

/**
 * Converts a rave polar scan into a fmi image
 * @param[in] scan - the rave polar scan
 * @param[in] quantity - the quantity to use (if NULL, default parameter is assumed)
 * @param[in] image - the image to be filled
 * @return 1 on success otherwise 0
 */
static int RaveFmiImageInternal_scanToFmiImage(PolarScan_t* scan, const char* quantity, FmiImage* image)
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
static int RaveFmiImageInternal_fieldToFmiImage(RaveField_t* field, FmiImage* image)
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
static PolarScan_t* RaveFmiImageInternal_fmiImageToScan(FmiImage* image, const char* quantity)
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

/**
 * Creates a rave field from an fmi image.
 * @param[in] image - the fmi image
 * @param[in] quantity - the quantity of the parameter. If null, default is DBZH.
 * @return the rave field on success otherwise NULL
 */
static RaveField_t* RaveFmiImageInternal_fmiImageToField(FmiImage* image)
{
  RaveField_t* field = NULL;
  RaveField_t* result = NULL;
  int ray = 0, bin = 0;
  RAVE_ASSERT((image != NULL), "image == NULL");

  field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (field == NULL) {
    RAVE_CRITICAL0("Failed to allocate memory for field");
    goto done;
  }

  if (!RaveField_createData(field, image->width, image->height, RaveDataType_UCHAR)) {
    RAVE_CRITICAL0("Failed to allocate memory for data");
    goto done;
  }

  for (ray = 0; ray < image->height; ray++) {
    for (bin = 0; bin < image->width; bin++) {
      RaveField_setValue(field, bin, ray, (double)get_pixel(image, bin, ray, 0));
    }
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/*@} End of Private functions */

/*@{ Interface functions */
int RaveFmiImage_initialize(RaveFmiImage_t* self, int width, int height)
{
  int result = 0;
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (width <= 0 || height <= 0) {
    RAVE_ERROR0("You can not initialize a fmi image with width or height = 0");
  } else {
    FmiImage* images = new_image(1);
    if (images != NULL) {
      RaveFmiImageInternal_resetImage(self);
      self->image = images;
      self->image->width = width;
      self->image->height = height;
      initialize_image(self->image);
      result = 1;
    }
  }
  return result;
}

void RaveFmiImage_clear(RaveFmiImage_t* self, unsigned char v)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (self->image != NULL) {
    fill_image(self->image, (Byte)v);
  }
}

FmiImage* RaveFmiImage_getImage(RaveFmiImage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->image;
}

PolarScan_t* RaveFmiImage_toPolarScan(RaveFmiImage_t* self, const char* quantity)
{
  PolarScan_t* result = NULL;
  PolarScan_t* scan = NULL;
  RaveObjectList_t* attributes = NULL;

  int i = 0;
  int nrAttrs = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  scan = RaveFmiImageInternal_fmiImageToScan(&self->image[i], quantity);
  if (scan == NULL) {
    RAVE_CRITICAL0("Failed to convert image to scan");
    goto done;
  }

  attributes = RaveObjectHashTable_values(self->attrs);
  if (attributes == NULL) {
    RAVE_CRITICAL0("Failed to get attributes");
    goto done;
  }
  nrAttrs = RaveObjectList_size(attributes);
  for (i = 0; i < nrAttrs; i++) {
    RaveAttribute_t* attr = (RaveAttribute_t*)RaveObjectList_get(attributes, i);
    PolarScan_addAttribute(scan, attr);
    RAVE_OBJECT_RELEASE(attr);
  }

  result = RAVE_OBJECT_COPY(scan);
done:
  RAVE_OBJECT_RELEASE(attributes);
  RAVE_OBJECT_RELEASE(scan);
  return result;
}

RaveField_t* RaveFmiImage_toRaveField(RaveFmiImage_t* self)
{
  RaveField_t* result = NULL;
  RaveField_t* field = NULL;
  RaveObjectList_t* attributes = NULL;

  int i = 0;
  int nrAttrs = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  field = RaveFmiImageInternal_fmiImageToField(&self->image[i]);
  if (field == NULL) {
    RAVE_CRITICAL0("Failed to convert image to field");
    goto done;
  }

  attributes = RaveObjectHashTable_values(self->attrs);
  if (attributes == NULL) {
    RAVE_CRITICAL0("Failed to get attributes");
    goto done;
  }

  nrAttrs = RaveObjectList_size(attributes);
  for (i = 0; i < nrAttrs; i++) {
    RaveAttribute_t* attr = (RaveAttribute_t*)RaveObjectList_get(attributes, i);
    RaveField_addAttribute(field, attr);
    RAVE_OBJECT_RELEASE(attr);
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(attributes);
  RAVE_OBJECT_RELEASE(field);
  return result;
}

int RaveFmiImage_addAttribute(RaveFmiImage_t* self,  RaveAttribute_t* attribute)
{
  char* aname = NULL;
  char* gname = NULL;
  const char* name = NULL;
  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  name = RaveAttribute_getName(attribute);
  if (!RaveAttributeHelp_extractGroupAndName(name, &gname, &aname)) {
    RAVE_ERROR1("Failed to extract group and name from %s", name);
    goto done;
  }

  if ((strcasecmp("how", gname)==0 ||
       strcasecmp("what", gname)==0 ||
       strcasecmp("where", gname)==0) &&
    strchr(aname, '/') == NULL) {
    result = RaveObjectHashTable_put(self->attrs, name, (RaveCoreObject*)attribute);
  }

done:
  RAVE_FREE(gname);
  RAVE_FREE(aname);
  return result;
}

RaveAttribute_t* RaveFmiImage_getAttribute(RaveFmiImage_t* self, const char* name)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (name == NULL) {
    RAVE_ERROR0("Trying to get an attribute with NULL name");
    return NULL;
  }
  return (RaveAttribute_t*)RaveObjectHashTable_get(self->attrs, name);
}

RaveList_t* RaveFmiImage_getAttributeNames(RaveFmiImage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveObjectHashTable_keys(self->attrs);
}

RaveFmiImage_t* RaveFmiImage_new(int width, int height)
{
  RaveFmiImage_t* result = RAVE_OBJECT_NEW(&RaveFmiImage_TYPE);
  if (result != NULL) {
    if (!RaveFmiImage_initialize(result, width, height)) {
      RAVE_OBJECT_RELEASE(result);
    }
  }
  return result;
}

RaveFmiImage_t* RaveFmiImage_fromPolarVolume(PolarVolume_t* volume, int scannr, const char* quantity)
{
  int nrScans = 0;
  PolarScan_t* scan = NULL;
  RaveFmiImage_t* image = NULL;
  RaveFmiImage_t* result = NULL;

  RAVE_ASSERT((volume != NULL), "volume == NULL");

  nrScans = PolarVolume_getNumberOfScans(volume);

  if (scannr < 0 || scannr >= nrScans) {
    RAVE_ERROR1("There is no scan at index %d for this volume.", scannr);
    goto done;
  }

  image = RAVE_OBJECT_NEW(&RaveFmiImage_TYPE);
  if (image == NULL) {
    RAVE_CRITICAL0("Failed to create fmi image");
    goto done;
  }

  scan = PolarVolume_getScan(volume, scannr);
  if (scan == NULL) {
    RAVE_ERROR1("Could not read scan at index %d.", scannr);
    goto done;
  }
  if (quantity == NULL || PolarScan_hasParameter(scan, quantity)) {
    if (!RaveFmiImageInternal_scanToFmiImage(scan, quantity, &image->image[0])) {
      RAVE_ERROR0("Failed to convert scan to fmi image");
      goto done;
    }
  }

  result = RAVE_OBJECT_COPY(image);
done:
  RAVE_OBJECT_RELEASE(scan);
  RAVE_OBJECT_RELEASE(image);
  return result;
}

RaveFmiImage_t* RaveFmiImage_fromPolarScan(PolarScan_t* scan, const char* quantity)
{
  RaveFmiImage_t* image = NULL;
  RaveFmiImage_t* result = NULL;

  RAVE_ASSERT((scan != NULL), "scan == NULL");

  image = RAVE_OBJECT_NEW(&RaveFmiImage_TYPE);
  if (image == NULL) {
    RAVE_CRITICAL0("Failed to create fmi image");
    goto done;
  }

  if (quantity == NULL || PolarScan_hasParameter(scan, quantity)) {
    if (!RaveFmiImageInternal_scanToFmiImage(scan, quantity, &image->image[0])) {
      RAVE_ERROR0("Failed to convert scan to fmi image");
      goto done;
    }
  }

  result = RAVE_OBJECT_COPY(image);
done:
  RAVE_OBJECT_RELEASE(image);
  return result;
}

RaveFmiImage_t* RaveFmiImage_fromRaveField(RaveField_t* field)
{
  RaveFmiImage_t* image = NULL;
  RaveFmiImage_t* result = NULL;

  RAVE_ASSERT((field != NULL), "field == NULL");

  image = RAVE_OBJECT_NEW(&RaveFmiImage_TYPE);
  if (image == NULL) {
    RAVE_CRITICAL0("Failed to create fmi image");
    goto done;
  }

  if (!RaveFmiImageInternal_fieldToFmiImage(field, &image->image[0])) {
    RAVE_ERROR0("Failed to convert rave field to fmi image");
    goto done;
  }

  result = RAVE_OBJECT_COPY(image);
done:
  RAVE_OBJECT_RELEASE(image);
  return result;
}

RaveFmiImage_t* RaveFmiImage_fromRave(RaveCoreObject* object, const char* quantity)
{
  RAVE_ASSERT((object != NULL), "object == NULL");
  if (RAVE_OBJECT_CHECK_TYPE(object, &PolarVolume_TYPE)) {
    return RaveFmiImage_fromPolarVolume((PolarVolume_t*)object, 0, quantity);
  } else if (RAVE_OBJECT_CHECK_TYPE(object, &PolarScan_TYPE)) {
    return RaveFmiImage_fromPolarScan((PolarScan_t*)object, quantity);
  } else if (RAVE_OBJECT_CHECK_TYPE(object, &RaveField_TYPE)) {
    return RaveFmiImage_fromRaveField((RaveField_t*)object);
  } else {
    RAVE_ERROR1("RaveFmiImage_fromRave does not support %s", object->roh_type->name);
  }
  return NULL;
}

/*@} End of Interface functions */
RaveCoreObjectType RaveFmiImage_TYPE = {
    "RaveFmiImage",
    sizeof(RaveFmiImage_t),
    RaveFmiImage_constructor,
    RaveFmiImage_destructor,
    RaveFmiImage_copyconstructor /* No copy constructor */
};

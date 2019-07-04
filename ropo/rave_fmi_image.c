/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of bRopo.

bRopo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

bRopo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with bRopo.  If not, see <http://www.gnu.org/licenses/>.
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
  double offset; /**< the offset the data has been stored with */
  double gain; /**< the offset the data has been stored with */
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
      RAVE_FREE(img->image->heights);
    }
    if (img->image->array != NULL) {
      RAVE_FREE(img->image->array);
    }
    if (img->image->original != NULL) {
      RAVE_FREE(img->image->original);
    }
    RAVE_FREE(img->image);
  }
  img->image = NULL;
}

/**
 * Constructor
 */
static int RaveFmiImage_constructor(RaveCoreObject* obj)
{
  RaveFmiImage_t* this = (RaveFmiImage_t*)obj;
  this->offset = 0.0;
  this->gain = 1.0;
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
  this->offset = src->offset;
  this->gain = src->gain;
  this->image = new_image(1);
  this->attrs = RAVE_OBJECT_CLONE(src->attrs);
  if (this->image == NULL || this->attrs == NULL) {
    goto error;
  }
  canonize_image(src->image, this->image);
  return 1;
error:
  RaveFmiImageInternal_resetImage(this);
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
static int RaveFmiImageInternal_scanToFmiImage(PolarScan_t* scan, const char* quantity, RaveFmiImage_t* raveimg)
{
  int i = 0, j = 0;
  int result = 0;
  double gain, offset;
  PolarScanParam_t* param = NULL;
  FmiImage* image = NULL;

  RAVE_ASSERT((scan != NULL), "scan == NULL");
  RAVE_ASSERT((raveimg != NULL), "image == NULL");

  image = RaveFmiImage_getImage(raveimg);
  reset_image(image);
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

  gain = PolarScanParam_getGain(param);
  offset = PolarScanParam_getOffset(param);

  RaveFmiImage_setGain(raveimg, PolarScanParam_getGain(param));
  RaveFmiImage_setOffset(raveimg, PolarScanParam_getOffset(param));
  RaveFmiImage_setNodata(raveimg, PolarScanParam_getNodata(param));
  RaveFmiImage_setUndetect(raveimg, PolarScanParam_getUndetect(param));

  image->original_type = PolarScanParam_getDataType(param);

  for (j = 0; j < image->height; j++) {
    for (i = 0; i < image->width; i++) {
      double value = 0.0, bvalue = 0.0;;
      if (image->original_type == RaveDataType_CHAR || image->original_type == RaveDataType_UCHAR) {
        PolarScanParam_getValue(param, i, j, &value);
        bvalue = value;
      } else {
        /* This is slightly dangerous since there might actually be data that is 0 but hopefully it's not going to be an issue since we create a new range that is used in the array used for the bropo calculations */
        RaveValueType t = PolarScanParam_getValue(param, i, j, &value);
        if (t == RaveValueType_UNDETECT) {
          bvalue = 0;
        } else if (t == RaveValueType_NODATA) {
          bvalue = 255;
        } else {
          bvalue = ((value*gain + offset) - (-32.0))/0.5;
        }
      }
      put_pixel(image, i, j, 0, (Byte)(bvalue)); // why + 0.5 ?
      put_pixel_orig(image, i, j, 0, value);
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
  double nodata=255.0, undetect=0.0, gain=1.0, offset=0.0;
  RaveAttribute_t* attr = NULL;

  RAVE_ASSERT((field != NULL), "field == NULL");
  RAVE_ASSERT((image != NULL), "image == NULL");

  reset_image(image);
  image->width=RaveField_getXsize(field);
  image->height=RaveField_getYsize(field);
  image->channels=1;
  initialize_image(image);

  attr = RaveField_getAttribute(field, "what/gain");
  if (attr != NULL) {
    RaveAttribute_getDouble(attr, &gain);
  }
  RAVE_OBJECT_RELEASE(attr);
  attr = RaveField_getAttribute(field, "what/offset");
  if (attr != NULL) {
    RaveAttribute_getDouble(attr, &offset);
  }
  RAVE_OBJECT_RELEASE(attr);
  attr = RaveField_getAttribute(field, "what/nodata");
  if (attr != NULL) {
    RaveAttribute_getDouble(attr, &nodata);
  }
  RAVE_OBJECT_RELEASE(attr);
  attr = RaveField_getAttribute(field, "what/undetect");
  if (attr != NULL) {
    RaveAttribute_getDouble(attr, &undetect);
  }
  RAVE_OBJECT_RELEASE(attr);

  image->original_type = RaveField_getDataType(field);

  for (j = 0; j < image->height; j++) {
    for (i = 0; i < image->width; i++) {
      double value = 0.0, bvalue = 0.0;;

      if (image->original_type == RaveDataType_CHAR || image->original_type == RaveDataType_UCHAR) {
        RaveField_getValue(field, i, j, &value);
        bvalue = value;
      } else {
        /* This is slightly dangerous since there might actually be data that is 0 but hopefully it's not going to be an issue since we create a new range that is used in the array used for the bropo calculations */
        RaveField_getValue(field, i, j, &value);
        if (value == undetect) {
          bvalue = 0;
        } else if (value == nodata) {
          bvalue = 255;
        } else {
          bvalue = ((value*gain + offset) - (-32.0))/0.5;
        }
      }

      put_pixel(image, i, j, 0, (Byte)(bvalue)); // why + 0.5 ?
      put_pixel_orig(image, i, j, 0, value);
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
static PolarScan_t* RaveFmiImageInternal_fmiImageToScan(FmiImage* image, double offset, double gain, const char* quantity, int datatype)
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

  PolarScanParam_setGain(param, gain);
  PolarScanParam_setOffset(param, offset);
  PolarScanParam_setNodata(param, image->original_nodata);
  PolarScanParam_setUndetect(param, image->original_undetect);
  if (quantity != NULL) {
    PolarScanParam_setQuantity(param, quantity);
  } else {
    PolarScanParam_setQuantity(param, "DBZH");
  }

  if (datatype == 0) {
    if (!PolarScanParam_createData(param, image->width, image->height, image->original_type)) {
      RAVE_CRITICAL0("Failed to allocate memory for data");
      goto done;
    }
  } else {
    if (!PolarScanParam_createData(param, image->width, image->height, RaveDataType_UCHAR)) {
      RAVE_CRITICAL0("Failed to allocate memory for data");
      goto done;
    }
  }

  for (ray = 0; ray < image->height; ray++) {
    for (bin = 0; bin < image->width; bin++) {
      if (datatype != 2 && (image->original_type == RaveDataType_CHAR || image->original_type == RaveDataType_UCHAR || datatype == 1)) {
        PolarScanParam_setValue(param, bin, ray, (double)get_pixel(image, bin, ray, 0));
      } else {
        PolarScanParam_setValue(param, bin, ray, (double)get_pixel_orig(image, bin, ray, 0));
      }
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
static RaveField_t* RaveFmiImageInternal_fmiImageToField(FmiImage* image, int datatype)
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

  if (datatype == 0) {
    if (!RaveField_createData(field, image->width, image->height, image->original_type)) {
      RAVE_CRITICAL0("Failed to allocate memory for data");
      goto done;
    }
  } else {
    if (!RaveField_createData(field, image->width, image->height, RaveDataType_UCHAR)) {
      RAVE_CRITICAL0("Failed to allocate memory for data");
      goto done;
    }
  }

  for (ray = 0; ray < image->height; ray++) {
    for (bin = 0; bin < image->width; bin++) {
      if (datatype != 2 && (image->original_type == RaveDataType_CHAR || image->original_type == RaveDataType_UCHAR || datatype == 1)) {
        RaveField_setValue(field, bin, ray, (double)get_pixel(image, bin, ray, 0));
      } else {
        RaveField_setValue(field, bin, ray, (double)get_pixel_orig(image, bin, ray, 0));
      }
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
      self->image->channels = 1;
      initialize_image(self->image);
      result = 1;
    }
  }
  return result;
}

void RaveFmiImage_fill(RaveFmiImage_t* self, unsigned char v)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (self->image != NULL) {
    fill_image(self->image, (Byte)v);
    if (self->image->original != NULL) {
      int i;
      for (i = 0; i < self->image->volume; i++) {
        self->image->original[i] = (double)v;
      }
    }
  }
}

void RaveFmiImage_fillOriginal(RaveFmiImage_t* self, double v)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (self->image != NULL) {
    if (self->image->original != NULL) {
      int i;
      int nv = self->image->width*self->image->height*self->image->channels;
      for (i = 0; i < nv; i++) {
        self->image->original[i] = (double)v;
      }
    }
  }
}

FmiImage* RaveFmiImage_getImage(RaveFmiImage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->image;
}

PolarScan_t* RaveFmiImage_toPolarScan(RaveFmiImage_t* self, const char* quantity, int datatype)
{
  PolarScan_t* result = NULL;
  PolarScan_t* scan = NULL;
  RaveObjectList_t* attributes = NULL;

  int i = 0;
  int nrAttrs = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  scan = RaveFmiImageInternal_fmiImageToScan(&self->image[0], self->offset, self->gain, quantity, datatype);
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

RaveField_t* RaveFmiImage_toRaveField(RaveFmiImage_t* self, int datatype)
{
  RaveField_t* result = NULL;
  RaveField_t* field = NULL;
  RaveObjectList_t* attributes = NULL;

  int i = 0;
  int nrAttrs = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  field = RaveFmiImageInternal_fmiImageToField(&self->image[0], datatype);
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

void RaveFmiImage_setGain(RaveFmiImage_t* self, double gain)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->gain = gain;
}

double RaveFmiImage_getGain(RaveFmiImage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->gain;
}

void RaveFmiImage_setOffset(RaveFmiImage_t* self, double offset)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->offset = offset;
}

double RaveFmiImage_getOffset(RaveFmiImage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->offset;
}

void RaveFmiImage_setNodata(RaveFmiImage_t* self, double v)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->image->original_nodata = v;
}

double RaveFmiImage_getNodata(RaveFmiImage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->image->original_nodata;
}

void RaveFmiImage_setUndetect(RaveFmiImage_t* self, double v)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->image->original_undetect = v;
}

double RaveFmiImage_getUndetect(RaveFmiImage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->image->original_undetect;
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
    if (!RaveFmiImageInternal_scanToFmiImage(scan, quantity, image)) {
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
    if (!RaveFmiImageInternal_scanToFmiImage(scan, quantity, image)) {
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

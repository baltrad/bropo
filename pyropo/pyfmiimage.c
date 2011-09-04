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
 * Python version of the fmi image.
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-08-31
 */
#include "Python.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PYFMIIMAGE_MODULE   /**< to get correct part in pyfmiimage */
#include "pyfmiimage.h"

#include <rave.h>
#include <arrayobject.h>
#include "pypolarscan.h"
#include "pypolarvolume.h"
#include "pyravefield.h"
#include "pyrave_debug.h"
#include "rave_alloc.h"

/**
 * Debug this module
 */
PYRAVE_DEBUG_MODULE("_pyfmiimage");

/**
 * Sets a python exception and goto tag
 */
#define raiseException_gotoTag(tag, type, msg) \
{PyErr_SetString(type, msg); goto tag;}

/**
 * Sets python exception and returns NULL
 */
#define raiseException_returnNULL(type, msg) \
{PyErr_SetString(type, msg); return NULL;}

/**
 * Error object for reporting errors to the python interpreeter
 */
static PyObject *ErrorObject;

/// --------------------------------------------------------------------
/// FmiImage
/// --------------------------------------------------------------------
/*@{ FmiImage */
/**
 * Returns the native RaveFmiImage_t instance.
 * @param[in] pyfmiimage - the python fmi image instance
 * @returns the native fmi image instance.
 */
static RaveFmiImage_t*
PyFmiImage_GetNative(PyFmiImage* pyfmiimage)
{
  RAVE_ASSERT((pyfmiimage != NULL), "pyfmiimage == NULL");
  return RAVE_OBJECT_COPY(pyfmiimage->image);
}

/**
 * Creates a python fmi image from a native fmi image or will create an
 * initial native FmiImage if p is NULL.
 * @param[in] p - the native fmi image (or NULL)
 * @param[in] width - the width (only used if p != NULL and > 0)
 * @param[in] height - the height (only used if p != NULL and > 0)
 * @returns the python fmi image.
 */
static PyFmiImage*
PyFmiImage_New(RaveFmiImage_t* p, int width, int height)
{
  PyFmiImage* result = NULL;
  RaveFmiImage_t* cp = NULL;

  if (p == NULL) {
    cp = RAVE_OBJECT_NEW(&RaveFmiImage_TYPE);
    if (cp == NULL) {
      RAVE_CRITICAL0("Failed to allocate memory for fmi image.");
      raiseException_returnNULL(PyExc_MemoryError, "Failed to allocate memory for fmi image.");
    }
    if (width > 0 && height > 0) {
      if (!RaveFmiImage_initialize(cp, width, height)) {
        RAVE_CRITICAL0("Failed to initialize fmi image");
        raiseException_gotoTag(done, PyExc_MemoryError, "Failed to initialize fmi image");
      }
    }
  } else {
    cp = RAVE_OBJECT_COPY(p);
    result = RAVE_OBJECT_GETBINDING(p); // If p already have a binding, then this should only be increfed.
    if (result != NULL) {
      Py_INCREF(result);
    }
  }

  if (result == NULL) {
    result = PyObject_NEW(PyFmiImage, &PyFmiImage_Type);
    if (result != NULL) {
      PYRAVE_DEBUG_OBJECT_CREATED;
      result->image = RAVE_OBJECT_COPY(cp);
      RAVE_OBJECT_BIND(result->image, result);
    } else {
      RAVE_CRITICAL0("Failed to create PyFmiImage instance");
      raiseException_gotoTag(done, PyExc_MemoryError, "Failed to allocate memory for fmi image.");
    }
  }

done:
  RAVE_OBJECT_RELEASE(cp);
  return result;
}

/**
 * Deallocates the fmi image
 * @param[in] obj the object to deallocate.
 */
static void _pyfmiimage_dealloc(PyFmiImage* obj)
{
  /*Nothing yet*/
  if (obj == NULL) {
    return;
  }
  PYRAVE_DEBUG_OBJECT_DESTROYED;
  RAVE_OBJECT_UNBIND(obj->image, obj);
  RAVE_OBJECT_RELEASE(obj->image);
  PyObject_Del(obj);
}

/**
 * Creates a new instance of the fmi image
 * @param[in] self this instance.
 * @param[in] args arguments for creation empty or width,height.
 * @return the object on success, otherwise NULL
 */
static PyObject* _pyfmiimage_new(PyObject* self, PyObject* args)
{
  int width = 0, height = 0;
  if (!PyArg_ParseTuple(args, "|ii", &width, &height)) {
    return NULL;
  }
  return (PyObject*)PyFmiImage_New(NULL, width, height);
}

static PyObject* _pyfmiimage_fromRave(PyObject* self, PyObject* args)
{
  PyObject* inptr = NULL;
  RaveCoreObject* object = NULL;
  char* quantity = NULL;

  RaveFmiImage_t* fmiimage = NULL;
  PyFmiImage* result = NULL;

  if (!PyArg_ParseTuple(args, "O|s", &inptr, &quantity)) {
    return NULL;
  }

  if (PyPolarVolume_Check(inptr)) {
    object = (RaveCoreObject*)PyPolarVolume_GetNative((PyPolarVolume*)inptr);
    fmiimage = RaveFmiImage_fromPolarVolume((PolarVolume_t*)object, 0, quantity);
  } else if (PyPolarScan_Check(inptr)) {
    object = (RaveCoreObject*)PyPolarScan_GetNative((PyPolarScan*)inptr);
    fmiimage = RaveFmiImage_fromPolarScan((PolarScan_t*)object, quantity);
  } else if (PyRaveField_Check(inptr)) {
    object = (RaveCoreObject*)PyRaveField_GetNative((PyRaveField*)inptr);
    fmiimage = RaveFmiImage_fromRaveField((RaveField_t*)object);
  } else {
    raiseException_returnNULL(PyExc_TypeError,"fromRave can handle volumes, scans and rave fields");
  }

  if (fmiimage != NULL) {
    result = PyFmiImage_New(fmiimage, 0, 0);
  }

  RAVE_OBJECT_RELEASE(fmiimage);
  RAVE_OBJECT_RELEASE(object);

  return (PyObject*)result;
}

static PyObject* _pyfmiimage_fromRaveVolume(PyObject* self, PyObject* args)
{
  PyObject* inptr = NULL;
  RaveCoreObject* object = NULL;
  char* quantity = NULL;
  int scannr = 0;

  RaveFmiImage_t* fmiimage = NULL;
  PyFmiImage* result = NULL;

  if (!PyArg_ParseTuple(args, "Oi|s", &inptr, &scannr, &quantity)) {
    return NULL;
  }

  if (PyPolarVolume_Check(inptr)) {
    object = (RaveCoreObject*)PyPolarVolume_GetNative((PyPolarVolume*)inptr);
    fmiimage = RaveFmiImage_fromPolarVolume((PolarVolume_t*)object, scannr, quantity);
  } else {
    raiseException_returnNULL(PyExc_TypeError,"fromRaveVolume only handle volumes");
  }

  if (fmiimage != NULL) {
    result = PyFmiImage_New(fmiimage, 0, 0);
  }

  RAVE_OBJECT_RELEASE(fmiimage);
  RAVE_OBJECT_RELEASE(object);

  return (PyObject*)result;
}

/**
 * Adds an attribute to the image. Name of the attribute should be in format
 * ^(how|what|where)/[A-Za-z0-9_.]$. E.g how/something, what/sthis etc.
 * Currently, the only supported values are double, long, string.
 * @param[in] self - this instance
 * @param[in] args - bin index, ray index.
 * @returns true or false depending if it works.
 */
static PyObject* _pyfmiimage_addAttribute(PyFmiImage* self, PyObject* args)
{
  RaveAttribute_t* attr = NULL;
  char* name = NULL;
  PyObject* obj = NULL;
  PyObject* result = NULL;

  if (!PyArg_ParseTuple(args, "sO", &name, &obj)) {
    return NULL;
  }

  attr = RAVE_OBJECT_NEW(&RaveAttribute_TYPE);
  if (attr == NULL) {
    return NULL;
  }

  if (!RaveAttribute_setName(attr, name)) {
    raiseException_gotoTag(done, PyExc_MemoryError, "Failed to set name");
  }

  if (PyLong_Check(obj) || PyInt_Check(obj)) {
    long value = PyLong_AsLong(obj);
    RaveAttribute_setLong(attr, value);
  } else if (PyFloat_Check(obj)) {
    double value = PyFloat_AsDouble(obj);
    RaveAttribute_setDouble(attr, value);
  } else if (PyString_Check(obj)) {
    char* value = PyString_AsString(obj);
    if (!RaveAttribute_setString(attr, value)) {
      raiseException_gotoTag(done, PyExc_AttributeError, "Failed to set string value");
    }
  } else if (PyArray_Check(obj)) {
    PyArrayObject* arraydata = (PyArrayObject*)obj;
    if (PyArray_NDIM(arraydata) != 1) {
      raiseException_gotoTag(done, PyExc_AttributeError, "Only allowed attribute arrays are 1-dimensional");
    }
    if (!RaveAttribute_setArrayFromData(attr, PyArray_DATA(arraydata), PyArray_DIM(arraydata, 0), translate_pyarraytype_to_ravetype(PyArray_TYPE(arraydata)))) {
      raiseException_gotoTag(done, PyExc_AttributeError, "Failed to set array data");
    }
  } else {
    raiseException_gotoTag(done, PyExc_AttributeError, "Unsupported data type");
  }

  if (!RaveFmiImage_addAttribute(self->image, attr)) {
    raiseException_gotoTag(done, PyExc_AttributeError, "Failed to add attribute");
  }

  result = PyBool_FromLong(1);
done:
  RAVE_OBJECT_RELEASE(attr);
  return result;
}

/**
 * Returns an attribute with the specified name
 * @param[in] self - this instance
 * @param[in] args - name
 * @returns the attribute value for the name
 */
static PyObject* _pyfmiimage_getAttribute(PyFmiImage* self, PyObject* args)
{
  RaveAttribute_t* attribute = NULL;
  char* name = NULL;
  PyObject* result = NULL;
  if (!PyArg_ParseTuple(args, "s", &name)) {
    return NULL;
  }

  attribute = RaveFmiImage_getAttribute(self->image, name);
  if (attribute != NULL) {
    RaveAttribute_Format format = RaveAttribute_getFormat(attribute);
    if (format == RaveAttribute_Format_Long) {
      long value = 0;
      RaveAttribute_getLong(attribute, &value);
      result = PyLong_FromLong(value);
    } else if (format == RaveAttribute_Format_Double) {
      double value = 0.0;
      RaveAttribute_getDouble(attribute, &value);
      result = PyFloat_FromDouble(value);
    } else if (format == RaveAttribute_Format_String) {
      char* value = NULL;
      RaveAttribute_getString(attribute, &value);
      result = PyString_FromString(value);
    } else if (format == RaveAttribute_Format_LongArray) {
      long* value = NULL;
      int len = 0;
      int i = 0;
      npy_intp dims[1];
      RaveAttribute_getLongArray(attribute, &value, &len);
      dims[0] = len;
      result = PyArray_SimpleNew(1, dims, PyArray_LONG);
      for (i = 0; i < len; i++) {
        *((long*) PyArray_GETPTR1(result, i)) = value[i];
      }
    } else if (format == RaveAttribute_Format_DoubleArray) {
      double* value = NULL;
      int len = 0;
      int i = 0;
      npy_intp dims[1];
      RaveAttribute_getDoubleArray(attribute, &value, &len);
      dims[0] = len;
      result = PyArray_SimpleNew(1, dims, PyArray_DOUBLE);
      for (i = 0; i < len; i++) {
        *((double*) PyArray_GETPTR1(result, i)) = value[i];
      }
    } else {
      RAVE_CRITICAL1("Undefined format on requested attribute %s", name);
      raiseException_gotoTag(done, PyExc_AttributeError, "Undefined attribute");
    }
  } else {
    raiseException_gotoTag(done, PyExc_AttributeError, "No such attribute");
  }
done:
  RAVE_OBJECT_RELEASE(attribute);
  return result;
}

/**
 * Returns a list of attribute names
 * @param[in] self - this instance
 * @param[in] args - N/A
 * @returns a list of attribute names
 */
static PyObject* _pyfmiimage_getAttributeNames(PyFmiImage* self, PyObject* args)
{
  RaveList_t* list = NULL;
  PyObject* result = NULL;
  int n = 0;
  int i = 0;

  list = RaveFmiImage_getAttributeNames(self->image);
  if (list == NULL) {
    raiseException_returnNULL(PyExc_MemoryError, "Could not get attribute names");
  }
  n = RaveList_size(list);
  result = PyList_New(0);
  for (i = 0; result != NULL && i < n; i++) {
    char* name = RaveList_get(list, i);
    if (name != NULL) {
      PyObject* pynamestr = PyString_FromString(name);
      if (pynamestr == NULL) {
        goto fail;
      }
      if (PyList_Append(result, pynamestr) != 0) {
        Py_DECREF(pynamestr);
        goto fail;
      }
      Py_DECREF(pynamestr);
    }
  }
  RaveList_freeAndDestroy(&list);
  return result;
fail:
  RaveList_freeAndDestroy(&list);
  Py_XDECREF(result);
  return NULL;
}

/**
 * Returns the rave polar scan
 * @param[in] self - self
 * @param[in] args - a string specifying quantity. Default is DBZH.
 * @return the rave polar scan
 */
static PyObject* _pyfmiimage_toPolarScan(PyFmiImage* self, PyObject* args)
{
  char* quantity = NULL;
  PolarScan_t* scan = NULL;
  PyObject* result = NULL;

  if (!PyArg_ParseTuple(args, "|s", &quantity)) {
    return NULL;
  }
  scan = RaveFmiImage_toPolarScan(self->image, quantity);
  if (scan != NULL) {
    result = (PyObject*)PyPolarScan_New(scan);
  }
  RAVE_OBJECT_RELEASE(scan);
  return result;
}

/**
 * Returns the rave field
 * @param[in] self - self
 * @param[in] args - N/A
 * @return the rave field
 */
static PyObject* _pyfmiimage_toRaveField(PyFmiImage* self, PyObject* args)
{
  RaveField_t* field = NULL;
  PyObject* result = NULL;

  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  field = RaveFmiImage_toRaveField(self->image);
  if (field != NULL) {
    result = (PyObject*)PyRaveField_New(field);
  }
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/*@} End Of FmiImage */

/**
 * All methods a fmi image can have
 */
static struct PyMethodDef _pyfmiimage_methods[] =
{
  {"offset", NULL},
  {"gain", NULL},
  {"addAttribute", (PyCFunction)_pyfmiimage_addAttribute, 1},
  {"getAttribute", (PyCFunction)_pyfmiimage_getAttribute, 1},
  {"getAttributeNames", (PyCFunction)_pyfmiimage_getAttributeNames, 1},
  {"toPolarScan", (PyCFunction)_pyfmiimage_toPolarScan, 1},
  {"toRaveField", (PyCFunction)_pyfmiimage_toRaveField, 1},
  {NULL, NULL} /* sentinel */
};

/**
 * Returns the specified attribute in the fmi image
 */
static PyObject* _pyfmiimage_getattr(PyFmiImage* self, char* name)
{
  PyObject* res = NULL;
  if (strcmp("offset", name) == 0) {
    return PyFloat_FromDouble(RaveFmiImage_getOffset(self->image));
  } else if (strcmp("gain", name) == 0) {
    return PyFloat_FromDouble(RaveFmiImage_getGain(self->image));
  }

  res = Py_FindMethod(_pyfmiimage_methods, (PyObject*) self, name);
  if (res)
    return res;

  PyErr_Clear();
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

/**
 * Returns the specified attribute in the polar volume
 */
static int _pyfmiimage_setattr(PyFmiImage* self, char* name, PyObject* val)
{
  int result = -1;
  if (name == NULL) {
    goto done;
  }

  if (strcmp("offset", name) == 0) {
    double v = 0.0;
    if (PyFloat_Check(val)) {
      v = PyFloat_AsDouble(val);
    } else if (PyLong_Check(val)) {
      v = PyLong_AsDouble(val);
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "offset is a number");
    }
    RaveFmiImage_setOffset(self->image, v);
  } else if (strcmp("gain", name) == 0) {
    double v = 0.0;
    if (PyFloat_Check(val)) {
      v = PyFloat_AsDouble(val);
    } else if (PyLong_Check(val)) {
      v = PyLong_AsDouble(val);
    } else {
      raiseException_gotoTag(done, PyExc_TypeError, "offset is a number");
    }
    RaveFmiImage_setGain(self->image, v);
  } else {
    raiseException_gotoTag(done, PyExc_AttributeError, name);
  }

  result = 0;
done:
  return result;
}

/*@} End of Fmi Image */

/// --------------------------------------------------------------------
/// Type definitions
/// --------------------------------------------------------------------
/*@{ Type definitions */
PyTypeObject PyFmiImage_Type =
{
  PyObject_HEAD_INIT(NULL)0, /*ob_size*/
  "FmiImageCore", /*tp_name*/
  sizeof(PyFmiImage), /*tp_size*/
  0, /*tp_itemsize*/
  /* methods */
  (destructor)_pyfmiimage_dealloc, /*tp_dealloc*/
  0, /*tp_print*/
  (getattrfunc)_pyfmiimage_getattr, /*tp_getattr*/
  (setattrfunc)_pyfmiimage_setattr, /*tp_setattr*/
  0, /*tp_compare*/
  0, /*tp_repr*/
  0, /*tp_as_number */
  0,
  0, /*tp_as_mapping */
  0 /*tp_hash*/
};
/*@} End of Type definitions */

/// --------------------------------------------------------------------
/// Module setup
/// --------------------------------------------------------------------
/*@{ Module setup */
static PyMethodDef functions[] = {
  {"new", (PyCFunction)_pyfmiimage_new, 1},
  {"fromRave", (PyCFunction)_pyfmiimage_fromRave, 1},
  {"fromRaveVolume", (PyCFunction)_pyfmiimage_fromRaveVolume, 1},
  {NULL,NULL} /*Sentinel*/
};

/**
 * Initializes polar volume.
 */
void init_fmiimage(void)
{
  PyObject *module=NULL,*dictionary=NULL;
  static void *PyFmiImage_API[PyFmiImage_API_pointers];
  PyObject *c_api_object = NULL;
  PyFmiImage_Type.ob_type = &PyType_Type;

  module = Py_InitModule("_fmiimage", functions);
  if (module == NULL) {
    return;
  }
  PyFmiImage_API[PyFmiImage_Type_NUM] = (void*)&PyFmiImage_Type;
  PyFmiImage_API[PyFmiImage_GetNative_NUM] = (void *)PyFmiImage_GetNative;
  PyFmiImage_API[PyFmiImage_New_NUM] = (void*)PyFmiImage_New;

  c_api_object = PyCObject_FromVoidPtr((void *)PyFmiImage_API, NULL);

  if (c_api_object != NULL) {
    PyModule_AddObject(module, "_C_API", c_api_object);
  }

  dictionary = PyModule_GetDict(module);
  ErrorObject = PyString_FromString("_fmiimage.error");
  if (ErrorObject == NULL || PyDict_SetItemString(dictionary, "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _fmiimage.error");
  }

  import_array();
  import_pypolarscan();
  import_pypolarvolume();
  import_pyravefield();
  PYRAVE_DEBUG_INITIALIZE;
}
/*@} End of Module setup */

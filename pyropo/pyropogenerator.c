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
 * Python API to the ropo functions
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-08-31
 */
#include "pyropo_compat.h"
#include "Python.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PYROPOGENERATOR_MODULE   /**< to get correct part in pyropogenerator */
#include "pyropogenerator.h"

#include "pyfmiimage.h"
#include "pyrave_debug.h"
#include "rave_alloc.h"

/**
 * Debug this module
 */
PYRAVE_DEBUG_MODULE("_ropogenerator");

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
/// RopoGenerator
/// --------------------------------------------------------------------
/*@{ RopoGenerator */
/**
 * Returns the native RaveRopoGenerator_t instance.
 * @param[in] ropogenerator - the python ropo generator instance
 * @returns the native RaveRopoGenerator_t instance.
 */
static RaveRopoGenerator_t*
PyRopoGenerator_GetNative(PyRopoGenerator* ropogenerator)
{
  RAVE_ASSERT((ropogenerator != NULL), "ropogenerator == NULL");
  return RAVE_OBJECT_COPY(ropogenerator->generator);
}

/**
 * Creates a python ropo generator from a native ropo generator or will create an
 * initial native ropo generator if p is NULL.
 * @param[in] p - the native ropo generator (or NULL)
 * @param[in] image - the fmi image (only used if p != NULL)
 * @returns the python fmi image.
 */
static PyRopoGenerator*
PyRopoGenerator_New(RaveRopoGenerator_t* p, RaveFmiImage_t* image)
{
  PyRopoGenerator* result = NULL;
  RaveRopoGenerator_t* cp = NULL;

  if (p == NULL) {
    cp = RAVE_OBJECT_NEW(&RaveRopoGenerator_TYPE);
    if (cp == NULL) {
      RAVE_CRITICAL0("Failed to allocate memory for ropo generator.");
      raiseException_returnNULL(PyExc_MemoryError, "Failed to allocate memory for ropo generator.");
    }

    if (image != NULL) {
      RaveRopoGenerator_setImage(cp, image);
    }
  } else {
    cp = RAVE_OBJECT_COPY(p);
    result = RAVE_OBJECT_GETBINDING(p); // If p already have a binding, then this should only be increfed.
    if (result != NULL) {
      Py_INCREF(result);
    }
  }

  if (result == NULL) {
    result = PyObject_NEW(PyRopoGenerator, &PyRopoGenerator_Type);
    if (result != NULL) {
      PYRAVE_DEBUG_OBJECT_CREATED;
      result->generator = RAVE_OBJECT_COPY(cp);
      RAVE_OBJECT_BIND(result->generator, result);
    } else {
      RAVE_CRITICAL0("Failed to create PyRopoGenerator instance");
      raiseException_gotoTag(done, PyExc_MemoryError, "Failed to allocate memory for ropo generator.");
    }
  }

done:
  RAVE_OBJECT_RELEASE(cp);
  return result;
}

/**
 * Deallocates the ropo generator
 * @param[in] obj the object to deallocate.
 */
static void _pyropogenerator_dealloc(PyRopoGenerator* obj)
{
  /*Nothing yet*/
  if (obj == NULL) {
    return;
  }
  PYRAVE_DEBUG_OBJECT_DESTROYED;
  RAVE_OBJECT_UNBIND(obj->generator, obj);
  RAVE_OBJECT_RELEASE(obj->generator);
  PyObject_Del(obj);
}

/**
 * Creates a new instance of the ropo generator
 * @param[in] self this instance.
 * @param[in] args arguments for creation or a rave fmi image.
 * @return the object on success, otherwise NULL
 */
static PyObject* _pyropogenerator_new(PyObject* self, PyObject* args)
{
  PyObject* inptr = NULL;

  if (!PyArg_ParseTuple(args, "|O", &inptr)) {
    return NULL;
  }
  if (inptr != NULL && !PyFmiImage_Check(inptr)) {
    raiseException_returnNULL(PyExc_TypeError, "Generator only takes FmiImageCore instances");
  }
  if (inptr != NULL) {
    return (PyObject*)PyRopoGenerator_New(NULL, ((PyFmiImage*)inptr)->image);
  } else {
    return (PyObject*)PyRopoGenerator_New(NULL, NULL);
  }
}

/**
 * Returns the image that this generator is run on.
 * @param[in] self - self
 * @param[in] args - N/A
 * @return the PyFmiImage on success otherwise NULL
 */
static PyObject* _pyropogenerator_getImage(PyRopoGenerator* self, PyObject* args)
{
  RaveFmiImage_t* image = NULL;
  PyObject* result = NULL;
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  image = RaveRopoGenerator_getImage(self->generator);
  if (image != NULL) {
    result = (PyObject*)PyFmiImage_New(image,0,0);
  }
  RAVE_OBJECT_RELEASE(image);

  if (result != NULL) {
    return result;
  }

  Py_RETURN_NONE;
}

/**
 * Sets the image this generator should be run on.
 * @param[in] self - self
 * @param[in] args - the PyFmiImage object
 * @return NONE on success otherwise NULL
 */
static PyObject* _pyropogenerator_setImage(PyRopoGenerator* self, PyObject* args)
{
  PyObject* inptr = NULL;
  if (!PyArg_ParseTuple(args, "O", &inptr)) {
    return NULL;
  }
  if (!PyFmiImage_Check(inptr)) {
    raiseException_returnNULL(PyExc_TypeError, "Generator can only be set with fmi images");
  }
  RaveRopoGenerator_setImage(self->generator, ((PyFmiImage*)inptr)->image);
  Py_RETURN_NONE;
}

/**
 * See \ref RaveRopoGenerator_threshold
 * @param[in] self - self
 * @param[in] args - i (threshold)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_threshold(PyRopoGenerator* self, PyObject* args)
{
  int threshold = 0;
  if (!PyArg_ParseTuple(args, "i", &threshold)) {
    return NULL;
  }
  RaveRopoGenerator_threshold(self->generator, threshold);
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_speck
 * @param[in] self - self
 * @param[in] args - ii (minDbz, maxA)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_speck(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, maxA = 0;
  if (!PyArg_ParseTuple(args, "ii", &minDbz, &maxA)) {
    return NULL;
  }
  if (!RaveRopoGenerator_speck(self->generator, minDbz, maxA)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run speck");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_speckNormOld
 * @param[in] self - self
 * @param[in] args - iii (minDbz, maxA, maxN)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_speckNormOld(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, maxA = 0, maxN = 0;
  if (!PyArg_ParseTuple(args, "iii", &minDbz, &maxA, &maxN)) {
    return NULL;
  }
  if (!RaveRopoGenerator_speckNormOld(self->generator, minDbz, maxA, maxN)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run speckNormOld");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_emitter
 * @param[in] self - self
 * @param[in] args - ii (minDbz, length)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_emitter(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, length = 0;
  if (!PyArg_ParseTuple(args, "ii", &minDbz, &length)) {
    return NULL;
  }
  if (!RaveRopoGenerator_emitter(self->generator, minDbz, length)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run emitter");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_emitter2
 * @param[in] self - self
 * @param[in] args - iii (minDbz, length, width)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_emitter2(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, length = 0, width = 0;
  if (!PyArg_ParseTuple(args, "iii", &minDbz, &length, &width)) {
    return NULL;
  }
  if (!RaveRopoGenerator_emitter2(self->generator, minDbz, length, width)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run emitter2");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_clutter
 * @param[in] self - self
 * @param[in] args - ii (minDbz, maxCompactness)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_clutter(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, maxCompactness = 0;
  if (!PyArg_ParseTuple(args, "ii", &minDbz, &maxCompactness)) {
    return NULL;
  }
  if (!RaveRopoGenerator_clutter(self->generator, minDbz, maxCompactness)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run clutter");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_clutter2
 * @param[in] self - self
 * @param[in] args - ii (minDbz, maxSmoothness)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_clutter2(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, maxSmoothness = 0;
  if (!PyArg_ParseTuple(args, "ii", &minDbz, &maxSmoothness)) {
    return NULL;
  }
  if (!RaveRopoGenerator_clutter2(self->generator, minDbz, maxSmoothness)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run clutter2");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_softcut
 * @param[in] self - self
 * @param[in] args - iii (maxDbz, r, r2)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_softcut(PyRopoGenerator* self, PyObject* args)
{
  int maxDbz = 0, r = 0, r2 = 0;
  if (!PyArg_ParseTuple(args, "iii", &maxDbz, &r, &r2)) {
    return NULL;
  }
  if (!RaveRopoGenerator_softcut(self->generator, maxDbz, r, r2)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run softcut");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_biomet
 * @param[in] self - self
 * @param[in] args - iiii (maxDbz, dbzDelta, maxAlt, altDelta)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_biomet(PyRopoGenerator* self, PyObject* args)
{
  int maxDbz = 0, dbzDelta = 0, maxAlt = 0, altDelta;
  if (!PyArg_ParseTuple(args, "iiii", &maxDbz, &dbzDelta, &maxAlt, &altDelta)) {
    return NULL;
  }
  if (!RaveRopoGenerator_biomet(self->generator, maxDbz, dbzDelta, maxAlt, altDelta)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run biomet");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_ship
 * @param[in] self - self
 * @param[in] args - ii (minRelDbz, minA)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_ship(PyRopoGenerator* self, PyObject* args)
{
  int minRelDbz = 0, minA = 0;
  if (!PyArg_ParseTuple(args, "ii", &minRelDbz, &minA)) {
    return NULL;
  }
  if (!RaveRopoGenerator_ship(self->generator, minRelDbz, minA)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run ship");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_sun
 * @param[in] self - self
 * @param[in] args - iii (minDbz, minLength, maxThickness)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_sun(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, minLength = 0, maxThickness = 0;
  if (!PyArg_ParseTuple(args, "iii", &minDbz, &minLength, &maxThickness)) {
    return NULL;
  }
  if (!RaveRopoGenerator_sun(self->generator, minDbz, minLength, maxThickness)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run sun");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_sun2
 * @param[in] self - self
 * @param[in] args - iiiii (minDbz, minLength, maxThickness,azimuth,elevation)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_sun2(PyRopoGenerator* self, PyObject* args)
{
  int minDbz = 0, minLength = 0, maxThickness = 0, azimuth = 0, elevation = 0;
  if (!PyArg_ParseTuple(args, "iiiii", &minDbz, &minLength, &maxThickness, &azimuth, &elevation)) {
    return NULL;
  }
  if (!RaveRopoGenerator_sun2(self->generator, minDbz, minLength, maxThickness, azimuth, elevation)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to run sun2");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_classify
 * @param[in] self - self
 * @param[in] args - N/A
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_classify(PyRopoGenerator* self, PyObject* args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  if (!RaveRopoGenerator_classify(self->generator)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to classify detector sequence");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_declassify
 * @param[in] self - self
 * @param[in] args - N/A
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_declassify(PyRopoGenerator* self, PyObject* args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  RaveRopoGenerator_declassify(self->generator);
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_restore
 * @param[in] self - self
 * @param[in] args - ii (threshold)
 * @return The restored PyFmiImage on success otherwise NULL
 */
static PyObject* _pyropogenerator_restore(PyRopoGenerator* self, PyObject* args)
{
  RaveFmiImage_t* image = NULL;
  PyObject* result = NULL;
  int threshold = 0;
  if (!PyArg_ParseTuple(args, "i", &threshold)) {
    return NULL;
  }
  image = RaveRopoGenerator_restore(self->generator, threshold);
  if (image == NULL) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to classify detector sequence");
  }
  result = (PyObject*)PyFmiImage_New(image, 0, 0);
  RAVE_OBJECT_RELEASE(image);
  return result;
}

/**
 * See \ref RaveRopoGenerator_restore2
 * @param[in] self - self
 * @param[in] args - ii (threshold)
 * @return The restored PyFmiImage on success otherwise NULL
 */
static PyObject* _pyropogenerator_restore2(PyRopoGenerator* self, PyObject* args)
{
  RaveFmiImage_t* image = NULL;
  PyObject* result = NULL;
  int threshold = 0;
  if (!PyArg_ParseTuple(args, "i", &threshold)) {
    return NULL;
  }
  image = RaveRopoGenerator_restore2(self->generator, threshold);
  if (image == NULL) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to classify detector sequence");
  }
  result = (PyObject*)PyFmiImage_New(image, 0, 0);
  RAVE_OBJECT_RELEASE(image);
  return result;
}

/**
 * See \ref RaveRopoGenerator_restoreSelf
 * @param[in] self - self
 * @param[in] args - i (threshold)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_restoreSelf(PyRopoGenerator* self, PyObject* args)
{
  int threshold = 0;
  if (!PyArg_ParseTuple(args, "i", &threshold)) {
    return NULL;
  }
  if (!RaveRopoGenerator_restoreSelf(self->generator, threshold)) {
    raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to restore self");
  }
  return (PyObject*)PyRopoGenerator_New(self->generator, NULL);
}

/**
 * See \ref RaveRopoGenerator_getProbabilityFieldCount
 * @param[in] self - self
 * @param[in] args - N/A
 * @return the number of probability fields
 */
static PyObject* _pyropogenerator_getProbabilityFieldCount(PyRopoGenerator* self, PyObject* args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  return PyLong_FromLong((int)RaveRopoGenerator_getProbabilityFieldCount(self->generator));
}

/**
 * See \ref RaveRopoGenerator_getProbabilityField
 * @param[in] self - self
 * @param[in] args - i (index)
 * @return self on success otherwise NULL
 */
static PyObject* _pyropogenerator_getProbabilityField(PyRopoGenerator* self, PyObject* args)
{
  RaveFmiImage_t* image = NULL;
  PyObject* result = NULL;

  int index = 0;
  if (!PyArg_ParseTuple(args, "i", &index)) {
    return NULL;
  }
  image = RaveRopoGenerator_getProbabilityField(self->generator, index);
  if (image == NULL) {
    raiseException_returnNULL(PyExc_IndexError, "Failed to return probability field at specified index");
  }
  result = (PyObject*)PyFmiImage_New(image, 0, 0);
  RAVE_OBJECT_RELEASE(image);
  return result;
}

/**
 * All methods a ropo generator can have
 */
static struct PyMethodDef _pyropogenerator_methods[] =
{
  {"classification", NULL, METH_VARARGS},
  {"markers", NULL, METH_VARARGS},
  {"getImage", (PyCFunction)_pyropogenerator_getImage, 1},
  {"setImage", (PyCFunction)_pyropogenerator_setImage, 1},
  {"threshold", (PyCFunction)_pyropogenerator_threshold, 1},
  {"speck", (PyCFunction)_pyropogenerator_speck, 1},
  {"speckNormOld", (PyCFunction)_pyropogenerator_speckNormOld, 1},
  {"emitter", (PyCFunction)_pyropogenerator_emitter, 1},
  {"emitter2", (PyCFunction)_pyropogenerator_emitter2, 1},
  {"clutter", (PyCFunction)_pyropogenerator_clutter, 1},
  {"clutter2", (PyCFunction)_pyropogenerator_clutter2, 1},
  {"softcut", (PyCFunction)_pyropogenerator_softcut, 1},
  {"biomet", (PyCFunction)_pyropogenerator_biomet, 1},
  {"ship", (PyCFunction)_pyropogenerator_ship, 1},
  {"sun", (PyCFunction)_pyropogenerator_sun, 1},
  {"sun2", (PyCFunction)_pyropogenerator_sun2, 1},
  {"classify", (PyCFunction)_pyropogenerator_classify, 1},
  {"declassify", (PyCFunction)_pyropogenerator_declassify, 1},
  {"restore", (PyCFunction)_pyropogenerator_restore, 1},
  {"restore2", (PyCFunction)_pyropogenerator_restore2, 1},
  {"restoreSelf", (PyCFunction)_pyropogenerator_restoreSelf, 1},
  {"getProbabilityFieldCount", (PyCFunction)_pyropogenerator_getProbabilityFieldCount, 1},
  {"getProbabilityField", (PyCFunction)_pyropogenerator_getProbabilityField, 1},
  {NULL, NULL} /* sentinel */
};

/**
 * Returns the specified attribute in the fmi image
 */
static PyObject* _pyropogenerator_getattro(PyRopoGenerator* self, PyObject* name)
{
  PyObject* res = NULL;

  if (PY_COMPARE_STRING_WITH_ATTRO_NAME("classification", name) == 0) {
	RaveFmiImage_t* image = NULL;
    image = RaveRopoGenerator_getClassification(self->generator);
    if (image == NULL) {
      raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to get classification");
    }
    res = (PyObject*)PyFmiImage_New(image,0,0);
    RAVE_OBJECT_RELEASE(image);
    return res;
  } else if (PY_COMPARE_STRING_WITH_ATTRO_NAME("markers", name) == 0) {
	RaveFmiImage_t* image = NULL;
	image = RaveRopoGenerator_getMarkers(self->generator);
	if (image == NULL) {
	  raiseException_returnNULL(PyExc_RuntimeWarning, "Failed to get classification");
	}
	res = (PyObject*)PyFmiImage_New(image,0,0);
	RAVE_OBJECT_RELEASE(image);
	return res;
  }
  return PyObject_GenericGetAttr((PyObject*)self, name);
}

/**
 * Returns the specified attribute in the polar volume
 */
static int _pyropogenerator_setattro(PyRopoGenerator* self, PyObject* name, PyObject* val)
{
  int result = -1;
  if (name == NULL) {
    goto done;
  }

  raiseException_gotoTag(done, PyExc_AttributeError, PY_RAVE_ATTRO_NAME_TO_STRING(name));

  result = 0;
done:
  return result;
}

/*@} End of Fmi Image */

/// --------------------------------------------------------------------
/// Type definitions
/// --------------------------------------------------------------------
/*@{ Type definitions */
PyTypeObject PyRopoGenerator_Type =
{
  PyVarObject_HEAD_INIT(NULL, 0) /*ob_size*/
  "RopoGeneratorCore", /*tp_name*/
  sizeof(PyRopoGenerator), /*tp_size*/
  0, /*tp_itemsize*/
  /* methods */
  (destructor)_pyropogenerator_dealloc, /*tp_dealloc*/
  0, /*tp_print*/
  (getattrfunc)0,               /*tp_getattr*/
  (setattrfunc)0,               /*tp_setattr*/
  0,                            /*tp_compare*/
  0,                            /*tp_repr*/
  0,                            /*tp_as_number */
  0,
  0,                            /*tp_as_mapping */
  0,                            /*tp_hash*/
  (ternaryfunc)0,               /*tp_call*/
  (reprfunc)0,                  /*tp_str*/
  (getattrofunc)_pyropogenerator_getattro, /*tp_getattro*/
  (setattrofunc)_pyropogenerator_setattro, /*tp_setattro*/
  0,                            /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT, /*tp_flags*/
  0,                            /*tp_doc*/
  (traverseproc)0,              /*tp_traverse*/
  (inquiry)0,                   /*tp_clear*/
  0,                            /*tp_richcompare*/
  0,                            /*tp_weaklistoffset*/
  0,                            /*tp_iter*/
  0,                            /*tp_iternext*/
  _pyropogenerator_methods,              /*tp_methods*/
  0,                            /*tp_members*/
  0,                            /*tp_getset*/
  0,                            /*tp_base*/
  0,                            /*tp_dict*/
  0,                            /*tp_descr_get*/
  0,                            /*tp_descr_set*/
  0,                            /*tp_dictoffset*/
  0,                            /*tp_init*/
  0,                            /*tp_alloc*/
  0,                            /*tp_new*/
  0,                            /*tp_free*/
  0,                            /*tp_is_gc*/
};
/*@} End of Type definitions */

#ifdef KALLE
/*@{ Functions */
static void _pyropointernal_update_classification(FmiImage *prob,FmiImage *master_prob,FmiImage *mark,Byte marker)
{
  register int i;
  for (i = 0; i < prob->volume;i++) {
    if (prob->array[i] >= master_prob->array[i]) {
      if (mark != NULL) {
        mark->array[i]=marker;
      }
      master_prob->array[i]=prob->array[i];
    }
  }
}

static PyObject* _pyropo_speck(PyObject* self, PyObject* args)
{
  PyObject* inptr = NULL;
  PyObject* result = NULL;
  PyFmiImage* image = NULL;
  RaveFmiImage_t* prob = NULL;
  FmiImage* sourceSweep = NULL; /* Do not release, internal memory */
  FmiImage* probSweep = NULL;   /* Do not release, internal memory */

  int sweep = 0;
  int intensity = 0, sz = 0; /* min DBZ, max area */
  if (!PyArg_ParseTuple(args, "Oii|i", &inptr, &intensity, &sz, &sweep)) {
    return NULL;
  }

  if (!PyFmiImage_Check(inptr)) {
    raiseException_returnNULL(PyExc_TypeError, "speck takes FmiImages as input");
  }
  image = (PyFmiImage*)inptr;
  sourceSweep = RaveFmiImage_getSweep(image->image, sweep);
  if (sourceSweep == NULL) {
    raiseException_returnNULL(PyExc_TypeError, "Input fmi image does not contain that sweep");
  }

  prob = RaveFmiImage_new(1);
  if (prob == NULL) {
    return NULL;
  }
  probSweep = RaveFmiImage_getSweep(prob, 0);
  detect_specks(sourceSweep, probSweep, intensity, histogram_area);
  semisigmoid_image(probSweep, sz);
  invert_image(probSweep);
  translate_intensity(probSweep, 255, 0);

  result = (PyObject*)PyFmiImage_New(prob, 0);
  RAVE_OBJECT_RELEASE(prob);
  return result;
}

static PyObject* _pyropo_emitter(PyObject* self, PyObject* args)
{
  PyObject* inptr = NULL;
  PyObject* result = NULL;
  PyFmiImage* image = NULL;
  RaveFmiImage_t* prob = NULL;
  FmiImage* sourceSweep = NULL; /* Do not release, internal memory */
  FmiImage* probSweep = NULL;   /* Do not release, internal memory */

  int sweep = 0;
  int intensity = 0, sz = 0; /* min DBZ, max area */
  if (!PyArg_ParseTuple(args, "Oii|i", &inptr, &intensity, &sz, &sweep)) {
    return NULL;
  }

  if (!PyFmiImage_Check(inptr)) {
    raiseException_returnNULL(PyExc_TypeError, "speck takes FmiImages as input");
  }
  image = (PyFmiImage*)inptr;
  sourceSweep = RaveFmiImage_getSweep(image->image, sweep);
  if (sourceSweep == NULL) {
    raiseException_returnNULL(PyExc_TypeError, "Input fmi image does not contain that sweep");
  }

  prob = RaveFmiImage_new(1);
  if (prob == NULL) {
    return NULL;
  }
  probSweep = RaveFmiImage_getSweep(prob, 0);
  detect_emitters(sourceSweep, probSweep, intensity, sz);

  result = (PyObject*)PyFmiImage_New(prob, 0);
  RAVE_OBJECT_RELEASE(prob);
  return result;
}

static PyObject* _pyropo_restore(PyObject* self, PyObject* args)
{
  PyObject* sweepptr = NULL;
  PyObject* inprobabilityptr = NULL;
  PyFmiImage* sweep = NULL;
  PyFmiImage* probability = NULL;
  RaveFmiImage_t* resultimage = NULL;
  PyObject* result = NULL;
  FmiImage* sourceSweep = NULL; /* Do not release, internal memory */
  FmiImage* probabilitySweep = NULL; /* Do not release, internal memory */
  int threshold = 255;
  int sweepnr = 0;

  if (!PyArg_ParseTuple(args, "OOi|i", &sweepptr, &inprobabilityptr, &threshold, &sweepnr)) {
    return NULL;
  }

  if (!PyFmiImage_Check(sweepptr) || !PyFmiImage_Check(inprobabilityptr)) {
    raiseException_returnNULL(PyExc_TypeError, "restore takes sweep, probability field and threshold as input");
  }
  sweep = (PyFmiImage*)sweepptr;
  probability = (PyFmiImage*)inprobabilityptr;

  sourceSweep = RaveFmiImage_getSweep(sweep->image, sweepnr);
  if (sourceSweep == NULL) {
    raiseException_returnNULL(PyExc_TypeError, "Input fmi image does not contain that sweep");
  }
  probabilitySweep = RaveFmiImage_getSweep(probability->image, 0);
  if (probabilitySweep == NULL) {
    raiseException_returnNULL(PyExc_TypeError, "Probability field is empty");
  }

  resultimage = RaveFmiImage_new(1);
  if (resultimage == NULL) {
    return NULL;
  }

  restore_image(sourceSweep, RaveFmiImage_getSweep(resultimage, 0), probabilitySweep, threshold);

  result = (PyObject*)PyFmiImage_New(resultimage, 0);
  RAVE_OBJECT_RELEASE(resultimage);
  return result;
}

static PyObject* _pyropo_restore2(PyObject* self, PyObject* args)
{
  PyObject* sweepptr = NULL;
  PyObject* inprobabilityptr = NULL;
  PyFmiImage* sweep = NULL;
  PyFmiImage* probability = NULL;
  RaveFmiImage_t* resultimage = NULL;
  PyObject* result = NULL;
  FmiImage* sourceSweep = NULL; /* Do not release, internal memory */
  FmiImage* probabilitySweep = NULL; /* Do not release, internal memory */
  int threshold = 255;
  int sweepnr = 0;

  if (!PyArg_ParseTuple(args, "OOi|i", &sweepptr, &inprobabilityptr, &threshold, &sweepnr)) {
    return NULL;
  }

  if (!PyFmiImage_Check(sweepptr) || !PyFmiImage_Check(inprobabilityptr)) {
    raiseException_returnNULL(PyExc_TypeError, "restore2 takes sweep, probability field and threshold as input");
  }
  sweep = (PyFmiImage*)sweepptr;
  probability = (PyFmiImage*)inprobabilityptr;

  sourceSweep = RaveFmiImage_getSweep(sweep->image, sweepnr);
  if (sourceSweep == NULL) {
    raiseException_returnNULL(PyExc_TypeError, "Input fmi image does not contain that sweep");
  }
  probabilitySweep = RaveFmiImage_getSweep(probability->image, 0);
  if (probabilitySweep == NULL) {
    raiseException_returnNULL(PyExc_TypeError, "Probability field is empty");
  }

  resultimage = RaveFmiImage_new(1);
  if (resultimage == NULL) {
    return NULL;
  }

  restore_image2(sourceSweep, RaveFmiImage_getSweep(resultimage, 0), probabilitySweep, threshold);

  result = (PyObject*)PyFmiImage_New(resultimage, 0);
  RAVE_OBJECT_RELEASE(resultimage);
  return result;
}

static PyObject* _pyropo_update_classification(PyObject* self, PyObject* args)
{
  PyObject *classificationptr = NULL, *probabilityptr = NULL, *targetprobabilityptr = NULL;
  PyFmiImage *classification = NULL, *probability = NULL, *targetprobability = NULL;
  FmiImage *cSweep = NULL, *pSweep = NULL, *tSweep = NULL;

  FmiRadarPGMCode marker = CLEAR;

  if (!PyArg_ParseTuple(args, "OOi|O", &probabilityptr, &targetprobabilityptr, &marker, &classificationptr)) {
    return NULL;
  }

  if (!PyFmiImage_Check(probabilityptr) || !PyFmiImage_Check(targetprobabilityptr)) {
    raiseException_returnNULL(PyExc_TypeError, "Input types must be either OOi|O with probfield, targetprobfield, marker, classification field");
  }
  if (classificationptr != NULL && !PyFmiImage_Check(classificationptr)) {
    raiseException_returnNULL(PyExc_TypeError, "Input types must be either OOi|O with probfield, targetprobfield, marker, classification field");
  }
  classification = (PyFmiImage*)classificationptr;
  probability = (PyFmiImage*)probabilityptr;
  targetprobability = (PyFmiImage*)targetprobabilityptr;

  if (RaveFmiImage_getSweepCount(probability->image) != 1 ||
      RaveFmiImage_getSweepCount(targetprobability->image) != 1 ||
      (classification != NULL && RaveFmiImage_getSweepCount(targetprobability->image) != 1)) {
    raiseException_returnNULL(PyExc_TypeError, "Input types must be either OOi|O with probfield, targetprobfield, marker, classification field with sweep count 1");
  }
  if (classification != NULL) {
    cSweep = RaveFmiImage_getSweep(classification->image, 0);
  }
  pSweep = RaveFmiImage_getSweep(probability->image, 0);
  tSweep = RaveFmiImage_getSweep(targetprobability->image, 0);

  canonize_image(pSweep, cSweep);
  canonize_image(pSweep, tSweep);

  _pyropointernal_update_classification(pSweep, tSweep, cSweep, marker);

  Py_RETURN_NONE;
}
#endif

/*@} End of Functions */

/*@{ Module setup */
static PyMethodDef functions[] = {
  {"new", (PyCFunction)_pyropogenerator_new, 1},
  {NULL,NULL} /*Sentinel*/
};

MOD_INIT(_ropogenerator)
{
  PyObject *module=NULL,*dictionary=NULL;
  static void *PyRopoGenerator_API[PyRopoGenerator_API_pointers];
  PyObject *c_api_object = NULL;

  MOD_INIT_SETUP_TYPE(PyRopoGenerator_Type, &PyType_Type);

  MOD_INIT_VERIFY_TYPE_READY(&PyRopoGenerator_Type);

  MOD_INIT_DEF(module, "_ropogenerator", NULL/*doc*/, functions);
  if (module == NULL) {
    return MOD_INIT_ERROR;
  }

  PyRopoGenerator_API[PyRopoGenerator_Type_NUM] = (void*)&PyRopoGenerator_Type;
  PyRopoGenerator_API[PyRopoGenerator_GetNative_NUM] = (void *)PyRopoGenerator_GetNative;
  PyRopoGenerator_API[PyRopoGenerator_New_NUM] = (void*)PyRopoGenerator_New;


  c_api_object = PyCapsule_New(PyRopoGenerator_API, PyRopoGenerator_CAPSULE_NAME, NULL);
  dictionary = PyModule_GetDict(module);
  PyDict_SetItemString(dictionary, "_C_API", c_api_object);

  ErrorObject = PyErr_NewException("_ropogenerator.error", NULL, NULL);
  if (ErrorObject == NULL || PyDict_SetItemString(dictionary, "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _ropogenerator.error");
    return MOD_INIT_ERROR;
  }

  import_fmiimage();
  PYRAVE_DEBUG_INITIALIZE;
  return MOD_INIT_SUCCESS(module);
}
/*@} End of Module setup */

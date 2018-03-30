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
 * Python version of the ropo generator
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-08-31
 */
#ifndef PYROPOGENERATOR_H
#define PYROPOGENERATOR_H
#include "rave_ropo_generator.h"

/**
 * The fmi image object
 */
typedef struct {
   PyObject_HEAD /*Always have to be on top*/
   RaveRopoGenerator_t* generator;  /**< the generator */
} PyRopoGenerator;

#define PyRopoGenerator_Type_NUM 0                              /**< index of type */

#define PyRopoGenerator_GetNative_NUM 1                         /**< index of GetNative*/
#define PyRopoGenerator_GetNative_RETURN RaveRopoGenerator_t*         /**< return type for GetNative */
#define PyRopoGenerator_GetNative_PROTO (PyRopoGenerator*)        /**< arguments for GetNative */

#define PyRopoGenerator_New_NUM 2                               /**< index of New */
#define PyRopoGenerator_New_RETURN PyRopoGenerator*                  /**< return type for New */
#define PyRopoGenerator_New_PROTO (RaveRopoGenerator_t*, RaveFmiImage_t*)        /**< arguments for New */

#define PyRopoGenerator_API_pointers 3                          /**< number of type and function pointers */

#define PyRopoGenerator_CAPSULE_NAME "_fmiimage._C_API"

#ifdef PYROPOGENERATOR_MODULE
/** Forward declaration of type */
extern PyTypeObject PyRopoGenerator_Type;

/** Checks if the object is a PyPolarVolume or not */
#define PyRopoGenerator_Check(op) ((op)->ob_type == &PyRopoGenerator_Type)

/** Forward declaration of PyRopoGenerator_GetNative */
static PyRopoGenerator_GetNative_RETURN PyRopoGenerator_GetNative PyRopoGenerator_GetNative_PROTO;

/** Forward declaration of PyRopoGenerator_New */
static PyRopoGenerator_New_RETURN PyRopoGenerator_New PyRopoGenerator_New_PROTO;

#else
/** Pointers to types and functions */
static void **PyRopoGenerator_API;

/**
 * Returns a pointer to the internal fmi image, remember to release the reference
 * when done with the object. (RAVE_OBJECT_RELEASE).
 */
#define PyRopoGenerator_GetNative \
  (*(PyRopoGenerator_GetNative_RETURN (*)PyRopoGenerator_GetNative_PROTO) PyRopoGenerator_API[PyRopoGenerator_GetNative_NUM])

/**
 * Creates a new ropo generator instance. Release this object with Py_DECREF. If a RaveRopoGenerator_t instance is
 * provided and this instance already is bound to a python instance, this instance will be increfed and
 * returned.
 * @param[in] generator - the RaveRopoGenerator_t instance.
 * @param[in] image - the ropo generator for this object only used if generator == NULL
 * @returns the PyRopoGenerator instance.
 */
#define PyRopoGenerator_New \
  (*(PyRopoGenerator_New_RETURN (*)PyRopoGenerator_New_PROTO) PyRopoGenerator_API[PyRopoGenerator_New_NUM])

/**
 * Checks if the object is a python fmi image.
 */
#define PyRopoGenerator_Check(op) \
  (Py_TYPE(op) == &PyRopoGenerator_Type)

#define PyRopoGenerator_Type (*(PyTypeObject*)PyRopoGenerator_API[PyRopoGenerator_Type_NUM])

/**
 * Imports the PyRopoGenerator module (like import _ropogenerator in python).
 */
#define import_ropogenerator() \
		PyRopoGenerator_API = (void **)PyCapsule_Import(PyRopoGenerator_CAPSULE_NAME, 1);

#endif

#endif /* PYROPOGENERATOR_H */

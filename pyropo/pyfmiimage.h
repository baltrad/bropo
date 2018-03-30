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
#ifndef PYFMIIMAGE_H
#define PYFMIIMAGE_H
#include "rave_fmi_image.h"


/**
 * The fmi image object
 */
typedef struct {
   PyObject_HEAD /*Always have to be on top*/
   RaveFmiImage_t* image;  /**< the fmi image */
} PyFmiImage;

#define PyFmiImage_Type_NUM 0                              /**< index of type */

#define PyFmiImage_GetNative_NUM 1                         /**< index of GetNative*/
#define PyFmiImage_GetNative_RETURN RaveFmiImage_t*         /**< return type for GetNative */
#define PyFmiImage_GetNative_PROTO (PyFmiImage*)        /**< arguments for GetNative */

#define PyFmiImage_New_NUM 2                               /**< index of New */
#define PyFmiImage_New_RETURN PyFmiImage*                  /**< return type for New */
#define PyFmiImage_New_PROTO (RaveFmiImage_t*, int, int)        /**< arguments for New */

#define PyFmiImage_API_pointers 3                          /**< number of type and function pointers */

#define PyFmiImage_CAPSULE_NAME "_fmiimage._C_API"

#ifdef PYFMIIMAGE_MODULE

/** Forward declaration of type */
extern PyTypeObject PyFmiImage_Type;

/** Checks if the object is a PyPolarVolume or not */
#define PyFmiImage_Check(op) ((op)->ob_type == &PyFmiImage_Type)

/** Forward declaration of PyFmiImage_GetNative */
static PyFmiImage_GetNative_RETURN PyFmiImage_GetNative PyFmiImage_GetNative_PROTO;

/** Forward declaration of PyFmiImage_New */
static PyFmiImage_New_RETURN PyFmiImage_New PyFmiImage_New_PROTO;

#else
/** Pointers to types and functions */
static void **PyFmiImage_API;

/**
 * Returns a pointer to the internal composite, remember to release the reference
 * when done with the object. (RAVE_OBJECT_RELEASE).
 */
#define PyFmiImage_GetNative \
  (*(PyFmiImage_GetNative_RETURN (*)PyFmiImage_GetNative_PROTO) PyFmiImage_API[PyFmiImage_GetNative_NUM])

/**
 * Creates a new fmi image instance. Release this object with Py_DECREF. If a FmiImage_t instance is
 * provided and this instance already is bound to a python instance, this instance will be increfed and
 * returned.
 * @param[in] image - the RaveFmiImage_t intance.
 * @param[in] width - the width, only used if image == NULL and width > 0 and height > 0
 * @param[in] height - the height, only used if image == NULL and width > 0 and height > 0
 * @returns the PyFmiImage instance.
 */
#define PyFmiImage_New \
  (*(PyFmiImage_New_RETURN (*)PyFmiImage_New_PROTO) PyFmiImage_API[PyFmiImage_New_NUM])

/**
 * Checks if the object is a python fmi image.
 */
#define PyFmiImage_Check(op) \
   (Py_TYPE(op) == &PyFmiImage_Type)


#define PyFmiImage_Type (*(PyTypeObject*)PyFmiImage_API[PyFmiImage_Type_NUM])

/**
 * Imports the PyArea module (like import _area in python).
 */
#define import_fmiimage() \
    PyFmiImage_API = (void **)PyCapsule_Import(PyFmiImage_CAPSULE_NAME, 1);

#endif

#endif /* PYFMIIMAGE_H */

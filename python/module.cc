#include "LaDaConfig.h"

#include <Python.h>
#include <structmember.h>
#include <numpy/arrayobject.h>

#include <errors/exceptions.h>
#include <math/math.h>

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
# define PyMODINIT_FUNC void
#endif
#define LADA_PYTHON_MODULE 0
#include "python.h"

namespace LaDa
{
  namespace python
  {
    namespace 
    {
#     include "object.cc"
#     include "quantity.cc"
    }
  }
}



PyMODINIT_FUNC initcppwrappers(void) 
{
  using namespace LaDa::python;
  using namespace LaDa;
  static void *api_capsule[LADA_SLOT(python)];
  static PyMethodDef methods_table[] = { {NULL, NULL, 0, NULL} };
  PyObject *c_api_object;

  char const doc[] =  "Wrapper around basic C++ helper functions.\n\n"
                      "This module only contains a capsule for cpp functions.\n";
  PyObject* module = Py_InitModule3("cppwrappers", methods_table, doc);
  if(not module) return;

  import_array(); // needed for NumPy 

  /* Initialize the C API pointer array */
# undef LADA_PYTHON_PYTHON_H
# include "python.h"

  /* Create a Capsule containing the API pointer array's address */
  # ifdef LADA_PYTHONTWOSIX
      c_api_object = PyCObject_FromVoidPtr((void *)api_capsule, NULL);
  # else
      static const char name[] = "lada.cppwrappers._C_API";
      c_api_object = PyCapsule_New((void *)api_capsule, name, NULL);
  # endif
  if (c_api_object != NULL) PyModule_AddObject(module, "_C_API", c_api_object);
}

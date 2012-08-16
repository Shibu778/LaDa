#include "LaDaConfig.h"

#include <Python.h>
#include <structmember.h>
#define PY_ARRAY_UNIQUE_SYMBOL enumeration_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include <vector>
#include <algorithm>


#include <python/numpy_types.h>
#include <python/exceptions.h>
#include <python/wrap_numpy.h>
#include <python/quantity.h>

#include "fciterator.h"

namespace LaDa
{
  namespace enumeration
  {
    //! Creates a new fciterator.
    FCIterator* PyFCIterator_New()
    {
      FCIterator* result = (FCIterator*) fciterator_type()->tp_alloc(fciterator_type(), 0);
      if(not result) return NULL;
      result->yielded = (PyArrayObject*)Py_None;
      Py_INCREF(Py_None);
      new(&result->counter) std::vector<t_fc>;
      return result;
    }
    //! Creates a new fciterator with a given type.
    FCIterator* PyFCIterator_NewWithArgs(PyTypeObject* _type, PyObject *_args, PyObject *_kwargs)
    {
      FCIterator* result = (FCIterator*)_type->tp_alloc(_type, 0);
      if(not result) return NULL;
      result->yielded = (PyArrayObject*)Py_None;
      Py_INCREF(Py_None);
      new(&result->counter) std::vector<t_fc>;
      return result;
    }

    // Creates a new fciterator with a given type, also calling initialization.
    FCIterator* PyFCIterator_NewFromArgs(PyTypeObject* _type, PyObject *_args, PyObject *_kwargs)
    {
      FCIterator* result = PyFCIterator_NewWithArgs(_type, _args, _kwargs);
      if(result == NULL) return NULL;
      if(_type->tp_init((PyObject*)result, _args, _kwargs) < 0) {Py_DECREF(result); return NULL; }
      return result;
    }

    static int gcclear(FCIterator *self)
    { 
      Py_CLEAR(self->yielded);
      return 0;
    }


    // Function to deallocate a string atom.
    static void fciterator_dealloc(FCIterator *_self)
    {
      gcclear(_self);
  
      // calls destructor explicitely.
      PyTypeObject* ob_type = _self->ob_type;
      _self->~FCIterator();

      ob_type->tp_free((PyObject*)_self);
    }
  
    // Function to initialize an atom.
    static int fciterator_init(FCIterator* _self, PyObject* _args, PyObject *_kwargs)
    {
      if(_kwargs != NULL and PyDict_Size(_kwargs))
      {
        LADA_PYERROR(TypeError, "FCIterator does not expect keyword arguments.");
        return -1;
      }
      int length = 0;
      if(not PyArg_ParseTuple(_args, "ii", &length, &_self->ntrue))
        return -1;
      if(length <= 0)
      {
        LADA_PYERROR(ValueError, "length argument should be strictly positive.");
        return -1;
      }
      if(_self->ntrue <= 0)
      {
        LADA_PYERROR(ValueError, "The second argument (number of '1's) "
                                 "should be strictly positive.");
        return -1;
      }
      if(_self->ntrue > length)
      {
        LADA_PYERROR(ValueError, "The second argument (number of '1's) "
                                 "should be smaller or equalt to the first"
                                 "(bitstring size).");
        return -1;
      }
      _self->counter.resize(length);
      std::fill(_self->counter.begin(), _self->counter.begin()+_self->ntrue, 1);
      std::fill(_self->counter.begin()+_self->ntrue, _self->counter.end(), 0);
      _self->is_first = true;
      if(_self->yielded != NULL)
      {
        PyArrayObject* dummy = _self->yielded;
        _self->yielded = NULL;
        Py_DECREF(dummy);
      }
      typedef math::numpy::type<bool> t_type;
      npy_intp d[1] = {(npy_intp)length};
      _self->yielded = (PyArrayObject*)
          PyArray_SimpleNewFromData(1, d, t_type::value, &_self->counter[0]);
      if(not _self->yielded) return -1;
#     ifdef LADA_MACRO
#       error LADA_MACRO already defined
#     endif
#     ifdef NPY_ARRAY_WRITEABLE
#       define LADA_MACRO NPY_ARRAY_WRITEABLE
#     else
#       define LADA_MACRO NPY_WRITEABLE
#     endif
      if(_self->yielded->flags & LADA_MACRO) _self->yielded->flags -= LADA_MACRO;
#     undef LADA_MACRO
#     ifdef NPY_ARRAY_C_CONTIGUOUS
#       define LADA_MACRO NPY_ARRAY_C_CONTIGUOUS;
#     else 
#       define LADA_MACRO NPY_C_CONTIGUOUS
#     endif
      if(not (_self->yielded->flags & LADA_MACRO)) _self->yielded->flags += LADA_MACRO;
#     undef LADA_MACRO
      _self->yielded->base = (PyObject*)_self;
      Py_INCREF(_self);
      return 0;
    }
  
    static int traverse(FCIterator *self, visitproc visit, void *arg)
    {
      Py_VISIT(self->yielded);
      return 0;
    }
  
    static PyObject* self(PyObject* _self)
    {
      Py_INCREF(_self);
      return _self;
    }

    static PyObject* next(FCIterator* _self)
    {
      if(_self->is_first)
      {
        _self->is_first = false; 
#       ifdef LADA_DEBUG
          if(_self->yielded == NULL)
          {
            LADA_PYERROR(internal, "Yielded was not initialized.");
            return NULL;
          }
          if(_self->yielded->data != &_self->counter[0])
          {
            LADA_PYERROR(internal, "Yielded does not reference counter.");
            return NULL;
          }
#       endif
        Py_INCREF(_self->yielded);
        return (PyObject*)_self->yielded;
      }
      std::vector<t_fc>::reverse_iterator const i_first = _self->counter.rbegin();
      std::vector<t_fc>::reverse_iterator const i_last = _self->counter.rend();
      // finds first zero.
      std::vector<t_fc>::reverse_iterator i_zero = i_first;
      bool found_zero = false;
      for(; i_zero != i_last; ++i_zero)
        if(not *i_zero) { found_zero = true; break; }
      if(not found_zero)
      {
        if((PyObject*)_self->yielded != Py_None)
        {
          PyArrayObject* dummy = _self->yielded;
          _self->yielded = (PyArrayObject*)Py_None;
          Py_INCREF(Py_None);
          Py_DECREF(dummy);
        }
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
      }
      // finds first one preceded by zero.
      std::vector<t_fc>::reverse_iterator i_one = i_zero + 1;
      bool found_one = false;
      for(; i_one != i_last; ++i_one)
        if(*i_one) { found_one = true; break; }
      if(not found_one)
      {
        if((PyObject*)_self->yielded != Py_None)
        {
          PyArrayObject* dummy = _self->yielded;
          _self->yielded = (PyArrayObject*)Py_None;
          Py_INCREF(Py_None);
          Py_DECREF(dummy);
        }
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
      }
      // swap the two. 
      *i_one = false;
      *(--i_one) = true;
      // if true, then 1's are tight on the left
      if(i_one != i_zero and i_zero != i_first)
      {
        // fill in with ones from the right
        --i_one;
        for(int i(0), N(i_zero - i_first); i < N; --i_one, ++i) *i_one = true;
        // fill in with additional zeros.
        for(; i_one != i_first; --i_one) *i_one = false;
        *i_one = false;
      }
      
#     ifdef LADA_DEBUG
        if(_self->yielded == NULL)
        {
          LADA_PYERROR(internal, "Yielded was not initialized.");
          return NULL;
        }
        if(_self->yielded->data != &_self->counter[0])
        {
          LADA_PYERROR(internal, "Yielded does not reference counter.");
          return NULL;
        }
#     endif
      Py_INCREF(_self->yielded);
      return (PyObject*)_self->yielded;
    }

    PyObject *reset(FCIterator *_self)
    {
      if(_self->counter.size() == 0)
      {
        LADA_PYERROR(internal, "Iterator was never initialized");
        return NULL;
      }
      std::fill(_self->counter.begin(), _self->counter.begin() + _self->ntrue, 1);
      std::fill(_self->counter.begin() + _self->ntrue, _self->counter.end(), 0);

      typedef math::numpy::type<bool> t_type;
      npy_intp d[1] = {(npy_intp)_self->counter.size()};
      _self->yielded = (PyArrayObject*)
          PyArray_SimpleNewFromData(1, d, t_type::value, &_self->counter[0]);
      _self->is_first = true;
      if(not _self->yielded) return NULL;
      Py_RETURN_NONE;
    }


    // Returns pointer to fciterator type.
    PyTypeObject* fciterator_type()
    {
#     ifdef LADA_DECLARE
#       error LADA_DECLARE already declared
#     endif
#     define LADA_DECLARE(name, func, args, doc) \
        {#name, (PyCFunction)func, METH_ ## args, doc} 
      static PyMethodDef methods[] = {
        LADA_DECLARE(reset, reset, NOARGS, "Resets iterator."),
          {NULL}  /* Sentinel */
      };
#     undef LADA_DECLARE
#     define LADA_DECLARE(name, object, doc) \
        { const_cast<char*>(#name), T_OBJECT_EX, \
          offsetof(FCIterator, object), 0, const_cast<char*>(doc) }
      static PyMemberDef members[] = {
        LADA_DECLARE(yielded, yielded, "Object to be yielded."),
        {NULL, 0, 0, 0, NULL}  /* Sentinel */
      };
#     undef LADA_DECLARE

      static PyTypeObject dummy = {
          PyObject_HEAD_INIT(NULL)
          0,                                 /*ob_size*/
          "lada.enum.cppwrappers.FCIterator",   /*tp_name*/
          sizeof(FCIterator),              /*tp_basicsize*/
          0,                                 /*tp_itemsize*/
          (destructor)fciterator_dealloc,  /*tp_dealloc*/
          0,                                 /*tp_print*/
          0,                                 /*tp_getattr*/
          0,                                 /*tp_setattr*/
          0,                                 /*tp_compare*/
          0,                                 /*tp_repr*/
          0,                                 /*tp_as_number*/
          0,                                 /*tp_as_sequence*/
          0,                                 /*tp_as_mapping*/
          0,                                 /*tp_hash */
          0,                                 /*tp_call*/
          0,                                 /*tp_str*/
          0,                                 /*tp_getattro*/
          0,                                 /*tp_setattro*/
          0,                                 /*tp_as_buffer*/
          Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_ITER, /*tp_flags*/
          "Defines a fixerd concentration iterator.\n\n"
          "This iterator holds a numpy array where each element is incremented\n"
          "between 1 and a specified value. Once an element (starting from the\n"
          "last) reaches the specified value, it is reset to 1 and the next is\n"
          "element is incremented. It looks something like this:\n\n"
          "  >>> for u in FCIterator(2, 3): print u\n"
          "  [1 1]\n"
          "  [1 2]\n"
          "  [1 3]\n"
          "  [2 1]\n"
          "  [2 2]\n"
          "  [2 3]\n\n"
          "The last item is incremented up to 3, and the first item up to 2, as\n"
          "per the input. FCIterator accepts any number of arguments, as long\n"
          "as they are integers. The yielded result ``u`` is a read-only numpy\n"
          "array.\n\n"
          ".. warning::\n\n"
          "   The yielded array *always* points to the same address in memory:\n\n"
          "   .. code-block:: python\n"
          "      iterator = FCIterator(2, 3)\n"
          "      a = iterator.next()\n"
          "      assert all(a == [1, 1])\n"
          "      b = iterator.next()\n"
          "      assert all(b == [1, 2])\n"
          "      assert all(a == b)\n"
          "      assert a is b\n\n"
          "   If you need to keep track of a prior value, use numpy's copy_\n"
          "   function.\n\n"
          "   The other effect is that :py:class:`FCIterator` cannot be used\n"
          "   with zip_ and similar functions:\n\n"
          "   .. code-block:: python\n\n"
          "     for u, v in zip( FCIterator(5, 5, 5), \n"
          "                      product(range(1, 6), repeat=3) ):\n"
          "       assert all(u == 1)\n\n"
          ".. copy_: http://docs.scipy.org/doc/numpy/reference/generated/numpy.copy.html\n"
          ".. zip_: http://docs.python.org/library/functions.html#zip\n",
          (traverseproc)traverse,            /* tp_traverse */
          (inquiry)gcclear,                  /* tp_clear */
          0,		                             /* tp_richcompare */
          0,                                 /* tp_weaklistoffset */
          (getiterfunc)self,                 /* tp_iter */
          (iternextfunc)next,                /* tp_iternext */
          methods,                           /* tp_methods */
          members,                           /* tp_members */
          0,                                 /* tp_getset */
          0,                                 /* tp_base */
          0,                                 /* tp_dict */
          0,                                 /* tp_descr_get */
          0,                                 /* tp_descr_set */
          0,                                 /* tp_dictoffset */
          (initproc)fciterator_init,       /* tp_init */
          0,                                 /* tp_alloc */
          (newfunc)PyFCIterator_NewWithArgs,  /* tp_new */
      };
      return &dummy;
    }

  } // namespace Crystal
} // namespace LaDa

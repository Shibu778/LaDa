#ifndef LADA_CRYSTAL_NEIGHBORS_H
#define LADA_CRYSTAL_NEIGHBORS_H

#include "LaDaConfig.h"

#include <boost/lambda/lambda.hpp>

#include <misc/types.h>
#include "structure/structure.h"

namespace LaDa
{
  namespace crystal
  {
    namespace
    {
      //! \brief Creates list of first neigbors up to given input.
      //! \details Always return all nth neighbors. In other words, in fcc, if you
      //!          ask for 6 neighbor, actually 12 are returned. 
      //! \returns A python list of 3-tuples. Each tuple consists of a (unwrapped
      //!          python) reference to an atom, a vector which goes from the
      //!          center to the relevant periodic image of that neighbor, and the
      //!          distance between the center and that neighbor.
      PyObject* neighbors( Structure const &_structure, Py_ssize_t _nmax, 
                           math::rVector3d const &_center,
                           types::t_real _tolerance=types::tolerance )
#     ifdef LADA_CRYSTAL_MODULE
          ;
#     else
          { return (*(PyObject*(*)(Structure const&, Py_ssize_t, math::rVector3d const&, types::t_real))
                    api_capsule[16])(_structure, _nmax, _center, _tolerance); }
#     endif
    } // anonymous namespace
  } // end of crystal namespace.
} // namespace LaDa

#endif

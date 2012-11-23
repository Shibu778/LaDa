#if LADA_CRYSTAL_MODULE != 1
namespace LaDa
{
  namespace crystal 
  {
    namespace
    {
#endif

#if LADA_CRYSTAL_MODULE != 1
  //! \brief Finds and stores point group operations.
  //! \details Rotations are determined from G-vector triplets with the same
  //!          norm as the unit-cell vectors.
  //! \param[in] cell The cell for which to find the point group.
  //! \param[in] _tol acceptable tolerance when determining symmetries.
  //!             -1 implies that types::tolerance is used.
  //! \retval python list of affine symmetry operations for the given structure.
  //!         Each element is a 4x3 numpy array, with the first 3 rows
  //!         forming the rotation, and the last row is the translation.
  //!         The affine transform is applied as rotation * vector + translation.
  //!         `cell_invariants` always returns isometries (translation is zero).
  //! \see Taken from Enum code, PRB 77, 224115 (2008).
  PyObject* cell_invariants(math::rMatrix3d const &_cell, types::t_real _tolerance = -1e0)
    LADA_END({ return (*(PyObject*(*)(math::rMatrix3d const &, types::t_real))
                       api_capsule[LADA_SLOT(crystal)])(_cell, _tolerance); })
#else
  api_capsule[LADA_SLOT(crystal)] = (void *)cell_invariants;
#endif
#define BOOST_PP_VALUE BOOST_PP_INC(LADA_SLOT(crystal))
#include LADA_ASSIGN_SLOT(crystal)

#if LADA_CRYSTAL_MODULE != 1
  //! \brief Finds and stores space group operations.
  //! \param[in] _structure The structure for which to find the space group.
  //! \param[in] _tol acceptable tolerance when determining symmetries.
  //!             -1 implies that types::tolerance is used.
  //! \retval spacegroup python list of symmetry operations for the given structure.
  //!         Each element is a 4x3 numpy array, with the first 3 rows
  //!         forming the rotation, and the last row is the translation.
  //!         The affine transform is applied as rotation * vector + translation.
  //! \warning Works for primitive lattices only.
  //! \see Taken from Enum code, PRB 77, 224115 (2008).
  PyObject* space_group(Structure const &_lattice, types::t_real _tolerance = -1e0)
    LADA_END({ return (*(PyObject*(*)(Structure const &, types::t_real))
                       api_capsule[LADA_SLOT(crystal)])(_lattice, _tolerance); })
#else
  api_capsule[LADA_SLOT(crystal)] = (void *)space_group;
#endif
#define BOOST_PP_VALUE BOOST_PP_INC(LADA_SLOT(crystal))
#include LADA_ASSIGN_SLOT(crystal)

#if LADA_CRYSTAL_MODULE != 1
    } // anonymous namespace
  }
}
#endif

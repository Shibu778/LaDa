//
//  Version: $Id$
//
#ifndef LADA_ENUM_TRANSFORM_H_
#define LADA_ENUM_TRANSFORM_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <vector>
#include <utility>

#include <crystal/smith.h>
#include <crystal/symmetry_operator.h>

#include "numeric_type.h"

namespace LaDa
{
  namespace enumeration
  {
    //! \brief Symmetry operation of the lattice operating on an integer structure.
    //! \see Transform an integer structure to another. Appendix of PRB 80, 014120.
    class Transform : public Crystal::SymmetryOperator
    {
      //! Type of the container of supercell-independent elements.
      typedef std::pair<types::t_int, atat::rVector3d> t_Independent;
      //! Type of the container of supercell-independent elements.
      typedef std::vector<t_Independent> t_Independents;
      public:
        //! Copy Constructor
        Transform   (Crystal::SymmetryOperator const &_c, Crystal::Lattice const &_lat) throw(boost::exception);
        //! Copy Constructor
        Transform    (Transform const &_c)
                   : Crystal::SymmetryOperator(_c),
                     permutations_(_c.permutations_), 
                     nsites_(_c.nsites_),
                     card_(_c.card_) {}
        //! Initializes transform for specific supercell.
        void init( Crystal::t_SmithTransform const &_transform ) throw(boost::exception);
        //! Performs transformation.
        t_uint operator()(t_uint _x, FlavorBase const &_flavorbase) const;

      private:
        //! Permutation.
        std::vector<size_t> permutations_;
        //! Independent elements d_{N,d} and t_{N,d} 
        t_Independents independents_;
        //! Number of sites in the unit lattice-cell.
        size_t nsites_;
        //! Total number of sites in the supercell.
        size_t card_;
    };

    boost::shared_ptr< std::vector<Transform> >  create_transforms( Crystal::Lattice const &_lat );
  }
}

#endif

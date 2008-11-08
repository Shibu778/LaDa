//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <cstdlib>

#include <algorithm>
#include <functional>
#include <boost/filesystem/operations.hpp>

#include <physics/physics.h>
#include <opt/ndim_iterator.h>
#include <opt/atat.h>
#include <opt/debug.h>
#include <opt/atat.h>
#include <opt/tinyxml.h>

#include "atomic_center.h"
  
namespace LaDa
{
  namespace Vff
  { 
    Atomic_Center :: Atomic_Center  ( Crystal::Structure &_str, t_Atom &_e,
                                      types::t_unsigned _i )
                                   : origin(&_e), structure(&_str)
    {
       is_site_one = ( structure->lattice->get_atom_site_index( _e ) == 0 );
       is_site_one_two_species = ( structure->lattice->get_nb_types( 0 ) == 2 );
       index = _i;
    }

    types::t_unsigned  Atomic_Center :: kind() const
    {
      if ( is_site_one )
        return structure->lattice->convert_real_to_type_index( 0, origin->type );
      else if ( is_site_one_two_species )
        return 2 + structure->lattice->convert_real_to_type_index( 1, origin->type );
      return 1 + structure->lattice->convert_real_to_type_index( 1, origin->type );
    }

    types::t_unsigned  Atomic_Center :: bond_kind( const Atomic_Center &_bond ) const
    {
      if ( is_site_one ) 
        return  structure->lattice->convert_real_to_type_index( 1, _bond.origin->type );
      else if ( is_site_one_two_species )
        return  structure->lattice->convert_real_to_type_index( 0, _bond.origin->type );
      return 0; 
    }

    types::t_int Atomic_Center :: add_bond( t_BondRefd _bond, 
                                            const types::t_real _cutoff ) 
    {
      bool found_bond = false;
      opt::Ndim_Iterator< types::t_int, std::less_equal<types::t_int> > period;
      period.add(-1,1);
      period.add(-1,1);
      period.add(-1,1);

      do // goes over periodic image
      {
        // constructs perdiodic image of atom *i_bond
        atat::rVector3d frac_image, image;
        frac_image[0] =  (types::t_real) period.access(0);
        frac_image[1] =  (types::t_real) period.access(1);
        frac_image[2] =  (types::t_real) period.access(2);
        image = _bond->origin->pos + structure->cell * frac_image;

        // checks if within 
        if( atat::norm2( image - origin->pos ) < _cutoff  )
        {
          // adds bond
          types :: t_unsigned site = structure->lattice->get_atom_site_index( image );
          if ( (not site) == is_site_one )
            return -1;  // error bond between same site

          bonds.push_back( _bond );
          translations.push_back( frac_image );
          do_translates.push_back( atat::norm2( frac_image ) > atat::zero_tolerance );
          found_bond = true;
        }
      } while ( ++period ); 

      return found_bond ? (types::t_int) bonds.size() : -1; // not a bond
    }
#   ifdef _LADADEBUG
      void Atomic_Center :: const_iterator :: check() const
      {
        __ASSERT(not parent,
                 "Pointer to parent atom is invalid.\n")
        __ASSERT( parent->bonds.size() == 0,
                  "The number of bond is zero.\n")
        __ASSERT( parent->translations.size() == 0,
                  "The number of translations is zero.\n")
        __ASSERT( parent->do_translates.size() == 0,
                  "The number of translation switches is zero.\n")
        __ASSERT( i_bond - parent->bonds.end() > 0,
                  "The bond iterator is beyond the last bond.\n")
        if( i_bond ==  parent->bonds.end() ) return;
        __ASSERT( i_bond - parent->bonds.begin() < 0,
                  "The bond iterator is before the first bond.\n")
        types::t_int pos = i_bond -  parent->bonds.begin();
        __ASSERT( i_translation - parent->translations.end() > 0,
                  "The translation iterator is beyond the last bond.\n")
        __ASSERT( i_translation - parent->translations.begin() < 0,
                  "The translation iterator is before the first bond.\n")
        __ASSERT( i_translation != parent->translations.begin() + pos,
                     "The bond iterator and the translation "
                  << "iterator are out of sync.\n")
        __ASSERT( i_do_translate - parent->do_translates.end() > 0,
                  "The do_translate iterator is beyond the last bond.\n")
        __ASSERT( i_do_translate - parent->do_translates.begin() < 0,
                  "The do_translate iterator is before the first bond.\n")
        __ASSERT( i_do_translate != parent->do_translates.begin() + pos,
                     "The bond iterator and the do_translate "
                  << "iterator are out of sync.\n")
      }
      void Atomic_Center :: const_iterator :: check_valid() const
      {
        check();
        __ASSERT( i_bond == parent->bonds.end(),
                  "Invalid iterator.\n";)
        __ASSERT( not parent->origin,
                  "Origin of the parent atom is invalid.\n")
        __ASSERT( not (*i_bond)->origin,
                  "Origin of the bond atom is invalid.\n")
      }
#   endif
  } // namespace vff
} // namespace LaDa
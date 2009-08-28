//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <crystal/structure.h>
#include <crystal/neighbors.h>

#include "bases.h"
#include "representation.h"

namespace LaDa
{
  namespace atomic_potential
  {
    // Strict weak ordering functor from knowledge of basis.
    struct basis_sort
    {
      Basis const& basis;
      basis_sort( Basis const &_b ) : basis(_b) {}
      basis_sort( basis_sort const &_b ) : basis(_b.basis) {}
      bool operator()( Crystal::Neighbor const * const _a,
                       Crystal::Neighbor const * const _b ) const
      {
        if( Fuzzy::neq( _a->distance, _b->distance ) ) return _a->distance < _b->distance;
        const types::t_real ax = _a->pos * basis.x;
        const types::t_real bx = _b->pos * basis.x;
        if( Fuzzy::neq( ax, bx ) ) return ax > bx;
        const types::t_real ay = _a->pos * basis.y;
        const types::t_real by = _b->pos * basis.y;
        if( Fuzzy::neq( ay, by ) ) return ay > by;
        const types::t_real az = _a->pos * basis.z;
        const types::t_real bz = _b->pos * basis.z;
        if( Fuzzy::neq( az, bz ) ) return az > bz;
        return false;
      }
    }; 


    template<class A>
      void transform( A const& _atoms, VariableSet &_vars, Basis const &_basis,
                      Crystal::TStructure<std::string> const &_structure );

    Representation :: Representation(Crystal::TStructure<std::string> const &_structure,
                                     size_t _natoms )
    {
      // now loops over bases.
      typedef Bases< Crystal::TStructure<std::string> > t_Bases;
      t_Bases bases( _structure );
      t_Bases::const_iterator i_basis = bases.begin();
      t_Bases::const_iterator i_basis_end = bases.end();

      // first neighbor containor.
      Crystal :: Neighbors neighbors( _natoms );
      neighbors.origin = i_basis->origin + atat::rVector3d(1,0,0);
      size_t index(1);
      
      // sorting container.
      std::vector<Crystal::Neighbor const*> atoms(_natoms, NULL);
      size_t origin_type(0);
      
      for(; i_basis != i_basis_end; ++i_basis )
      {
        if( index != i_basis->index ) 
        {
          index = i_basis->index;

          // finds new nearest neighbors.
          neighbors.origin = i_basis->origin;
          std::vector<Crystal::Neighbor const*>::iterator i_atom = atoms.begin();
          std::vector<Crystal::Neighbor const*>::iterator i_atom_end = atoms.end();
          Crystal::Neighbors::const_iterator i_neigh = neighbors.begin( _structure );
          for(; i_atom != i_atom_end; ++i_atom, ++i_neigh ) *i_atom = &(*i_neigh);

          // finds type index at origin.
          origin_type = 0;
          types::t_int const sindex( _structure.atoms[index].site );
          std::string const &type = _structure.atoms[index].type;
          foreach( std::string const& str, _structure.lattice->sites[sindex<0? 0: sindex].type )
          {
            if( str == type ) break;
            ++origin_type;
          }
        };
        // sorts according to basis.
        std::sort
        ( 
          atoms.begin(),
          atoms.end(),
          basis_sort(*i_basis) 
        );

        // gets atomic type of the structure
        VariableSet variable_set;
        variable_set.weight = i_basis->weight;
        variable_set.variables.reserve(3*_natoms-2);
        variable_set.variables.push_back(VariableSet::t_Variable(0, origin_type)); 
        transform( atoms, variable_set, *i_basis, _structure );
        add_(variable_set);
      }
    }

    void Representation::add_( VariableSet const &_rep )
    {
      t_Sets::iterator i_found = std::find( sets_.begin(), sets_.end(), _rep );
      if( i_found == sets_.end() ) sets_.push_back(_rep);
      else i_found->weight += _rep.weight;
    }

    template<class A>
      void transform( A const& _atoms, VariableSet &_vars, Basis const &_basis,
                      Crystal::TStructure<std::string> const &_structure )
      {
        LADA_ASSERT( _structure.lattice, "Lattice is not set.\n" )
        foreach( typename A::value_type const ptr_atom, _atoms ) 
        {
          Crystal::TStructure<std::string>::t_Atom const &atom = _structure.atoms[ ptr_atom->index ];
          specie_type type(0);
          foreach( std::string const& str, _structure.lattice->sites[atom.site<0? 0: atom.site].type )
          {
            if( str == atom.type ) break;
            ++type;
          }
          atat::rVector3d const vec(ptr_atom->pos); 
          switch( _vars.variables.size() )
          {
            default: _vars.variables.push_back( VariableSet::t_Variable(vec * _basis.z, type) );
            case 2:  _vars.variables.push_back( VariableSet::t_Variable(vec * _basis.y, type) );
            case 1:  _vars.variables.push_back( VariableSet::t_Variable(vec * _basis.x, type) ); break;
          }
        }
      }
     
    std::ostream& operator<<( std::ostream& _stream, Representation const &_rep )
    {
      _stream << "Representation:\n";
      Representation::const_iterator i_first = _rep.begin();
      Representation::const_iterator const i_end = _rep.end();
      for(; i_first != i_end; ++i_first)
      {
        _stream << "  _ weight: " << i_first->weight << ", ";
        typedef Representation::const_iterator::value_type::t_Variables::const_iterator cit;
        cit i_var = i_first->variables.begin();
        cit const i_var_end = i_first->variables.end();
        for(size_t i(1);  i_var != i_var_end; ++i_var, ++i)
        {
          if( i % 10 == 0 ) _stream << "\n                   ";
          _stream << *i_var << " ";
        }
        _stream << "\n";
      }
      return _stream;
    }

  } // namespace atomic_potential
} // namespace LaDa
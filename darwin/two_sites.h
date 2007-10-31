//
//  Version: $Id$
//
#ifndef _TWOSITES_H_
#define _TWOSITES_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>

#include <eo/eoOp.h>

#include <tinyxml/tinyxml.h>

#include "lamarck/structure.h"
#include "opt/types.h"

#include "evaluator.h"
#include "concentration.h"
#include "functors.h"
#include "taboos.h"
#include "gatraits.h"

#ifdef _MPI
#include "mpi/mpi_object.h"
#endif

#include "single_site.h"

#include "gaoperators.h"

/** \ingroop Genetic
 * @{ */
//! \brief Defines base classes for ternary and quaternary semi-conductors
//! \details It is expected the lattice will contain two different sites, one
//for cations, another for anions.  The atoms in the structure are rearranged
//(see TwoSites::rearrange_structure) such that
namespace TwoSites
{
  void rearrange_structure(Ising_CE::Structure &);

  typedef SingleSite :: Object Object;

  struct Fourier
  {
    template<class T_R_IT, class T_K_IT>
    Fourier( T_R_IT _rfirst, T_R_IT _rend,
             T_K_IT _kfirst, T_K_IT _kend );
    template<class T_R_IT, class T_K_IT, class T_O_IT >
    Fourier( T_R_IT _rfirst, T_R_IT _rend,
             T_K_IT _kfirst, T_K_IT _kend,
             T_O_IT _rout ); // sets rvector values from kspace values
  };

  class Concentration : public X_vs_y
  {
    public:
      types::t_real x, y;
      types::t_unsigned N;
      types::t_int Nfreeze_x, Nfreeze_y;
      std::vector<bool> sites;

    public:
      Concentration  () 
                    : X_vs_y(), x(0), y(0), N(0),
                      Nfreeze_x(0), Nfreeze_y(0) {}
      Concentration   ( const Concentration &_conc)
                    : X_vs_y(_conc), x(_conc.x), y(_conc.y),
                      N(_conc.N), Nfreeze_x(_conc.Nfreeze_x), Nfreeze_y(_conc.Nfreeze_y),
                      sites(_conc.sites) {}
      ~Concentration() {}

      bool Load( const TiXmlElement &_node );

      void operator()( Ising_CE::Structure &_str );
      void operator()( Object &_obj );
      void operator()( const Ising_CE::Structure &_str, Object &_object,
                       types::t_int _concx, types::t_int _concy );
      void get( const Ising_CE::Structure &_str);
      void get( const Object &_obj );
      void setfrozen ( const Ising_CE::Structure &_str );

    protected:
      void normalize( Ising_CE::Structure &_str, const types::t_int _site, 
                      types::t_real _tochange);

  };

  template<class T_INDIVIDUAL >
  class Evaluator : public GA::Evaluator< T_INDIVIDUAL >
  {
    public:
      typedef T_INDIVIDUAL t_Individual;
      typedef Traits::GA< Evaluator<t_Individual> > t_GATraits;
    protected:
      typedef typename t_Individual::t_IndivTraits t_IndivTraits;
      typedef typename t_IndivTraits::t_Object t_Object;
      typedef typename t_IndivTraits::t_Concentration t_Concentration;
      typedef GA::Evaluator<t_Individual> t_Base;
      typedef Evaluator<t_Individual> t_This;

    public:
      using t_Base :: Load;

    protected:
      using t_Base :: current_individual;
      using t_Base :: current_object;

    protected:
      typedef Ising_CE::Structure::t_kAtoms t_kvecs;
      typedef Ising_CE::Structure::t_Atoms t_rvecs;
      
    protected:
      Ising_CE::Lattice lattice;
      Ising_CE::Structure structure;
      t_Concentration concentration;

    public:
      Evaluator   () {}
      Evaluator   ( const t_This &_c )
                : lattice( _c.lattice ), structure( _c.structure ) {}
      ~Evaluator() {};


      bool Save( const t_Individual &_indiv, TiXmlElement &_node, bool _type ) const;
      bool Load( t_Individual &_indiv, const TiXmlElement &_node, bool _type );
      bool Load( const TiXmlElement &_node );
      eoGenOp<t_Individual>* LoadGaOp(const TiXmlElement &_el )
       { return GA::LoadGaOp<t_Individual>( _el, structure, concentration ); }
      GA::Taboo_Base<t_Individual>* LoadTaboo(const TiXmlElement &_el )
      {
        if ( concentration.single_c ) return NULL;
        GA::xTaboo<t_Individual> *xtaboo = new GA::xTaboo< t_Individual >( concentration );
        if ( xtaboo and xtaboo->Load( _el ) )  return xtaboo;
        if ( xtaboo ) delete xtaboo;
        return NULL;
      }

      bool initialize( t_Individual &_indiv )
      {
        GA::Random< t_Individual > random( concentration, structure, _indiv );
        _indiv.invalidate(); return true;
      }
      void init( t_Individual &_indiv )
      {
        t_Base :: init( _indiv );
        // sets structure to this object 
        structure << *current_object;
      }

      //! \brief Used to submits individuals to history, etc, prior to starting %GA
      //! \details initializes the endopoints of a convex-hull, for instance.
      //! Presubmitted individuals are not put into the population.
      //! \see GA::Evaluator::presubmit()
      void presubmit( std::list<t_Individual> &_pop );

    protected:
      bool consistency_check();
  };


} // namespace TwoSites

#include "two_sites.impl.h"

/* @} */

#endif // _TWOSITES_OBJECT_H_

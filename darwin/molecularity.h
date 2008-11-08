//
//  Version: $Id$
//
#ifndef _MOLECULARITY_H_
#define _MOLECULARITY_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <ostream>

#include <eo/eoOp.h>

#include <tinyxml/tinyxml.h>

#include <vff/layered.h>
#include <crystal/structure.h>
#include <opt/function_base.h>
#include <opt/gsl_minimizers.h>
#include <opt/types.h>

#include "layered.h"
#include "bandgap_stubs.h"
#include "vff.h"
#include "evaluator.h"
#include "individual.h"
#include "gaoperators.h"

namespace Lada
{

  //! \ingroup Genetic
  //! \brief Decoration search for band-gaps and in-plane-stress of layered structures.
  //! \details This is mostly an application of Layered to band-gap optimization.
  //!          The in-plane-stress ia added as a second objective, so that the
  //!          structure can be grown. 
  //! \warning It is assumed that the lattice contains two sites.
  namespace Molecularity
  {
    //! \brief bitstring object for layered structures, including variables for stress
    //! and band-edges.
    class Object : public Layered::Object<>,
                   public ::GA::Keepers::BandGap,
                   public ::GA::Keepers::ConcOne
    {
      protected:
        //! Layered bitstring base
        typedef Layered :: Object<> t_LayeredBase;  

      public:
        //! see function::Base::t_Type
        typedef t_LayeredBase :: t_Type t_Type; 
        //! see function::Base::t_Container
        typedef t_LayeredBase :: t_Container t_Container;

      public:
        //! Constructor
        Object() : t_LayeredBase(), ::GA::Keepers::BandGap(), ::GA::Keepers::ConcOne() {}
        //! Copy Constructor
        Object   (const Object &_c)
               : t_LayeredBase(_c),
                 ::GA::Keepers::BandGap(_c), 
                 ::GA::Keepers::ConcOne(_c) {};
        //! Destructor
        virtual ~Object() {};
        
        //! Loads strain and band-gap info from XML
        bool Load( const TiXmlElement &_node )
          { return     ::GA::Keepers::BandGap::Load(_node) 
                   and ::GA::Keepers::ConcOne::Load(_node); }
        //! Saves strain and band-gap info to XML
        bool Save( TiXmlElement &_node ) const
          { return     ::GA::Keepers::BandGap::Save(_node)
                   and ::GA::Keepers::ConcOne::Save(_node); }
        //! Serializes a scalar individual.
        template<class Archive> void serialize(Archive & _ar, const unsigned int _version)
        {
          _ar & boost::serialization::base_object< t_LayeredBase >(*this);  
          _ar & boost::serialization::base_object< ::GA::Keepers::BandGap >(*this);  
          _ar & boost::serialization::base_object< ::GA::Keepers::ConcOne >(*this);  
        }
    };

    //! \brief Explicitely defines stream dumping of Molecularity::Object 
    //! \details Modulates the print out for all formats but XML. 
    //! \warning don't touch the dumping to a string, unless you want to change
    //!          read/write to XML. It's got nothing to do with anything here...
    //!          Just repeating myself.
    std::ostream& operator<<(std::ostream &_stream, const Object &_o);
    
    //! \brief %Individual type for Molecularity.
    //! \details The object type is the one above, eg a BitString::Object adapted
    //!          for Layered structures and containing info for stress and
    //!          band-gap. The concentration functor, as well as the Fourier
    //!          transform functors are also specialized for layered objects.
    typedef Individual::Types< Object, 
                               Layered::Concentration<2>, 
                               Layered::Fourier<2>    > :: t_Vector t_Individual;

    //! \brief Evaluator class for band-gap search of a layered structure.
    //! \details Mostly, this class defines  a BandGap::Darwin instance, and a
    //!          Vff::Darwin<Vff::Layered> instance for evaluating (and
    //!          minimizing) in-plane-stress and for evaluating band-gaps.
    class Evaluator : public Layered::Evaluator< t_Individual >
    {
      public:
        //! Type of the individual
        typedef Molecularity::t_Individual     t_Individual;
        //! All %types relevant to %GA
        typedef Traits::GA< Evaluator >        t_GATraits;

        //! Constructor
        Evaluator() : t_Base(), bandgap(structure) {}
        //! Copy Constructor
        Evaluator   ( const Evaluator &_c )
                  : t_Base(_c), bandgap(_c.bandgap) {}
        //! Destructor
        ~Evaluator() {}

        //! Saves an individual to XML
        bool Save( const t_Individual &_indiv, TiXmlElement &_node, bool _type ) const
         { return _indiv.Object().Save(_node) and t_Base::Save( _indiv, _node, _type ); }
        //! Load an individual from XML
        bool Load( t_Individual &_indiv, const TiXmlElement &_node, bool _type );
        //! Loads the lattice, layered structure, bandgap, and vff from XML.
        bool Load( const TiXmlElement &_node );

        //! Computes the band-gap and in-plane-stress of the current_individual.
        void evaluate();
        //! Allows for periodic all-electron computations
        eoF<bool>* LoadContinue(const TiXmlElement &_el );

        //! \brief Intializes before calls to evaluation member routines
        //! \details The bandgap does not need explicit initialization, since it
        //!          will act upon the structure as minimized by vff. More
        //!          explicitely, its "initialization" is carried out in the body
        //!          of Darwin::evaluate().
        void init( t_Individual &_indiv )
          { t_Base :: init( _indiv ); bandgap.init(); }

#       ifdef _MPI
          //! forwards comm and suffix to bandgap.
          void set_mpi( boost::mpi::communicator *_comm, const std::string &_str )
            { t_Base::set_mpi( _comm, _str ); bandgap.set_mpi( _comm, _str ); }
#       endif
        using t_Base :: Load; 

      protected:
        //! Type of this class
        typedef Evaluator                      t_This;
        //! Type of the base class
        typedef Layered::Evaluator<t_Individual> t_Base;
        //! Type of the Vff functional.
        typedef Vff::Layered t_Vff;
        //! Type of the BandGap/Vff all-in-one functional.
        typedef BandGap::Darwin<t_Vff> t_Functional;

      protected:
        //! Transforms stress and band-edges to quantities in \a _indiv.
        void object_to_quantities( t_Individual & _indiv );

        //! The pescan interface object for obtaining band-gaps
        t_Functional bandgap;
        using t_Base :: current_individual;
        using t_Base :: current_object;
    };

  }
} // namespace LaDa

#include "molecularity.impl.h"

#endif // _MOLECULARITY_H_

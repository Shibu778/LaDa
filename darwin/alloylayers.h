//
//  Version: $Id$
//
#ifndef _DARWIN_ALLOY_LAYERS_H_
#define _DARWIN_ALLOY_LAYERS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/lexical_cast.hpp>
#include <string>
#include <ostream>

#include <eo/eoOp.h>

#include <tinyxml/tinyxml.h>

#include <crystal/structure.h>
#include <opt/types.h>
#include <mpi/mpi_object.h>

#include "evaluator.h"
#include "individual.h"
#include "bitstring.h"
#include "gaoperators.h"
#include "vff.h"


namespace GA
{
  /** \ingroup Genetic
   * @{ */
  //! Optimization for superlattices of random alloy layers of specified concentration.
  namespace Layered
  {
    //! \brief Partially overrides GA::Evaluator default to implement behaviors
    //!        relevant to random-alloy layers. 
    //! \tparam T_INDIVIDUAL the type of the GA individual.
    //! \tparam T_TRANSLATE a policy to transforms a GA object into a structure.
    //!                     It should contain a void translate( const t_Object&,
    //!                     Crystal::Structure& ) member, a void translate(
    //!                     const Crystal::Structure&, t_Object& ) member, 
    //!                     a void translate( const std::string&, t_Object&)
    //!                     member, an a void translate( const t_Object&,
    //!                     std::string ).  None of these member need be
    //!                     public.  However, inheritance is further public
    //!                     interfaces. They allow transformation from objects
    //!                     to structure (for use in functional evaluations )
    //!                     and back, and from objects to strings (for
    //!                     serialization) and back. The string need not be
    //!                     human-readable, it is meant  to save the
    //!                     individual's genome in a compressed format. Results
    //!                     are serialized in a human readable manner by using
    //!                     the first two member functions.
    //! \tparam T_ASSIGN a policy which assigns functional values into GA
    //!                  quantities. E.g. sets Individual::quantities to whatever
    //!                  is needed. This extra layer of indirection should allow
    //!                  for us to input into the quantities different functional
    //!                  values as needed, such as bandgap, bandedges, strain...
    //!                  In other words, it allows static and dynamic setting of
    //!                  multi-valued quantities. This policy must contain an
    //!                  void assign( const t_Object&, t_Quantity ) member. It
    //!                  need not be public.  However, inheritance is public to
    //!                  allow further public interfaces.
    template< class T_INDIVIDUAL, class T_TRANSLATE, class T_ASSIGN  >
    class Evaluator : public GA::Evaluator< T_INDIVIDUAL >,
                      public T_TRANSLATE 
                      public T_ASSIGN
    {
      public:
        //! Type of the individual.
        typedef T_INDIVIDUAL t_Individual;
        //! Type of the translation policy
        typedef T_TRANSLATE t_Translate;
        //! Type of the Assignement policy
        typedef T_ASSIGN t_Assign;

      protected:
        //! Traits of the individual.
        typedef typename t_Individual ::t_IndivTraits t_IndivTraits;
        //! Concentration Functor.
        typedef typename t_IndivTraits::t_Concentration t_Concentration;
        //! Evaluator base.
        typedef GA::Evaluator<t_Individual> t_Base;
        //! This type
        typedef Evaluator<t_Individual, t_Translate, t_Assign >     t_This;

      protected:
        using t_Base :: current_object;
        using t_Translate :: tranlate;
        using t_Assign :: assign;

      public:
        using t_Base::Load;

      public:
        //! Constructor
        Evaluator() : t_Base() {}
        //! Copy Constructor
        Evaluator   ( const Evaluator &_c )
                  : t_Base(_c), structure( _c.structure),
                    direction(_c.direction), extent(_c.extent),
                    layer_size(_c.layer_size) {}
        //! Destructor
        virtual ~Evaluator() {}

        //! \brief Loads the lattice, the epitaxial parameters from XML, and constructs
        //!        the structure.
        bool Load( const TiXmlElement &_node );
        //! Loads an individual from XML.
        bool Load( t_Individual &_indiv, const TiXmlElement &_node, bool _type );
        //! Saves an individual to XML.
        bool Save( const t_Individual &_indiv, TiXmlElement &_node, bool _type ) const;

        //! Creates random individuals using GA::Random.
        bool initialize( t_Individual &_indiv );

        //! Sets Layered::Evaluator::Structure to \a _indiv
        void init( t_Individual &_indiv );

        //! Prints conceration attributes and structure
        std::string print() const;

      protected:
        //! Loads epitaxial growth parameters and constructs the structure
        bool Load_Structure( const TiXmlElement &_node );
      
        //! The structure (cell-shape) for which decoration search is done
        Crystal :: Structure structure;
        //! The growth direction.
        atat::iVector3d direction;
        //! The number of alloy layers.
        atat::iVector3d extent;
        //! The size of each layer.
        types::t_real layer_size;
    };


  } // namespace Layered
  /** @} */


}

#include "alloylayers.impl.h"

#endif // _LAYERED_H_

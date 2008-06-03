//
//  Version: $Id$
//
#ifndef _CONSTITTUENT_STRAIN_H_
#define _CONSTITTUENT_STRAIN_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <tinyxml/tinyxml.h>
#include <opt/function_base.h>
#include <opt/types.h>
#include <mpi/mpi_object.h>

#include "atat/vectmac.h"

#include "structure.h"

namespace Ising_CE 
{
  //! Contains everything constituent-strain related.
  namespace ConstituentStrain
  {
    /** \brief Defines the constituent strain.
     *  \details The constituent strain is composed of a set of harmonics
     *           Functional::harmonics, which are applied on a set of
     *           reciprocal-space vectors  Functional::k_vecs. It
     *           evaluates the following sum \f[ \sum_{\overrightarrow{k}}
     *           |S(\overrightarrow{k})|^2 J_{Functional}(x,
     *           \overrightarrow{k})\f], where \f$\overrightarrow{k}\f$ are the
     *           reciprocal space vectors, \f$S(\overrightarrow{k})\f$ are the
     *           structure factors of the structure, \e x is the concentration,
     *           and \f$J_{Functional}(x, \overrightarrow{k})\f$ are the sum of the
     *           harmonics. Each harmonic is an instance of Ising_CE::Harmonic.
     *           
     *           This %function, like other funtion::Base derived type,
     *           interfaces to minimizers through its function::Base::variables
     *           member. In this case, function::Base::variables should contain
     *           the real-space occupations of the lattice-sites, in the same
     *           order as the atoms in Functional::r_vecs.
     *
     *           For more information on harmonics, constituent strain, or the
     *           %Cluster Formalism, you can start here:  <A
     *           HREF="http://dx.doi.org/10.1103/PhysRevB.46.12587"> David B.
     *           Laks, \e et \e al. PRB \b 46, 12587-12605 (1992) </A>.
     *  \warning As with most of the %Cluster Expansion stuff, this class is
     *           specialized for an input cell-shape.
     */
    template< class T_HARMONIC >
    class Functional : public function::Base<types::t_real>
    {
        //! Type of the Harmonics.
        typedef T_HARMONIC t_Harmonic;
        //! Type of the base class
        typedef function::Base<types::t_real> t_Base;

      public: 
        //! The type of the collection of harmonics
        typedef std::vector<t_Harmonic> t_Harmonics;
        //! see function::Base::t_Type
        typedef types::t_real t_Type;
        //! see function::Base::t_Container
        typedef std::vector<types::t_real>  t_Container;

      protected: 
        //! Real-space cartesian coordinates of the structure.
        std::vector<atat::rVector3d> r_vecs;
        //! Reciprocal-space cartesian coordinates of the structure.
        std::vector<atat::rVector3d> k_vecs;
        //! Harmonics functions.
        static t_Harmonics harmonics;
        __MPICODE(
          /** \ingroup mpi
           *  \brief Communicator for parallel computation.
           *  \details During evaluations, the computation over the list of
           *           \b k-vectors is scattered across all processes. **/
          boost::mpi::communicator *comm;
        )
   
      public:
        //! Constructor
        Functional() : t_Base() __MPICONSTRUCTORCODE( comm( ::mpi::main ) ) {}
        //! Constructor and Initializer
        Functional   ( const Ising_CE::Structure& _str,
                       t_Container *_vars=NULL) 
                   : function::Base<t_Type>(_vars)
                     __MPICONSTRUCTORCODE( comm( ::mpi::main ) ) 
          { operator<<( _str ); }

        //! Copy Constructor
        Functional   ( const Functional &_c )
                          : t_Base(_c), r_vecs( _c.r_vecs), k_vecs( _c.k_vecs) 
                            __MPICONSTRUCTORCODE( comm( _c.comm ) ) {}
        //! Destructorq
        ~Functional(){};

     
      public: 
        //! Returns the constituent strain for the current function::Base::variables.
        types::t_real evaluate();
        //! Computes the gradient and stores it in \a _grad.
        void evaluate_gradient(types::t_real* const _grad)
          { evaluate_with_gradient( _grad ); }
        //! \brief Returns the value and computes the gradient for the current !
        //!        function::Base::variables.
        types::t_real evaluate_with_gradient(types::t_real* const gradient);
        //! Return the gradient in direction \a _pos.
        types::t_real evaluate_one_gradient( types::t_unsigned _pos );

      public:
        //! Loads the (static) harmonics from XML.
        bool Load_Harmonics( const TiXmlElement &_element);
        //! Loads the constituent strain from XML.
        bool Load (const TiXmlElement &_element);
        //! Dumps the constituent strain to XML.
        void print_xml( TiXmlElement& _node ) const;

        //! Returns a constatn reference to the reciprocal-space vector collection.
        const std::vector<atat::rVector3d>& get_kvectors() const
            { return k_vecs; }

        #ifdef _LADADEBUG
          //! Debug stuff.
          void check_derivative();
        #endif // _LADADEBUG

  #ifdef _MPI
          /** \ingroup Genetic
           *  \brief Sets the communicator. **/
          void set_mpi( boost::mpi::communicator *_c ) { comm = _c; }
  #endif
        //! Serializes a cluster.
        template<class ARCHIVE> void serialize(ARCHIVE & _ar,
                                               const unsigned int _version);
        void operator<<( const Ising_CE::Structure &_str );
    };

    template<class T_HARMONIC> template<class ARCHIVE>
      void Functional<T_HARMONIC> :: serialize(ARCHIVE & _ar, const unsigned int _version)
      {
        _ar & r_vecs;
        _ar & k_vecs;
        _ar & harmonics;
      }
  } // namespace ConstituentStrain

} // namespace Ising_CE

#include "constituent_strain.impl.h"

#endif // _CONSTITTUENT_STRAIN_H_

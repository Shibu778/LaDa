//
//  Version: $Id$
//
#ifndef _VFF_FUNCTIONAL_LAYERED_H_
#define _VFF_FUNCTIONAL_LAYERED_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "functional.h"


namespace LaDa
{ 
  namespace Vff
  {
    //! Computes in-plane stress from stress matrix \a _stress and plane \a _dir.
    types::t_real inplane_stress( const Eigen::Matrix3d &_stress,
                                  const Eigen::Vector3d &_dir );


    //! \brief Valence Force Field for "layered" structures
    //! \details In this Vff implementation, strain is only allowed in one
    //!          epitaxial growth direction, Layered::direction. In practice,
    //!          this means a few changes to variable packing and unpacking (at
    //!          least were the strain/stress is concerned), as well as
    //!          redefining member functions which make use of packing and
    //!          unpacking.  It is expected that the first unit-cell vector of
    //!          structure (from input) is the direction in which relaxation is
    //!          allowed.
    //! \see Mostly, this class is meant to work with epitaxial structure
    //!      optimization as implemented in Darwin::Molecularity. 
    class Layered : public Functional
    {
      //! The type of the atom  
      typedef Crystal::Structure::t_Atom  t_Atom;
      //! The type of the atom container
      typedef Crystal::Structure::t_Atoms t_Atoms;
      typedef Functional t_Base; //!< Base Class
      public:
        //! Type of the return.
        typedef types :: t_real t_Return;
        //! Type of the container.
        typedef std::vector< types :: t_real > t_Arg;
        //! Type of the gradient argument.
        typedef types :: t_real* t_GradientArg;


      protected:
        //! Type of the container holding the atomic centers
        typedef t_Base::t_Centers t_Centers;  
        //! Type of the atomic centers
        typedef t_Centers::value_type t_Center;  
        //! Type of the container holding the atomic functionals
        typedef t_Base::t_AtomicFunctionals t_AtomicFunctionals;  
        //! Type of the atomic functionals
        typedef t_AtomicFunctionals::value_type t_AtomicFunctional;  

      protected:
        //! Direction in which to allow lattice-cell relaxation
        Eigen::Vector3d direction; 
        //! Direction in which to allow lattice-cell relaxation, normalized
        Eigen::Vector3d u; 
        /** \brief The Strain \f$\hat{S}\f$, as defined in Vff::Functional is
         * \f$\hat{S} = \hat{1}+\epsilon \hat{S}'\f$, with
         * \f$\hat{S}'\f$ the template strain. */
        Eigen::Matrix3d  template_strain; 
        //! Wether epitaxial direction if fixed by input or  structure cell
        bool is_fixed_by_input;
        
      public:
        //! \brief Constructor and Initializer
        //! \param _str structure for which to compute energy and stress
        Layered   ( Crystal :: Structure &_str )
                : t_Base( _str ), direction(0,0,0), u(0,0,0),
                  template_strain(), is_fixed_by_input(false)
          { template_strain.zero(); }
        //! \brief Copy Constructor
        Layered   ( const Layered &_c )
                : t_Base( _c ), direction( _c.direction ), u(_c.u),
                  template_strain(_c.template_strain), 
                  is_fixed_by_input(_c.is_fixed_by_input) {}
        //! \brief Destructor
        ~Layered() {}

        //! \brief unpacks function::Base::variables, then calls energy
        //! \details This function is redeclared, so that it correctly calls the
        //!          correct unpack_variables() member function from this class,
        //!          and not its base. The alternative is to make pack and unpack
        //!          virtual.
        //! \sa function::Base, function::Base::evaluate
        t_Return operator()( const t_Arg& _arg ) const; 
        //! Evaluates a gradient.
        void gradient( const t_Arg& _arg, t_GradientArg _grad ) const;

        //! \brief initializes stuff before minimization
        //! \details Defines the packing and unpacking process, such that only unfrozen
        //! degrees of liberty are known to the minimizer
        //! \sa function::Base, Minimizer::Base
        bool init( t_Arg& _arg );

        //! Prints functional to \a stream.
        void print_out( std::ostream &stream ) const;
        //! Sets epitaxial direction.
        void set_direction( const Eigen::Vector3d& _direction)
        {
          is_fixed_by_input = true;
          direction = _direction;
          create_template_strain();
        } 
        
      protected:
        //! \brief packs variables from minimizer
        //! \details Functional knows about Functional::Structure, whereas minizers now
        //! about function::Base, this function does the interface between the two
        void pack_variables( t_Arg& _arg, const Eigen::Matrix3d& _strain ) const;
        //! \brief unpacks variables from minimizer
        //! \details Functional knows about Functional::Structure, whereas minizers now
        //! about function::Base, this function does the interface between the two
        void unpack_variables(const t_Arg& _arg, Eigen::Matrix3d& strain) const;
        //! \brief packs variables from minimizer
        //! \details Functional knows about Functional::Structure, whereas
        //! minizers now about function::Base, this function does the interface
        //! between the two
        void pack_gradients(const Eigen::Matrix3d& _stress, t_GradientArg _grad) const;


        //! Initializes Layered::u and Layered::template_strain
        void create_template_strain();

        //! Load from XML
        bool Load( const TiXmlElement &_node );

        //! \brief Loads Functional directly from \a _node.
        //! \details If \a _node is not the correct node, the results are undefined.
        bool Load_( const TiXmlElement &_node );
    };


    inline types::t_real inplane_stress( const Eigen::Matrix3d &_stress,
                                         const Eigen::Vector3d &_dir     )
    {
      types::t_real norm = _dir.squaredNorm();
      types::t_real trace = _stress(0,0) + _stress(1,1) + _stress(2,2);
      types::t_real axial = (_dir * (_stress * _dir) ) / norm;
      return ( trace - axial ) * 0.5;
    }

  } // namespace vff 
} // namespace LaDa

#endif // _VFF_FUNCTIONAL_H_

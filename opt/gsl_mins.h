//
//  Version: $Id$
//
#ifndef _GSL_MINS_H_
#define _GSL_MINS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iomanip>
#include <gsl/gsl_multimin.h>
#include <functional>
#include <algorithm>
#include <tinyxml/tinyxml.h>
#include <boost/lambda/bind.hpp>

#include "algorithms.h"
#include "gsl.h"

#ifdef _MPI
#  include <mpi/mpi_object.h>
#endif

namespace Minimizer {

  //! \cond
  namespace details
  {
    template< class T_FUNCTION >
      double gsl_f( const gsl_vector* _x, void* _data );
    template< class T_FUNCTION >
      void gsl_df( const gsl_vector* _x, void* _data, gsl_vector* _grad );
    template< class T_FUNCTION >
      void gsl_fdf( const gsl_vector *_x, void *_params, double *_r, gsl_vector *_grad);
  }
  //! \endcond

  //! \brief Minimizer interfaces for the Gnu Scientific Library
  //! \details Interface to the following algorithms:
  //!         - Fletcher-Reeves conjugate gradient
  //!         - Polak-Ribiere conjugate gradient 
  //!         - vector Broyden-Fletcher-Goldfarb-Shanno algorithm
  //!         - vector Broyden-Fletcher-Goldfarb-Shanno algorithm.
  //!           Second implementation, recommended by GSL manual...
  //!         - Steepest descent
  //!         .
  //! \xmlinput see TagMinimizer
  class Gsl 
  {
    protected:
      //!< Lists all known gsl multidimensional minimizers.
      enum t_gsl_minimizer_type
      { 
        GSL_NONE,   //!< No minizer... 
        GSL_FR,     //!< Fletcher-Reeves conjugate gradient algorithm.
        GSL_PR,     //!< Polak-Ribiere conjugate gradient algorithm.                  
        GSL_BFGS2,  //!< More efficient Broyden-Fletcher-Goldfarb-Shanno algorithm.
        GSL_BFGS,   //!< Broyden-Fletcher-Goldfarb-Shanno algorithm.
        GSL_SD      //!< Steepest Descent algorithm.
      };

    public:
      //! Fletcher-Reeves conjugate gradient algorithm.
      const static t_gsl_minimizer_type FletcherReeves = GSL_FR;
      //! Polak-Ribiere conjugate gradient algorithm.                  
      const static t_gsl_minimizer_type PolakRibiere = GSL_PR;
      //!  More efficient Broyden-Fletcher-Goldfarb-Shanno algorithm.
      const static t_gsl_minimizer_type BFGS2 = GSL_BFGS2;
      //!  Broyden-Fletcher-Goldfarb-Shanno algorithm.
      const static t_gsl_minimizer_type BFGS = GSL_BFGS;
      //!  Steepest Descent algorithm.
      const static t_gsl_minimizer_type SteepestDescent = GSL_SD;

      types::t_real tolerance; //!< Complete convergence
      types::t_real linetolerance; //!< Line convergences
      types::t_real linestep; //!< line step
      types::t_unsigned itermax; //!< maximum number of iterations
      t_gsl_minimizer_type type; //!< maximum number of iterations
      bool verbose; //!< Wether to print out during minimization
                                  
      
    public:
      //! Constructor and Initializer
      Gsl   ( t_gsl_minimizer_type _type, 
              types::t_unsigned _itermax,
              types::t_real _tol, 
              types::t_real _linetol, 
              types::t_real _linestep ) 
            : tolerance(_tol), linetolerance(_linetol),
              linestep(_linestep), itermax(_itermax),
              type( _type ), verbose(false) {}
            
      //! Constructor
      Gsl () : tolerance(types::tolerance),
               linetolerance(0.01),
               linestep(0.1), 
               itermax(500), verbose(false) {}
      //! Destructor
      virtual ~Gsl(){};

      //! Non-XML way to set-up the minimizers.
      void set_parameters( t_gsl_minimizer_type _type, 
                           types::t_unsigned _itermax,
                           types::t_real _tol, 
                           types::t_real _linetol, 
                           types::t_real _linestep );
      //! Minimization functor
      template< class T_FUNCTION >
        typename T_FUNCTION :: t_Return
          operator()( const T_FUNCTION &_func,
                      typename T_FUNCTION :: t_Arg &_arg ) const;
      //! \brief Finds the node - if it is there - which describes this minimizer
      //! \details Looks for a \<Minimizer\> tag first as \a _node, then as a
      //!          child of \a _node. Different minimizer, defined by the
      //!          attribute types are allowed:
      const TiXmlElement* find_node( const TiXmlElement &_node );
      //! \brief Loads Minimizer directly from \a _node.
      //! \details If \a _node is not the correct node, the results are undefined.
      bool Load_( const TiXmlElement &_node );
      //! Loads the minimizer from XML
      bool Load( const TiXmlElement &_node );
      //! Serializes a structure.
      template<class ARCHIVE> void serialize(ARCHIVE & _ar, const unsigned int _version);
  };

  template<typename T_FUNCTION> 
    typename T_FUNCTION :: t_Return 
      Gsl :: operator()( const T_FUNCTION &_func,
                         typename T_FUNCTION :: t_Arg &_arg ) const
    {
      namespace bl = boost::lambda;
      gsl_multimin_fdfminimizer *solver;
 
      __DEBUGTRYBEGIN
 
        if ( verbose ) std::cout << "Starting GSL minimization\n";
 
        gsl_multimin_function_fdf gsl_func;
        gsl_func.f = &details::gsl_f<const T_FUNCTION>;
        gsl_func.df = &details::gsl_df<const T_FUNCTION>;
        gsl_func.fdf =  &details::gsl_fdf<const T_FUNCTION>;
        gsl_func.n = _arg.size();
        gsl_func.params = (void*) &_func;
        
        const gsl_multimin_fdfminimizer_type *T;
        switch( type )
        {
          default:
          case GSL_BFGS2: T = gsl_multimin_fdfminimizer_vector_bfgs2; break;
          case GSL_FR: T = gsl_multimin_fdfminimizer_conjugate_fr; break;
          case GSL_PR: T = gsl_multimin_fdfminimizer_conjugate_pr; break;
          case GSL_BFGS: T = gsl_multimin_fdfminimizer_vector_bfgs; break;
          case GSL_SD: T = gsl_multimin_fdfminimizer_steepest_descent; break;
        }
        solver = gsl_multimin_fdfminimizer_alloc (T, gsl_func.n);
        if (not solver) return false;
        
        ::Gsl::Vector x( _arg );
        gsl_multimin_fdfminimizer_set( solver, &gsl_func, (gsl_vector*) x, 
                                       linestep, linetolerance);
        
        int status;
        double newe(0), olde(0);
        types::t_unsigned iter( 0 );
 
        do
        {
          iter++;
          olde=newe;
          status = gsl_multimin_fdfminimizer_iterate (solver);
 
          if( status ) break;
 
          status = gsl_multimin_test_gradient (solver->gradient, tolerance);
          if(status == GSL_SUCCESS) 
          {
            if( verbose )
              std::cout << "break on gradient small" << std::endl; 
            break; 
          }
 
          newe = gsl_multimin_fdfminimizer_minimum(solver);
          if( verbose )
            std::cout << "  Gsl Iteration " << iter 
                      << ": " << newe << "\n";
        }
        while ( status == GSL_CONTINUE and ( itermax == 0 or iter < itermax ) );
        if( verbose and ( status != GSL_SUCCESS and iter != itermax ) ) 
          std::cout << "Error while minimizing with gsl: "
                    << gsl_strerror( status ) << ".\n";
 
        newe = gsl_multimin_fdfminimizer_minimum( solver );
        if ( verbose )
          std::cout << "Final Iteration: " << newe << std::endl;
    
        gsl_vector *minx = gsl_multimin_fdfminimizer_x( solver );
        types::t_unsigned i(0);
        opt::concurrent_loop
        (
          _arg.begin(), _arg.end(), i,
          bl::_1 = bl::bind( &gsl_vector_get, minx, bl::_2 )
        );
 
        gsl_multimin_fdfminimizer_free (solver);
 
        return newe;
 
      __DEBUGTRYEND
      (
        gsl_multimin_fdfminimizer_free (solver);,
        "Error encountered while minimizing with the GSL library\n"
      )
 
    }  // dummy minimizer


  template<class ARCHIVE>
    void Gsl :: serialize(ARCHIVE & _ar, const unsigned int _version)
    {
       _ar & itermax;
       _ar & tolerance;
       _ar & linetolerance;
       _ar & linestep;
    }
  
  //! \cond
  namespace details
  {
    template< class T_FUNCTION >
      double gsl_f( const gsl_vector* _x, void* _data )
      {
        T_FUNCTION *_this = (T_FUNCTION*) _data;
        return (*_this)( _x->data );
      }
    template< class T_FUNCTION >
      void gsl_df( const gsl_vector* _x, void* _data, gsl_vector* _grad )
      {
        T_FUNCTION *_this = (T_FUNCTION*) _data;
        _this->gradient( _x->data, _grad->data );
        types::t_real result = (*_this)( _x->data );
      }
    template< class T_FUNCTION >
      void gsl_fdf( const gsl_vector *_x, void *_data, double *_r, gsl_vector *_grad)
      { 
        namespace bl = boost::lambda;
        T_FUNCTION *_this = (T_FUNCTION*) _data;
        *_r = (*_this)( _x->data );
        _this->gradient( _x->data, _grad->data );
      } 
  }
  //! \endcond
}

#endif

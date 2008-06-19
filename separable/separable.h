//
//  Version: $Id$
//
#ifndef _SEPARABLE_H_
#define _SEPARABLE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include<vector>
#include<boost/lambda/lambda.hpp>
#include<boost/boost/type_traits/function_traits.hpp>

#include<opt/types.h>

//! Contains all things separable functions.
namespace Separable
{
  // Forward declarations
  //! \cond
  namespace details
  { 

    template< class T_BASE >
      typename T_RETIT::value_type gradient( const T_BASE &_base,
                                             typename T_BASE :: t_Arg _arg );
    template< class T_BASE, class T_ARGSIT, class T_RETIT >
      typename T_RETIT::value_type gradient( const T_BASE &_base,
                                             T_ARGSIT _arg,
                                             T_RETIT _ret  );
    //! \brief Do not use this class. Is here for type identification within
    //!        templates.
    class Base {};
    template< class T_FUNCTION,
              bool arity = boost::function_traits<T_FUNCTION> :: arity >
       class FreeFunction;

    template< class T_TYPE, bool d = false >
      T_TYPE& deref( T_TYPE );
    template< class T_TYPE, bool d = false >
      T_TYPE& deref( T_TYPE );
  }
  //! \endcond

  //! \brief Defines a separable function of many-variables.
  //! \details Programmatically the law for which this function is "separable"
  //!          is defined via \a T_GROUPOP. \a T_BASIS defines a family of 1d functions.
  //!          This function is both one-dimensional when invoked with a
  //!          scalar, and n-dimensional when invoked with iterators.
  //! \tparam T_BASIS is a container of 1d functions. These functions should
  //!         zero order evaluation via a functor call, and grdient evaluation
  //!         via a t_Return gradient( t_Arg ) member function.
  //! \tparam T_GROUPOP is template class defining how to link return values from
  //!         different basis functions together. It will be, generally,
  //!         either std::sum, or std::multiplies.
  //! \tparam T_SCALAROP is template class defining how to link return values from
  //!         a basis function with a scalar coefficient.
  template< class T_BASIS, 
            template<class> class T_GROUPOP  = std::sum, 
            template<class> class T_SCALAROP = std::multiplies >
  class Base : public details :: Base
  {
    public:
      //! Type of the basis
      typedef T_BASIS t_Basis;
      //! Type of the arguments to the one-dimensional functions.
      typedef typename t_Basis::value_type :: t_Arg t_Arg;
      //! Type of the return of the one-dimensional functions.
      typedef typename t_Basis::value_type :: t_Return t_Return;
      //! Type of the operator linking a basis function and its coefficient.
      typedef T_SCALAROP< typename t_Return > t_ScalarOp;
      //! Type of the operator linking to basis functions.
      typedef T_GROUOP< typename t_Return > t_GroupOp;

      //! Whether this function has gradients
      const static bool has_gradient;

      //! Constructor
      Base() : basis(), has_gradient( T_BASIS::has_gradient )
       { coefs.resize( basis.size() ); }
      //! Destructor
      ~Base() {}

      //! Returns the function evaluated at \a _args.
      template< template<class> class T_CONTAINER >
        t_Return operator()( const T_CONTAINER<t_Args> &_args ) const
        { return operator()( _args.begin(), _args.end() ); }
      //! \brief Returns the function evaluated by variables in range 
      //!        \a _first to \a _last.
      //! \details In this case, the function is of as many variables as there
      //!          are functions in the basis.
      template< class T_ITERATOR >
        t_Return operator()( T_ITERATOR _first, T_ITERATOR _last ) const;
      //! \brief Returns the gradient of the one dimensional function.
      t_Return gradient( t_Arg _arg ) const 
        { return has_gradient ? details::gradient( *this, _arg ) : t_Return(0); }
      //! Computes the gradient and stores it in \a _ret.
      template< class T_ARGIT, class T_RETIT >
        void gradient( T_ARGIT _first, T_RETIT _ret ) const
        { if( has_gradient ) details::gradient( *this, _first, _ret); }
      //! \brief Return the function evaluated at \a _arg
      t_Return operator()( t_Arg _arg ) const;
      //! Returns a reference to coeficient _i
      t_Return& operator[]( types::t_unsigned _i ) { return coefs[_i]; }
      //! Returns a reference to coeficient _i
      const t_Return& operator[]( types::t_unsigned _i ) const 
        { return coefs[_i]; }
      //! Sets all coefs in range \a _first to \a _last.
      template< class T_ITERATOR >
        void set( T_ITERATOR _first, T_ITERATOR _last )
      //! Returns a reference to the basis
      t_Basis& Basis() { return basis; }
      //! Returns a constant reference to the basis.
      const t_Basis& Basis() const { return basis; }
      //! Serializes a structure.
      template<class ARCHIVE> void serialize( ARCHIVE & _ar,
                                              const unsigned int _version);

    protected:
      //! A family of functions. 
      Basis basis;
      //! \brief Type of the contained for the coefficients of a single
      //!        separable function.
      typedef std::vector< t_Return > t_Coefs;
      //! A container of coefficients.
      t_Coefs coefs;
      //! Links basis functions.
      t_GroupOp groupop;
      //! Links scalars to basis functions.
      t_ScalarOp scalarop;

    protected:
      //! For alternating least-square purposes.
      template< class T_ITIN, class T_ITOUT>
        t_Return expand( T_ITIN _first, T_ITIN _last, T_ITOUT _out ) const;
  };

  // Forward declaration.
  template< class T_ALLSQ > class AllsqInterface;

  //! Factor of a separable function
  //! \tparam T_BASIS is a container of 1d functions. These functions should
  //!         zero order evaluation via a functor call, and grdient evaluation
  //!         via a t_Return gradient( t_Arg ) member function.
  template< class T_BASIS > class Factor : public Base< T_BASIS >{};
  //! One single separable function.
  //! \tparam T_BASIS is a container of 1d functions. These functions should
  //!         zero order evaluation via a functor call, and grdient evaluation
  //!         via a t_Return gradient( t_Arg ) member function.
  template< class T_BASIS > class Summand :
      public Base< std::vector< Factor<T_BASIS, std::multiplies, std::sum> > >{};
  /** \brief A sum of separable functions.
   * \details The separable function \f$F(\mathbf{x})\f$ acting upon vector
   *          \f$\mathbf{x}\f$ and returning a scalar can be defined as 
   *    \f[
   *        F(\mathbf{x}) = \sum_r \prod_d \sum_i \lambda_{d,i}^{(r)}
   *                        g_{i,n}^{(r)}(x_i),
   *    \f]
   *          with the sum over \e r running over the ranks of the separable
   *          functions, the product over \e i are the separable functions
   *          proper, and the sum_n is an expansion of the factors ver some
   *          family of 1d-functions.
   * \tparam T_BASIS is a container of 1d functions. These functions should
   *         return a zero order evaluation via a functor call, and optionally
   *         a gradient evaluation via a t_Return gradient( t_Arg ) member
   *         function. **/
  template< class T_BASIS >
    class Function : public Base< std::vector< T_BASIS > >
    {
        template<class T_ALLSQ>  friend class AllsqInterface;
        //! Type of the base class.
        typedef Base< std::vector< T_BASIS > > t_Base;
      public:
        //! Type of the basis
        typedef T_BASIS t_Basis;
        //! Type of the arguments to the one-dimensional functions.
        typedef typename t_Basis::value_type :: t_Arg t_Arg;
        //! Type of the return of the one-dimensional functions.
        typedef typename t_Basis::value_type :: t_Return t_Return;
 
      public:
        //! Constructor
        Function() : t_Base() {}
        //! Destructor
        ~Function() {}
 
        //! Returns the function evaluated at \a _args.
        template< template<class> class T_CONTAINER >
          t_Return operator()( const T_CONTAINER<t_Args> &_args ) const
          { return operator()( _args.begin(), _args.end() ); }
        //! \brief Returns the function evaluated by variables in range 
        //!        \a _first to \a _last.
        //! \details In this case, the function is of as many variables as there
        //!          are functions in the basis.
        template< class T_ITERATOR >
          t_Return operator()( T_ITERATOR _first, T_ITERATOR _last ) const;
        //! Computes the gradient and stores in \a _ret.
        template< class T_ARGIT, class T_RETIT >
          void gradient( T_ARGIT _first, T_RETIT _ret ) const;
 
      protected:
        using t_Base::groupop;
        using t_Base::scalarop;
    };


  /** \brief Collapses a sum of separable function into Fitting::Allsq format.
   *  \details This flavor keeps track of computed basis functions
   *           (\f$g_{d,i}^{(r)}\f$).  This is quite a complex process. A sum
   *           of separable functions is expressed as follows: 
   *    \f[
   *        F(\mathbf{x}) = \sum_r \prod_d \sum_i \lambda_{d,i}^{(r)}
   *                        g_{i,n}^{(r)}(x_i),
   *    \f]
   *    We will keep track of the r, i, and n indices. Furthermore, the least
   *    square-fit method fits to  \e o observables. It expects a "2d" input matrix
   *    I[d, (r,i) ], where the slowest running index is the dimension \e d.
   *    I[d] should be a vecrtor container type of scalar values. These scalar
   *    values are orderer with  \e i the fastest running index and \e r the
   *    slowest. The method also expects a functor which can create the matrix
   *    \e A (as in \e Ax = \e b ) for each specific dimension \e d. The type
   *    of A is a vector of scalars. The fastest running index is \e i,
   *    followed by \e r, and finally \e o. 
   *    \tparam T_FUNCTION must be a Function< Summand< T_BASIS > > 
   **/
  template< class T_FUNCTION >
  class Collapse
  {
    public:
      //! Type of the sum of separable functions to collapse.
      typedef T_FUNCTION t_Function;
      //! Wether to update the coefficients between each dimension or not.
      bool do_update;

      //! Constructor
      Collapse   ( T_FUNCTION &_function )
                  : do_update(bool), is_initialized(false), D(0), nb_obs(0),
                    nb_ranks(0), function( _function ) {}

      //! \brief Constructs the matrix \a A for dimension \a _dim. 
      //! \details Note that depending \a _coef are used only for dim == 0,
      //!          and/or do_update == true.
      template< class T_MATRIX, class T_VECTORS >
      void operator()( T_MATRIX &_A, types::t_unsigned _dim, T_VECTORS &_coefs )

      //! \brief Constructs the completely expanded matrix.
      //! \tparams T_VECTORS is a vector of vectors, input[ o, d ] 
      template< class T_VECTORS >
      void init( const T_VECTORS &_x );
      //! Resets collapse functor. Clears memory.
      void reset();
      //! Creates a collection of random coefficients.
      //! \tparams should be a vector of vectors, coefs[d, (r,i) ].
      template< class T_VECTORS >
      void create_coefs( T_VECTORS &_coefs );

    protected:
      //! Initializes Collapse::factors.
      template< class T_MATRIX, class T_VECTORS >
      void initialize_factors( const T_VECTORS &_coefs );
      //! Updates Collapse::factors.
      template< class T_VECTORS >
      void update_factors( types::t_unsigned _dim, const T_VECTORS &_coefs )
      //! Assigns coeficients to function.
      template< class T_VECTORS >
      void reassign( const T_VECTORS &_solution );

    protected:
      //! False if unitialized.
      bool is_initialized;
      //! Maximum dimension.
      types::t_unsigned D;
      //! Number of observables.
      types::t_unsigned nb_obs;
      //! Number of ranks.
      types::t_unsigned nb_ranks;
      //! Reference to the function to fit.
      T_FUNCTION &function;
      //! Return type of the function
      typedef typename T_FUNCTION :: t_Return t_Type;
      //! \brief Type of the matrix containing expanded function elements.
      typedef std::vector< std::vector< t_Type > > t_Expanded;
      //! A matrix with all expanded function elements.
      //! \details expanded[ (o,d), (r,i) ]. The type is a vector of vectors.
      //!          The fastest-running \e internal index is \e i. The
      //!          fastest-running \e external index is d.
      //!          \f$ \text{expanded}[(o,d), (r,i)] = g_{d,i}^{(r)}(x_d^{(o)})\f$.
      t_Expanded expanded;
      //! Type of the factors.
      typedef std::vector< std::vector< t_Type > > t_Factors;
      /** \brief A matrix wich contains the factors of the separable functions.
       *  \details factors[(o,r), d]. A vector of vectors. The \e internal
       *           index is \e d. The fastest-running \e external
       *           index is \e r.
       *           \f$
       *               \text{factors}[d, (r,i)] = \sum_i
       *               \lambda_{d,i}^{(r)}g_{d,i}^{(r)}(x_d^{(o)})
       *           \f$. **/
      t_Factors factors;
      //! Type of the sizes.
      typedef std::vector< std::vector< types::t_unsigned > > t_Sizes;
      //! \brief Sizes of the basis per dimension and rank.
      //! \details sizes[ d, r ] = max i \@ (d,r). r is the fastest running
      //!          vector.
      t_Sizes sizes;
  }

} // end of Separable namespace

#include "separable.impl.h"

#endif

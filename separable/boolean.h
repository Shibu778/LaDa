//
//  Version: $Id$
//
#ifndef _SEPARABLE_BOOLEAN_H_
#define _SEPARABLE_BOOLEAN_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include<iostream>

#include<boost/array.hpp>

#include<opt/debug.h>
#include<opt/types.h>

namespace LaDa
{
  namespace Separable
  {
    //! \cond
    class Boolean;
    //! \endcond

    //! Writes the name of the function to a stream.
    std::ostream& operator<<( std::ostream& _stream, const Boolean& _func );

    //! A boolean basis of one true and one false function.
    class BooleanBasis;

    //! \brief A scalar function which takes a boolean arguments.
    class Boolean
    {
      friend std::ostream& operator<<( std::ostream& _stream, const Boolean& _func );
      friend class BooleanBasis;
      public:
        //! Type of the argument.
        typedef bool t_Arg;
        //! Type of the return.
        typedef types::t_real t_Return;
       
        //! Constructor 
        Boolean ( bool _which = false )
          { set( _which ); }
        //! Copy Constructor 
        Boolean ( const Boolean &_c ) : which( _c.which ), name( _c.name ) {}
        //! Destructor
        ~Boolean() {}

        //! evaluates the function over a range of positions.
        t_Return operator()( const t_Arg _bool ) const
        {
  #       ifdef __DOHALFHALF__
            return  which ? ( _bool ? t_Return(1) : t_Return(0) ): 
                            t_Return(1);
  #       else 
            return  _bool == which ? t_Return( 1 ): t_Return( 0 );
  #       endif
        }
        //! Sets function to true/false or false/true.
        void set( bool _which )
        {
          which = _which;
  #       ifdef __DOHALFHALF__
            name = _which ? "True/False": "True/True"; 
  #       else
            name = _which ? "True/False": "False/True"; 
  #       endif
        }

      protected:
        //! Decides whether this is a true or false function.
        bool which;
        //! name of this function.
        std::string name;
    };
    inline std::ostream& operator<<( std::ostream& _stream, const Boolean& _func )
      {  return _stream << _func.name; }

    class BooleanBasis : public  boost::array< Boolean, 2 >
    {
      public:
        //! Type of argument.
        typedef boost::array< Boolean, 2 > :: value_type :: t_Arg t_Arg;
        //! Type of return.
        typedef boost::array< Boolean, 2 > :: value_type :: t_Return t_Return;
        //! Does not have gradient.
        bool const static has_gradient = false;

      public:
        //! Constructor.
        BooleanBasis() 
          { elems[0].set(true); elems[1].set(false); }
        //! Copy Constructor.
        BooleanBasis( const BooleanBasis &_c ) : boost::array< Boolean, 2 >( _c ) {}
        //! Destructor.
        ~BooleanBasis() {}
    };

  // const bool BooleanBasis :: has_gradient( Boolean :: has_gradient );

  // //! A separable function which takes a boolean arguments.
  // class Separable
  // {
  //   public:
  //     //! Type of the argument.
  //     typedef bool t_Arg;
  //     //! Type of the return.
  //     typedef types::t_real t_Return;
  //     //! Does not have gradient.
  //     bool const static has_gradient;
  //    
  //     //! Constructor 
  //     Separable   ( const t_Atom & _atom ) 
  //               : pos( _atom.pos ), type( _atom.type ),
  //                 coef( 1.0 ), has_gradient(false) {}
  //     //! Destructor
  //     ~Separable() {}
  //
  //     //! evaluates the function over a range of positions.
  //     template< class T_ITERATOR >
  //     t_Return operator()( T_ITERATOR _first, T_ITERATOR _last );
  //
  //   protected:
  //     //! type of the coefficients.
  //     typedef std::pair< t_Return, t_Return > t_Coefficient;
  //     //! Type of the map
  //     typedef std::vector< t_Coefficient > t_Coefficients;
  //     //! A container of coefficients
  //     t_Coefficients coefs;
  // };
  //
  // //! A sum of separable CE functions.
  // class SumSeparable : public ::Separable::Base< Separable >  {};
  // bool Separable::Compare::operator()( const t_Key &_a, const t_Key &_b ) const
  // {
  //   if ( not fuzzy::eq( a[0], b[0] ) )
  //     return fuzzy::le( a[0], b[0] );
  //   if ( not fuzzy::eq( a[1], b[1] ) )
  //     return fuzzy::le( a[1], b[1] );
  //   return fuzzy::le( a[2], b[2] );
  // }
  //
  // void Separable :: add( atat::rVector3d & _pos )
  // {
  //   // position is already in basis.
  //   if( map.find( _pos ) != map.end() ) return;
  //   map[ _pos ] = t_Coefficient( 1,1 );
  // }

  // template< class T_ITERATOR > 
  //   Separable::t_Return Separable :: operator() ( T_ITERATOR _first,
  //                                                 T_ITERATOR _last ) const
  //   {
  //     t_Return result( *_first `);
  //     t_Coefficients :: const_iterator i_coef = coefs.begin();
  //     for(; _first != _last; ++_first, ++i_coef )
  //     {
  //       __ASSERT( i_coef != coefs.end(), "Inconsistent size.\n" );
  //       result *= *_first ? i_coef->first : i_found->second;
  //     }
  //     return 
  //   }
  } // end of Boolean namespace
} // namespace LaDa

#endif //  _SEPARABLE_BOOLEAN_H_

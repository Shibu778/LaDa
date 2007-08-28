#ifndef _GATRAITS_H_
#define _GATRAITS_H_

#include <list>
#include <vector>

#include <eo/eoPop.h>

#ifdef _MPI
#include "mpi/mpi_object.h"
#endif

#include "opt/types.h"
#include "opt/opt_function_base.h"
#include "opt/traits.h"

#ifdef _MPI
#include "mpi/mpi_object.h"
#endif

namespace Traits 
{

  template< class T_CONTAINER, bool SCALAR = true >
  struct VA
  {
      typedef T_CONTAINER       t_Container;
      typedef typename t_Container :: value_type      t_Type;
      typedef function :: Base < t_Type, t_Container > t_Functional;
      typedef std::vector< std::vector< t_Type > > t_QuantityGradients;
  };
  template< class T_CONTAINER >
  struct VA<T_CONTAINER, true>
  {
      typedef T_CONTAINER       t_Container;
      typedef typename t_Container :: value_type      t_Type;
      typedef function :: Base < t_Type, t_Container > t_Functional;
      typedef std::vector< t_Type > t_QuantityGradients;
  };

  template< class T_INDIVIDUAL,
            class T_OBJECT = typename T_INDIVIDUAL :: t_Object,
            class T_QUANTITY_TRAITS = Traits :: Quantity< typename T_OBJECT :: t_Quantity >,
            class T_VA_TRAITS = Traits::VA<typename T_OBJECT :: t_Container, T_QUANTITY_TRAITS :: is_scalar >,
            class T_POPULATION = eoPop< T_INDIVIDUAL >,
            class T_ISLANDS = typename std::list< T_POPULATION > >
  struct Indiv
  {
      typedef T_INDIVIDUAL      t_Individual;
      typedef T_OBJECT          t_Object;
      typedef T_QUANTITY_TRAITS t_QuantityTraits;
      typedef T_VA_TRAITS       t_VA_Traits;
      typedef T_POPULATION      t_Population;
      typedef T_ISLANDS         t_Islands;
      const static bool is_scalar = t_QuantityTraits :: is_scalar;
      const static bool is_vectorial = t_QuantityTraits :: is_vectorial;
  };

  template< class T_EVALUATOR,
            class T_INDIVIDUAL = typename T_EVALUATOR :: t_Individual,
            class T_INDIV_TRAITS = typename Traits::Indiv<T_INDIVIDUAL> >
    struct GA
    {
      typedef T_EVALUATOR t_Evaluator;
      typedef T_INDIVIDUAL t_Individual;
      typedef T_INDIV_TRAITS t_IndivTraits;
      typedef typename t_IndivTraits :: t_QuantityTraits t_QuantityTraits;
      typedef typename t_IndivTraits :: t_VA_Traits t_VA_Traits;
    };
//     typedef T_GATRAITS        t_GA_Traits;
//     typedef typename t_GA_Traits :: t_Evaluator        t_Evaluator;
//     typedef typename t_GA_Traits :: t_Individual       t_Individual;
//     typedef typename t_GA_Traits :: t_IndivTraits      t_IndivTraits;
//     typedef typename t_GA_Traits :: t_QuantityTraits   t_QuantityTraits;
//     typedef typename t_GA_Traits :: t_VAFunctional     t_VAFunctional;
//     typedef typename t_GA_Traits :: t_VAContainer      t_VAContainer;
//     typedef typename t_GA_Traits :: t_VAType           t_VAType;
//     typedef typename t_IndivTraits :: t_Population     t_Population;
//  
    template< class T_CONTAINER >
      void zero_out( T_CONTAINER &_cont )
      {
        size_t size = _cont.size();
        if ( not size ) return;
        typename T_CONTAINER :: iterator i_first = _cont.begin();
        typename T_CONTAINER :: iterator i_end = _cont.end();
        for(; i_first != i_end; ++i_first)
          zero_out( *i_first );
      }
    template<> inline void zero_out<types::t_real>( types::t_real &_cont )
      { _cont = types::t_real(0);  }
    template<> inline void zero_out<types::t_int>( types::t_int &_cont )
      { _cont = types::t_int(0);  }
    template<> inline void zero_out<types::t_unsigned>( types::t_unsigned &_cont )
      { _cont = types::t_unsigned(0);  }
    template<> inline void zero_out<bool>( bool &_cont )
      { _cont = false;  }
    template<> inline void zero_out<std::string>( std::string &_cont )
      { _cont = "";  }

}
#endif

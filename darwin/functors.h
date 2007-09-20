//
//  Version: $Id$
//
#ifndef _DARWIN_FUNCTORS_H_
#define _DARWIN_FUNCTORS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <eo/eoOp.h>
#include <eo/eoContinue.h>
#include <eo/utils/eoRNG.h>
#include <eo/utils/eoState.h>

#include "opt/types.h"
#include "lamarck/structure.h"

#include "gatraits.h"

namespace GA 
{
  template <class A1, class R>
 class const_eoUF : public eoFunctorBase, public std::unary_function<A1, R>
 {
   public:
     typedef A1 t_Argument;
     typedef R  t_Return;

   public:
     virtual ~const_eoUF() {}
   
     virtual t_Return operator()( t_Argument ) const = 0;
   
     static eoFunctorBase::unary_function_tag functor_category()
       { return eoFunctorBase::unary_function_tag(); }
 };

  // generic class for converting member function to binary operators
  template<class T_GATRAITS >
  class mem_monop_t : public eoMonOp<typename T_GATRAITS::t_Individual > 
  {
    public:
      typedef T_GATRAITS t_GATraits;
    protected:
      typedef typename t_GATraits :: t_Evaluator t_Evaluator; 
      typedef typename t_GATraits :: t_Individual t_Individual; 
    public:
      typedef bool ( t_Evaluator :: *t_Function )( t_Individual &);

    private:
      t_Evaluator &class_obj;
      t_Function class_func;
      std::string class_name;

    public:
      explicit
        mem_monop_t   ( t_Evaluator &_co, t_Function _func, const std::string &_cn )
                    : class_obj(_co), class_func(_func), class_name(_cn) {};

      void set_className( std::string &_cn) { class_name = _cn; }
      virtual std::string className() const { return class_name; }

      bool operator() (t_Individual &_object) 
        {  return ( (class_obj.*class_func) )( _object); }

  }; 
  // generic class for converting member function to zero operators
  template<class T_CLASS>
  class mem_zerop_t : public eoF<bool>
  {
    public:
      typedef T_CLASS t_Class; 
      typedef bool ( t_Class::*t_Function )();

    private:
      t_Class &class_obj;
      t_Function class_func;
      std::string class_name;

    public:
      explicit
        mem_zerop_t   ( t_Class &_co, t_Function _func, const std::string &_cn )
                    : class_obj(_co), class_func(_func), class_name(_cn) {};

      void set_className( std::string &_cn) { class_name = _cn; }
      virtual std::string className() const { return class_name; }

      bool operator() () 
        {  return ( (class_obj.*class_func) )(); }

  }; 
  
  // generic class for converting member function to binary genetic operators
  template<class T_EVALUATOR, class T_ARG>
  class mem_bingenop_arg_t : public eoGenOp<typename T_EVALUATOR :: t_Individual >
  {
    public:
      typedef T_EVALUATOR t_Evaluator;
      typedef T_ARG t_Arg; 
    protected:
      typedef typename t_Evaluator :: t_Individual t_Individual; 
    public:
      typedef bool ( t_Evaluator::*t_Function )(t_Individual &, const t_Individual&, t_Arg);

    private:
      t_Evaluator &class_obj;
      t_Arg arg;
      t_Function class_func;
      std::string class_name;

    public:
      explicit
        mem_bingenop_arg_t   ( t_Evaluator &_co, t_Function _func,
                        const std::string &_cn, t_Arg _arg )
                    : class_obj(_co), arg(_arg), class_func(_func), class_name(_cn) {};

      void set_className( std::string &_cn) { class_name = _cn; }
      virtual std::string className() const { return class_name; }

      unsigned max_production(void) { return 1; } 

      void apply(eoPopulator<t_Individual>& _pop)
      {
        t_Individual& a = *_pop;
        const t_Individual& b = _pop.select();
  
        if ( (class_obj.*class_func)(a, b, arg) )
          a.invalidate();
      }
  }; 

  // generic class for converting member function to binary genetic operators
  template<class T_EVALUATOR>
  class mem_bingenop_t : public eoGenOp<typename T_EVALUATOR :: t_Individual> 
  {
    public:
      typedef T_EVALUATOR t_Evaluator;
    protected:
      typedef typename t_Evaluator::t_Individual t_Individual; 
    public:
      typedef bool ( t_Evaluator::*t_Function )(t_Individual &, const t_Individual&);

    private:
      t_Evaluator &class_obj;
      t_Function class_func;
      std::string class_name;

    public:
      explicit
        mem_bingenop_t   ( t_Evaluator &_co, t_Function _func,
                           const std::string &_cn )
                    : class_obj(_co), class_func(_func), class_name(_cn) {};

      void set_className( std::string &_cn) { class_name = _cn; }
      virtual std::string className() const { return class_name; }

      unsigned max_production(void) { return 1; } 

      void apply(eoPopulator<t_Individual>& _pop)
      {
        t_Individual& a = *_pop;
        const t_Individual& b = _pop.select();
  
        if ( (class_obj.*class_func)(a, b) )
          a.invalidate();
      }
  }; 

  template< class T_EVALUATOR, class T_ARGS >
    mem_bingenop_arg_t<T_EVALUATOR, T_ARGS>*
        new_genop( T_EVALUATOR &_eval, 
                   typename mem_bingenop_arg_t<T_EVALUATOR, T_ARGS> :: t_Function _func,
                   const std::string  &_str, T_ARGS _arg )
    {
      return new mem_bingenop_arg_t<T_EVALUATOR, T_ARGS>( _eval, _func, _str, _arg );
    }
  template< class T_EVALUATOR >
    mem_bingenop_t<T_EVALUATOR>* 
      new_genop ( T_EVALUATOR &_eval,
                  typename mem_bingenop_t<T_EVALUATOR> :: t_Function _func,
                  const std::string  &_str )
    {
      return new mem_bingenop_t<T_EVALUATOR>( _eval, _func, _str );
    }


  // a dummy operator which does nothing 
  template< class T_OBJECT >
  class DummyOp : public eoMonOp<T_OBJECT>
  {
    protected:
      typedef T_OBJECT t_Object;

    public:
      DummyOp() {};
      ~DummyOp() {}

      bool operator() ( t_Object &_obj1 )
      { return false; } // do nothing!
  };

  template< class T_GATRAITS >
  class Continuator : public eoContinue< typename T_GATRAITS::t_Individual >
  {
    public:
      typedef T_GATRAITS t_GATraits;
    protected:
      typedef typename t_GATraits::t_Individual t_Individual;
      typedef typename t_GATraits::t_Population t_Population;

    protected:
      eoF<bool> &op;

    public:
      Continuator( eoF<bool> &_op ) : op(_op) {};
      ~Continuator() {}

      bool operator()(const t_Population &_pop )
      {
        return op();
      }
  };


}
#endif // _DARWIN_FUNCTORS_H_

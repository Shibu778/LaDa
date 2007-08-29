//
//  Version: $Id$
//
#ifndef _DARWIN_CHECKPOINT_H_
#define _DARWIN_CHECKPOINT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <iomanip>
#include <algorithm>

#include <eo/eoPop.h>
#include <eo/utils/eoStat.h>
#include <eo/utils/eoMonitor.h>
#include <eo/utils/eoUpdater.h>
#include <eo/eoContinue.h>
#include <eo/utils/eoHowMany.h>

#include "opt/types.h"
#include "print/xmg.h"

#include "taboos.h"
#include "operators.h"
#include "gencount.h"

namespace darwin
{
  template< class T_STORE, class T_EVALUATION >
  class PrintGA : public eoUpdater
  {
    public:
      typedef T_STORE t_Store; 
      typedef T_EVALUATION t_Evaluation; 

    protected:
      const t_Store &store;
      const t_Evaluation &evaluation;
      const GenCount &age;
      bool do_print_each_call;

    public:
      PrintGA   ( const t_Store &_store, const t_Evaluation &_eval, 
                  const GenCount &_age, bool _each )
              : store(_store), evaluation(_eval), age(_age), do_print_each_call(_each) {}
      PrintGA   ( const PrintGA &_c )
              : store(_c.results), evaluation(_c.evaluation), age(_c.age),
                do_print_each_call(_c.do_print_each_call)  {}

      virtual void operator()()
      {
        if ( not ( do_print_each_call or store.newresults() ) )
        {
          Print::xmg << Print::Xmg:: clearall;
          return;
        }

        std::string special = "";
        if ( not store.newresults() ) special = " ? ";
       
        Print::xmg << Print::Xmg::comment << special << "Iteration " << age() 
                         << Print::endl 
                         << Print::Xmg::comment << special << "Evaluation Calls: " 
                         << evaluation.nb_eval << " " << evaluation.nb_grad 
                         << Print::endl;
        
        if( store.newresults() )
          store.print_results( age() );
        Print::xmg << Print::flush;
      }

      // some anoying stuff
      void printOn( std::ostream &__os ) const {};
      void readFrom( std::istream &__os ) const {};
      void lastCall()
      {
        Print::xmg << Print::Xmg::comment << "Last Found Result" << Print::endl;
        store.print_results(age(), true);
        Print::xmg << Print::flush;
      }

      virtual std::string className(void) const { return "darwin::PrintFitness"; }
  };
  template< class T_INDIVIDUAL>
  class PrintFitness : public eoStatBase<T_INDIVIDUAL> 
  {
    public:
      typedef T_INDIVIDUAL t_Individual;
    protected:
      typedef eoPop<t_Individual> t_Population;

    protected:
      GenCount &age;

    public:
      PrintFitness   ( GenCount &_age )
                   : age(_age) {}
      PrintFitness   ( const PrintFitness<t_Individual> &_update )
                   : age(_update.age) {}

      virtual void operator()( const t_Population &_pop )
      {
        types::t_unsigned ga_age = age();
        typename t_Population :: const_iterator i_pop = _pop.begin();
        typename t_Population :: const_iterator i_end = _pop.end();

        for( ; i_pop != i_end; ++i_pop )
          if ( ga_age == i_pop->get_age() )
          {
            std::ostringstream sstr; 
            sstr << "Offspring: " << std::setw(12) << std::setprecision(7)
                 << i_pop->get_concentration() << " "
                 << i_pop->fitness();
            std::string str = sstr.str();
            Print::xmg.add_comment( str );
          }
      }

      // some anoying stuff
      void printOn( std::ostream &__os ) const {};
      void readFrom( std::istream &__os ) const {};
      void lastCall( const eoPop<t_Individual> &_pop) {}

      virtual std::string className(void) const { return "darwin::PrintFitness"; }
  };

  // checks for taboo unconvergence from a breeder, 
  // response. If response does not get through 
  template< class T_INDIVIDUAL>
  class NuclearWinter : public eoStatBase<T_INDIVIDUAL>
  {
    public:
      typedef T_INDIVIDUAL t_Individual;
    protected:
      typedef eoPop<t_Individual> t_Population;

    protected:
      Taboo_Base<t_Individual> &taboo;
      eoGenOp<t_Individual> &normal_ops;
      SequentialOp<t_Individual> nuclear_ops;
      eoGenOp<t_Individual> **breeding_ops;
      types::t_unsigned nuclear_winter_length, nuclear_winter_age;
      bool is_gone_nuclear;
      eoHowMany nuclear_howmany;
      eoHowMany normal_howmany;
      eoHowMany **breeding_howmany;

    public:
      NuclearWinter   ( Taboo_Base<t_Individual> &_taboo, 
                        eoGenOp<t_Individual> &_nops,
                        eoGenOp<t_Individual> &_nwops,
                        types::t_real &_normal_howmany )
                    : taboo(_taboo),
                      normal_ops(_nops),
                      breeding_ops(NULL),
                      nuclear_winter_length(2),
                      is_gone_nuclear(false),
                      nuclear_howmany(1.0),
                      normal_howmany(_normal_howmany),
                      breeding_howmany(NULL)
      {
        nuclear_ops.add(_nops, 1.0);
        nuclear_ops.add(_nwops, 1.0);
      }

      virtual ~NuclearWinter(){};

      void set_howmany( eoHowMany **_howmany)
      {
        breeding_howmany = _howmany;
        *breeding_howmany = &normal_howmany;
      }

      // checks for taboo unconvergenced
      // if true, then transforms each individual according to response
      // if still cannot create new individuals, then returns false
      // and GA exits
      virtual void operator()( const t_Population &_pop )
      {
        if ( is_gone_nuclear )
        { // ends nuclear winter
          ++nuclear_winter_age;
          if ( nuclear_winter_age > nuclear_winter_length ) 
          {
            *breeding_ops = &normal_ops; 
            *breeding_howmany = &normal_howmany;
            is_gone_nuclear = false;
            Print::xmg.add_comment("Nuclear-winter is over");
          }
        }
        else if ( taboo.is_problematic() )
        { // starts nuclear winter
          nuclear_winter_age = 0;
          is_gone_nuclear = true;
          *breeding_ops = &nuclear_ops; 
          *breeding_howmany = &nuclear_howmany;
          taboo.set_problematic(false);
          Print::xmg.add_comment("Going Nuclear");
        }
      }

      void set_op_address( eoGenOp<t_Individual> ** _ops )
        { breeding_ops = _ops; } 

      eoGenOp<t_Individual>* get_op_address() const
      {
        if (is_gone_nuclear)
          return &normal_ops;
        return &nuclear_ops;
      }

      // some anoying stuff
      void printOn( std::ostream &__os ) const {};
      void readFrom( std::istream &__os ) const {};
      virtual void lastCall( const t_Population &_pop) {};
      virtual std::string className(void) const { return "LaDa::NuclearWinter"; }

  };

  template< class T_INDIVIDUAL>
  class UpdateAgeTaboo : public eoStatBase<T_INDIVIDUAL> 
  {
    public:
      typedef T_INDIVIDUAL t_Individual;
    protected:
      typedef eoPop<t_Individual> t_Population;

    protected:
      Taboo< t_Individual, std::list<t_Individual> > & taboo;
      GenCount &age;
      types::t_unsigned max_age;
      types::t_unsigned check_every;
      bool do_print_out;

    public:
      UpdateAgeTaboo  ( Taboo< t_Individual, std::list<t_Individual> > & _taboo,
                        GenCount &_age, types::t_unsigned _max_age, bool _do_print_out = false )
                     : taboo(_taboo), age(_age), max_age(_max_age),
                       do_print_out( _do_print_out )
      {
        check_every = max_age / 10;
        if ( check_every == 0 )
          check_every = 1;
      };
      UpdateAgeTaboo  ( const UpdateAgeTaboo<t_Individual> & _update )
                     : taboo(_update.taboo), age(_update.age), 
                       max_age(_update.max_age), check_every( _update.check_every ),
                       do_print_out(_update.do_print_out) {};

      virtual void operator()( const t_Population &_pop )
      {
        types::t_unsigned ga_age = age();
        if ( ga_age < max_age or ga_age%check_every != 0 )
          return;

        typename t_Population :: const_iterator i_pop = _pop.begin();
        typename t_Population :: const_iterator i_end = _pop.end();

        for( ; i_pop != i_end; ++i_pop )
          if ( ga_age - i_pop->get_age() > max_age )
          {
            taboo.add( *i_pop );
            if ( do_print_out )
            { 
              std::ostringstream sstr;
              sstr << " AgeTaboo: new individual ";
              i_pop->print_out( sstr );
              std::string str = sstr.str();
              Print::xmg.add_comment( str );
            }
          }
      }

      // some anoying stuff
      void printOn( std::ostream &__os ) const {};
      void readFrom( std::istream &__os ) const {};
      virtual void lastCall( const eoPop<t_Individual> &_pop) {}

      virtual std::string className(void) const { return "Darwin::UpdateAgeTaboo"; }
  };

  // when binop( ref, term ), terminates GA
  template< class T_VALUE, class T_BINOP, class T_INDIVIDUAL>
  class Terminator : public eoContinue<T_INDIVIDUAL>
  {
    public:
      typedef T_VALUE t_Value;
      typedef T_BINOP t_BinOp;
      typedef T_INDIVIDUAL t_Individual;
    protected:
      typedef eoPop<t_Individual> t_Population;

    protected:
      t_Value &ref;
      t_Value term;
      t_BinOp binop;
      std::string type;

    public:
      Terminator   ( t_Value &_ref, t_Value _term, t_BinOp _op, 
                     std::string _type)
                 : ref(_ref), term(_term), binop(_op),
                   type(_type) {}
      Terminator   ( const Terminator< t_Value, t_BinOp, t_Individual> & _copy)
                 : ref(_copy.ref), term(_copy.term), binop(_copy.op), 
                   type(_copy.type) {}
      virtual ~Terminator() {}

      virtual bool operator() (const t_Population &_pop )
      {
        if ( (not term ) or ( term and binop( ref, term ) ) )
         return true;
        lastCall();
        return false;
      }
      virtual std::string className(void) const { return "Darwin::Terminator"; }

      void lastCall() const
      {
        std::ostringstream sstr;
        sstr << "Terminator, type: " << type
             << ", ref= " << ref 
             << ", term=" << term;
 
        std::string str = sstr.str();
        Print::xmg.add_comment( str );
      }
  };

  // island continuator. Takes two population iterators and goes through them
  template<class T_INDIVIDUAL, class T_INDIVTRAITS = Traits::Indiv<T_INDIVIDUAL> > 
  class IslandsContinuator : public eoContinue<T_INDIVIDUAL>
  {
    public:
      typedef T_INDIVIDUAL t_Individual;
      typedef T_INDIVTRAITS t_IndivTraits;
    protected:
      typedef typename t_IndivTraits :: t_Population t_Population;
      typedef typename t_IndivTraits :: t_Islands t_Islands;
      typedef typename t_Islands :: iterator iterator;
      typedef typename t_Islands :: const_iterator const_iterator;
    protected:
      std::list < eoContinue<t_Individual>* >       continuators;
      std::list < eoSortedStatBase<t_Individual>* > sorted;
      std::list < eoStatBase<t_Individual>* >       stats;
      std::list < eoMonitor* >                      monitors;
      std::list < eoUpdater* >                      updaters;
      GenCount generation_counter;
      types::t_unsigned  max_generations;
      std::string stop_filename;

    public:
      IslandsContinuator   ( types::t_unsigned _max, std::string _f = "stop" ) 
                         : generation_counter(0),
                           max_generations(_max), stop_filename(_f)
      {
        if ( stop_filename.empty() ) stop_filename = "stop";
        std::ostringstream sstr;
        sstr << "Will stop on finding file " << stop_filename;
        Print::xmg.add_comment( sstr.str() );
      }


      void add(eoContinue<t_Individual>& _cont)
        { continuators.push_back(&_cont); }
      void add(eoSortedStatBase<t_Individual>& _stat)
        { sorted.push_back(&_stat); }
      void add(eoStatBase<t_Individual>& _stat) 
        { stats.push_back(&_stat); }
      void add(eoMonitor& _mon)        
        { monitors.push_back(&_mon); }
      void add(eoUpdater& _upd)        
        { updaters.push_back(&_upd); }

      bool operator()(const t_Population& _pop)
      { return true; }

      // applies population dependant stuff
      void apply_stats(const t_Population& _pop) const
      {
        // first goes through sorted stats
        if ( not sorted.empty() )
        { 
          typename std::list < eoSortedStatBase<t_Individual>* > :: const_iterator i_sorted = sorted.begin();
          typename std::list < eoSortedStatBase<t_Individual>* > :: const_iterator i_end = sorted.end();
          std::vector<const t_Individual*> sorted_pop;
          _pop.sort(sorted_pop);
          for ( ; i_sorted != i_end; ++i_sorted )
            (*i_sorted)->operator()(sorted_pop);
        }
        // then through unsorted stats
        if ( not stats.empty() )
        { 
          typename std::list < eoStatBase<t_Individual>* > :: const_iterator i_stat = stats.begin();
          typename std::list < eoStatBase<t_Individual>* > :: const_iterator i_end = stats.end();
          for ( ; i_stat != i_end; ++i_stat )
            (*i_stat)->operator()(_pop);
        }
      } // bool apply ( ... )

      void apply_monitors_updaters()
      {
        // applies monitors
        if ( not monitors.empty() )
        { 
          typename std::list < eoMonitor* > :: const_iterator i_monitor = monitors.begin();
          typename std::list < eoMonitor* > :: const_iterator i_end = monitors.end();
          for ( ; i_monitor != i_end; ++i_monitor )
            (*i_monitor)->operator()();
        }
        // then through updaters
        if ( not updaters.empty() )
        { 
          typename std::list < eoUpdater* > :: const_iterator i_updater = updaters.begin();
          typename std::list < eoUpdater* > :: const_iterator i_end = updaters.end();
          for ( ; i_updater != i_end; ++i_updater )
            (*i_updater)->operator()();
        }
      }

      bool apply_continuators( const t_Population & _pop ) const
      {
        typename std::list < eoContinue<t_Individual>* > :: const_iterator i_continue = continuators.begin();
        typename std::list < eoContinue<t_Individual>* > :: const_iterator i_end = continuators.end();
        bool result = true;
        for (; i_continue != i_end; ++i_continue )
          if ( not (*i_continue)->operator()( _pop) )
            result = false;
        return result;
      }

      void lastCall( const t_Population& _pop ) 
      {
        // first goes through sorted stats
        if ( not sorted.empty() )
        { 
          typename std::list < eoSortedStatBase<t_Individual>* > :: const_iterator i_sorted = sorted.begin();
          typename std::list < eoSortedStatBase<t_Individual>* > :: const_iterator i_end = sorted.end();
          std::vector<const t_Individual*> sorted_pop;
          _pop.sort(sorted_pop);
          for ( ; i_sorted != i_end; ++i_sorted )
            (*i_sorted)->lastCall(sorted_pop);
        }
        // then through unsorted stats
        if ( not stats.empty() )
        { 
          typename std::list < eoStatBase<t_Individual>* > :: const_iterator i_stat = stats.begin();
          typename std::list < eoStatBase<t_Individual>* > :: const_iterator i_end = stats.end();
          for ( ; i_stat != i_end; ++i_stat )
            (*i_stat)->lastCall(_pop);
        }
        // applies monitors
        if ( not monitors.empty() )
        { 
          typename std::list < eoMonitor* > :: const_iterator i_monitor = monitors.begin();
          typename std::list < eoMonitor* > :: const_iterator i_end = monitors.end();
          for ( ; i_monitor != i_end; ++i_monitor )
            (*i_monitor)->lastCall();
        }
        // then through updaters
        if ( not updaters.empty() )
        { 
          typename std::list < eoUpdater* > :: const_iterator i_updater = updaters.begin();
          typename std::list < eoUpdater* > :: const_iterator i_end = updaters.end();
          for ( ; i_updater != i_end; ++i_updater )
            (*i_updater)->lastCall();
        }
      }

      bool apply ( iterator &_i_begin, iterator &_i_end )
      {
        iterator i_pop = _i_begin;

        for( ; i_pop != _i_end; ++i_pop )
          apply_stats( *i_pop );

        apply_monitors_updaters();

        Print::xmg << Print::flush; 

        bool result =    ( max_generations and generation_counter() < max_generations ) 
                      or ( not max_generations );
        for( i_pop = _i_begin; i_pop != _i_end; ++i_pop )
          if ( not apply_continuators( *i_pop ) )
            result = false;

        // checks if stop file exists
#ifdef _MPI
        if ( mpi::main.is_root_node() )
        {
#endif
        std::ifstream file( stop_filename.c_str(), std::ios_base::in );
        if ( file.is_open() )
        {
          std::ostringstream sstr;
          sstr << "Stopping on finding file " << stop_filename;
          Print::xmg.add_comment( sstr.str() );
          result = false; 
        }

#ifdef _MPI
        }
        result = mpi::main.all_or_all(result);
#endif 

        // last call
        if ( not result )
        {
          for( i_pop = _i_begin; i_pop != _i_end; ++i_pop )
            lastCall( *i_pop );
        }

        // increments generation
        ++generation_counter;

        return result;
      }

      virtual std::string className(void) const { return "LaDa::IslandsContinuator"; }
      GenCount& get_generation_counter() 
        { return generation_counter; }
      types::t_unsigned age() const
        { return generation_counter(); }
  };

  template < class T_CLASS >
  class SaveEvery: public eoUpdater
  {
    public:
      typedef T_CLASS t_Class;
    protected:
      typedef bool ( t_Class::*t_Function )();
    protected:
      t_Class &object;
      t_Function func;
      types::t_unsigned every, age;

    public:
      SaveEvery   ( t_Class &_object, t_Function _func, types::t_unsigned _n ) 
                : object(_object), func(_func), every( _n ), age(0) 
      {
        if ( not every ) every = 1;
      }
      SaveEvery   ( const SaveEvery<t_Class> &_copy )
                : object(_copy.object), func(_copy.func), every( _copy.n ), age(_copy.age) {}

      void operator()()
      {
        ++age;

        if ( age % every )
          return;

        if ( (object.*func)() )
          return;

        std::cerr << "Could not perform save " << std::endl;
      }
  };

#ifdef _MPI
  template< class T_TYPE >
  class Synchronize : public eoUpdater
  {
    public:
      typedef T_TYPE t_Type;
    protected:
      t_Type &object;
      t_Type current_value;
    public:
      explicit
        Synchronize( t_Type &_object ) : object(_object), current_value(_object) {}
      ~Synchronize(){}
      void operator()()
      {
        t_Type diff = object - current_value;
        mpi::main.all_sum_all(diff);
        current_value += diff;
        object = current_value;
      }
  };
#endif

} // namespace LaDa


#endif //  _CHECKPOINT_H_

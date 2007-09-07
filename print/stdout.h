//
//  Version: $Id$
//
#ifndef __PRINT_STDOUT_H_
#define __PRINT_STDOUT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <fstream>
#include <list>

#include "opt/types.h"
#include <revision.h>
#ifdef _MPI
#include "mpi/mpi_object.h"
#endif

#include "operations.h"

namespace Print
{
  class StdOut
  {
    protected:
      bool is_empty;
      bool do_print;
      std::string filename;
      std::ofstream file;

    public:
      StdOut   (const std::string &_f)
             : is_empty(true), do_print(true), filename("")
      { 
#if defined(_MPI) and not defined(_PRINT_ALL_PROCS)
        if ( not mpi::main.is_root_node() ) do_print = false;
#endif
        init_(_f); 
      }
      ~StdOut() { close(); }
      
      bool open();
      void close();

      bool is_set() const { return not filename.empty(); }
      bool is_open() { return do_print and file.is_open(); }
      void init(const std::string &_f) { if ( filename == _f ) return; init_(_f); }
      void set_do_print(bool _print = true) { do_print = _print; }
      template<class T_TYPE> inline StdOut& operator<< ( const T_TYPE &_whatever )
        { operator_( _whatever ); return *this; }
      template<class T_TYPE> inline void operator_ ( const T_TYPE &_whatever )
      {
        if ( not do_print ) return;
        do_checks();
        file << _whatever;
      }
#if defined(_MPI) and not defined(_PRINT_ALL_PROCS)
      void sync_filename() {}
#elif defined(_MPI)
      void sync_filename();
#endif

    private:
      void init_(const std::string &_f);
      void do_checks()
      {
        if ( not is_open() ) open();
        if ( not is_empty ) return;
        
        file << "### " << std::endl 
             << "### Subversion Revision Number " << SVN::Revision << std::endl
             << "### " << std::endl;
        is_empty = false;
      }
        
  };

  template<> inline void StdOut::operator_<Print::t_Operation> ( const Print::t_Operation &_op )
  {
    if ( not do_print ) return;
    do_checks();
    Print::apply_ops( file, _op );
  }
  template<> inline void StdOut::operator_<Print::setw> ( const Print::setw &_w )
  {
    if ( not do_print ) return;
    do_checks();
    _w(file);
  }
  template<> inline void StdOut::operator_<Print::setfill> ( const Print::setfill &_w )
  {
    if ( not do_print ) return;
    do_checks();
    _w(file);
  }
  template<> inline void StdOut::operator_<Print::setprecision> ( const Print::setprecision &_w )
  {
    if ( not do_print ) return;
    do_checks();
    _w(file);
  }

  extern StdOut out;


}

#endif // _PRINT_XMGRACE_H_

//
//  Version: $Id$
//
#include <fstream>

#include <lamarck/atom.h>
#include <opt/va_minimizer.h>

#include "single_site.h"

namespace SingleSite
{
  std::ostream& operator<<(std::ostream &_stream, const Object &_o)
  {
    Object::t_Container :: const_iterator i_var = _o.bitstring.begin();
    Object::t_Container :: const_iterator i_end = _o.bitstring.end();
    for(; i_var != i_end; ++i_var )
      _stream << ( *i_var > 0 ? '1' : '0' );
    return _stream;
  }
  void operator<<(std::string &_str, const Object &_o)
  {
    std::ostringstream sstr;
    sstr << _o; _str = sstr.str();
  }
  void operator<<(Ising_CE::Structure &_str, const Object &_o)
  {
    Object::t_Container :: const_iterator i_var = _o.bitstring.begin();
    Object::t_Container :: const_iterator i_end = _o.bitstring.end();
    Ising_CE::Structure :: t_Atoms :: iterator i_atom = _str.atoms.begin();
    Ising_CE::Structure :: t_Atoms :: iterator i_atom_end = _str.atoms.end();
    for(; i_var != i_end and i_atom != i_atom_end; ++i_atom )
    {
      if ( i_atom->freeze & Ising_CE::Structure::t_Atom::FREEZE_T ) 
        continue;
      i_atom->type = *i_var > 0 ? 1.0 : -1.0;
      ++i_var;
    }
  }

} // SingleSite



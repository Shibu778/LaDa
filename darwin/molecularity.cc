//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "molecularity.h"

  // "Molecularity" means that we try and simulate growth conditions...
  // The input should be a 1x1xn supercell, with n either 100, 110 or 111 
  // each unit-cell is a "molecule", say InAs or GaSb, but not a mix of the two.

namespace LaDa
{

  namespace Molecularity
  {
    bool Evaluator :: Load( t_Individual &_indiv,
                            const TiXmlElement &_node, 
                            bool _type )
    {
      t_Object &object = _indiv.Object();
      if ( not object.Load( _node ) ) return false;

      // set quantity
      object_to_quantities( _indiv );

      return t_Base::Load( _indiv, _node, _type );
    }
    
    bool Evaluator :: Load( const TiXmlElement &_node )
    {
      if ( not t_Base::Load( _node ) ) return false;

      if ( bandgap.Load( _node ) ) return true;

      std::cerr << " Could not load bandgap interface from input!! " << std::endl; 
      return false;
    }

  } // namespace Molecularity
}



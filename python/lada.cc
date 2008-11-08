//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <boost/python.hpp>

#include <revision.h>

#include "atat.hpp"
#include "lattice.hpp"
#include "atom.hpp"
#include "structure.hpp"
#include "physics.hpp"
#include "convexhull.impl.hpp"
#include "errortuple.hpp"
#ifdef __DOCE
#  include "ce.hpp"
#endif

// namespace PythonLaDa
// {
//   void expose_svn()
//   {
//     using namespace boost::python;
//     def( "Revision", &SVN::Revision );
//     def( "Year",     &SVN::Year );
//     def( "Month",    &SVN::Month );
//     def( "Day",      &SVN::Day );
//     def( "User",     &SVN::User );
//   }
// }

BOOST_PYTHON_MODULE(LaDa)
{
  // PythonLaDa::expose_svn();
  LaDa::Python::expose_physics();
  LaDa::Python::expose_atat();
  LaDa::Python::expose_lattice();
  LaDa::Python::expose_atom();
  LaDa::Python::expose_structure();
  LaDa::Python::expose_errors();
  LaDa::Python::exposeConvexHull<boost::python::object>( "ConvexHull" );
# ifdef __DOCE
  LaDa::Python::expose_ce();
# endif
}

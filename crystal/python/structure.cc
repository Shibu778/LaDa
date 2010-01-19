//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sstream>
#include <complex>

#include <boost/shared_ptr.hpp>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/str.hpp>
#include <boost/python/other.hpp>
#include <boost/python/self.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#ifdef _MPI
# include <boost/mpi/python.hpp>
#endif
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/complex.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/filesystem/operations.hpp>

#include <opt/types.h>
#include <opt/debug.h>
#include <python/misc.hpp>
#include <python/xml.hpp>
#include <math/serialize.h>

#include <physics/physics.h>

#include "../structure.h"
#include "../fill_structure.h"
#include "../lattice.h"
#include "../fractional_cartesian.h"

#include "structure.hpp"


namespace LaDa
{
  namespace Python
  {
    namespace bp = boost::python;
    namespace XML
    {
      template<> std::string nodename<Crystal::Structure>()  { return "Structure"; }
      template<> std::string nodename< Crystal::TStructure<std::string> >() 
        { return nodename<Crystal::Structure>(); }
    }

    template< class BULL >
    Crystal::Lattice const* return_crystal_lattice( BULL & )
    {
      if( not Crystal::Structure::lattice )
      {
        PyErr_SetString(PyExc_RuntimeError,
                        "Crystal::Structure::lattice has not been set.\n");
        bp::throw_error_already_set();
        return NULL;
      }
      return Crystal::Structure::lattice; 
    }

    template< class T_STRUCTURE >
      struct pickle_structure : bp::pickle_suite
      {
        static bp::tuple getinitargs( T_STRUCTURE const& _w)  
        {
          return bp::tuple();
        }
        static bp::tuple getstate(const T_STRUCTURE& _in)
        {
          std::ostringstream ss;
          boost::archive::text_oarchive oa( ss );
          oa << _in;

          return bp::make_tuple( ss.str() );
        }
        static void setstate( T_STRUCTURE& _out, bp::tuple state)
        {
          if( bp::len( state ) != 1 )
          {
            PyErr_SetObject(PyExc_ValueError,
                            ("expected 1-item tuple in call to __setstate__; got %s"
                             % state).ptr()
                );
            bp::throw_error_already_set();
          }
          const std::string str = bp::extract< std::string >( state[0] );
          std::istringstream ss( str.c_str() );
          boost::archive::text_iarchive ia( ss );
          ia >> _out;
        }
      };

    bp::str xcrysden( Crystal::Structure const & _str )
    {
      std::ostringstream sstr;
      _str.print_xcrysden( sstr ); 
      return bp::str( sstr.str().c_str() );
    }
    bp::str xcrysden_str( Crystal::TStructure<std::string> const & _str )
    {
      if( not _str.lattice ) return bp::str("");
      std::ostringstream sstr;
      sstr << "CRYSTAL\nPRIMVEC\n" << ( (~_str.cell) * _str.scale ) << "\nPRIMCOORD\n" 
                << _str.atoms.size() << " 1 \n";  
      Crystal::TStructure<std::string>::t_Atoms :: const_iterator i_atom = _str.atoms.begin();
      Crystal::TStructure<std::string>::t_Atoms :: const_iterator i_atom_end = _str.atoms.end();
      for(; i_atom != i_atom_end; ++i_atom )
      {
        sstr << " " << Physics::Atomic::Z(i_atom->type) 
             << " " << ( i_atom->pos * _str.scale ) << "\n";
      }
      return bp::str( sstr.str().c_str() );
    }

    template< class T_STRUCTURE >
      T_STRUCTURE* empty()
      {
        T_STRUCTURE* result = new T_STRUCTURE(); 
        if( result->lattice ) result->scale = result->lattice->scale;
        return result;
      }
    
    template< class T_STRUCTURE >
      T_STRUCTURE* copy( T_STRUCTURE const &_o )
      {
        T_STRUCTURE* result = new T_STRUCTURE( _o ); 
        if( result->lattice ) result->scale = result->lattice->scale;
        return result;
      }

    Crystal::TStructure<std::string>* real_to_string( Crystal::Structure const &_s )
    {
      Crystal::TStructure<std::string>* result = new Crystal::TStructure<std::string>(); 
      Crystal::convert_real_to_string_structure( _s, *result );     
      return result;
    }
    Crystal::Structure* string_to_real( Crystal::TStructure<std::string> const &_s )
    {
      Crystal::Structure* result = new Crystal::Structure(); 
      Crystal::convert_string_to_real_structure( _s, *result );     
      return result;
    }

    template<class T_STRUCTURE> 
      T_STRUCTURE* fromXML(std::string const &_path)
      {
        namespace bfs = boost::filesystem;
        if( not bfs::exists(_path) )
        {
          PyErr_SetString(PyExc_IOError, (_path + " does not exist.\n").c_str());
          bp::throw_error_already_set();
          return NULL;
        }
        T_STRUCTURE* result = new T_STRUCTURE;
        try
        { 
          if(not result->Load(_path))
          {
            PyErr_SetString(PyExc_IOError, ("Could not load structure from " + _path).c_str());
            delete result;
            result = NULL;
            bp::throw_error_already_set();
          }
        }
        catch(std::exception &_e)
        {
          PyErr_SetString(PyExc_IOError, ("Could not load structure from " + _path).c_str());
          delete result;
          result = NULL;
          bp::throw_error_already_set();
        }
        return result;
      }

    template< class T>
      math::rMatrix3d get_cell( T const &_str ) { return _str.cell; }
    template< class T>
      void set_cell( T &_str, math::rMatrix3d const &_cell) {_str.cell = _cell; }

    template<class T_STRUCTURE>
      bp::class_<T_STRUCTURE>   expose( std::string const &_name,
                                        std::string const &_desc, 
                                        std::string const &_type ) 
    {
      return bp::class_<T_STRUCTURE>( _name.c_str(), _desc.c_str() )
        .def( bp::init<T_STRUCTURE const&>() )
        .def( "__init__", bp::make_constructor( fromXML<Crystal::Structure> ) )
        .add_property
        (
          "cell",
          bp::make_function(&get_cell<T_STRUCTURE>, bp::return_value_policy<bp::return_by_value>()),
          &set_cell<T_STRUCTURE>, 
          ("A 3x3 numpy array representing the lattice vector in cartesian units, "
           "in units of self.L{scale<lada.crystal." + _name + ".scale>}.").c_str()
        )
        .def_readwrite( "atoms",   &T_STRUCTURE::atoms,
                        (   "The list of atoms of type L{" + _type
                          + "}, in units of self.{scale<" + _name + "}.").c_str() )
        .def_readwrite( "energy",  &T_STRUCTURE::energy, "Holds a real value." )
        .def_readwrite( "weight",  &T_STRUCTURE::weight,
                        "Optional weight for fitting purposes." )
        .def_readwrite( "scale",   &T_STRUCTURE::scale,
                        "A scaling factor for atomic-positions and cell-vectors." )
        .def_readwrite( "name", &T_STRUCTURE::name, "Holds a string." )
        .def( "__str__",  &print< T_STRUCTURE > ) 
        .def( "fromXML",  &XML::from< T_STRUCTURE >, bp::arg("file"),
              "Loads a structure from an XML file." )
        .def( "toXML",  &XML::to< T_STRUCTURE >, bp::arg("file"),
              "Adds a tag to an XML file describing this structure."  )
        .def( "xcrysden", &xcrysden_str, "Outputs in XCrysden format." )
        .add_property
        ( 
          "lattice",
          bp::make_function
          (
            &return_crystal_lattice< T_STRUCTURE >,
            bp::return_value_policy<bp::reference_existing_object>()
          ),
          "References the lattice within which this structure is defined."
          " Read, but do not write to this object." 
        ) 
        .def_pickle( pickle_structure< T_STRUCTURE >() );
    }

    void expose_structure()
    {
      typedef Crystal::Structure::t_FreezeCell t_FreezeCell;
      bp::enum_<t_FreezeCell>( "FreezeCell", "Tags to freeze cell coordinates." )
        .value( "none", Crystal::Structure::FREEZE_NONE )
        .value(   "xx", Crystal::Structure::FREEZE_XX )
        .value(   "xy", Crystal::Structure::FREEZE_XY )
        .value(   "xz", Crystal::Structure::FREEZE_XZ )
        .value(   "yy", Crystal::Structure::FREEZE_YY )
        .value(   "yz", Crystal::Structure::FREEZE_YZ )
        .value(   "zz", Crystal::Structure::FREEZE_ZZ )
        .value(  "all", Crystal::Structure::FREEZE_ALL )
        .export_values();

      expose< Crystal::Structure >
      (
        "Structure", 
        "Defines a structure.\n\nGenerally, it is a super-cell of a L{Lattice} object.",
        "Atom"
      ).def( "__init__", bp::make_constructor( string_to_real ) )
       .def_readwrite( "freeze", &Crystal::Structure::freeze,
                        "Tags to freeze coordinates when relaxing structure.\n" )
       .def_readwrite( "k_vecs",  &Crystal::Structure::k_vecs,
                       "The list of reciprocal-space vectors."
                       " It is constructure with respected to a LaDa.Lattice object.\n"  ) 
       .def( "concentration", &Crystal::Structure::get_concentration, "Returns concentration." )
       .def( bp::self == bp::other<Crystal::Structure>() );
      expose< Crystal::TStructure<std::string> >
      (
        "sStructure", 
        "Defines a structure.\n\nGenerally, it is a super-cell of a L{Lattice} object.",
        "StrAtom"
      ).def( "__init__", bp::make_constructor( real_to_string ) );

      bp::def("to_cartesian", &Crystal::to_cartesian<std::string>,
              "Transforms a structure from cartesian to fractional coordinates.\n" );
      bp::def("to_fractional", &Crystal::to_fractional<std::string>,
              "Transforms a structure from fractional to cartesian coordinates.\n" );

      bp::register_ptr_to_python< boost::shared_ptr<Crystal::Structure> >();
      bp::register_ptr_to_python< boost::shared_ptr< Crystal::TStructure<std::string> > >();

      bool (*for_real)( Crystal::Structure& ) = &Crystal::fill_structure;
      bool (*for_string)( Crystal::TStructure<std::string>& ) = &Crystal::fill_structure;
      bp::def("fill_structure", for_real, "Fills a structure when atomic positions are unknown." );
      bp::def("fill_structure", for_string, "Fills a structure when atomic positions are unknown." );
    }

  }
} // namespace LaDa

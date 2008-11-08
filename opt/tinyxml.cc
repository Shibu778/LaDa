//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include<boost/filesystem/operations.hpp>
#include<fstream>
#include<sstream>

#include "tinyxml.h"

namespace LaDa
{
  namespace opt
  {
    //! \brief Returns the node \a _name.
    //! \details Looks first to \a _element, then its childrent, then its
    //!          next siblings.
    //! \todo Look amongst all siblings.
    const TiXmlElement* find_node( const TiXmlElement &_element,
                                   const std::string& _name )
    {
      const TiXmlElement *parent;
    
      // Find first XML "Structure" node (may be _element from start, or a child of _element)
      std::string str = _element.Value();
      if ( str.compare(_name) == 0 ) return &_element;
      parent =  _element.FirstChildElement(_name);
      if( parent ) return parent;

      return _element.NextSiblingElement(_name);
    }


    //! \brief Returns the node \<Functional type=\a _name\>.
    //! \details Looks first to \a _element, then its childrent, then its
    //!          next siblings.
    const TiXmlElement* find_functional_node ( const TiXmlElement &_element,
                                               const std::string &_name )
    {
      const TiXmlElement *parent;
      std::string str;

      // This whole section tries to find a <Functional type="vff"> tag
      // in _element or its child
      str = _element.Value();
      if ( str.compare("Functional" ) != 0 )
        parent = _element.FirstChildElement("Functional");
      else parent = &_element;
      
      while (parent)
      {
        str = "";
        if ( parent->Attribute( "type" )  )
          str = parent->Attribute("type");
        if ( str.compare(_name) == 0 )
          break;
        parent = parent->NextSiblingElement("Functional");
      }
      if ( parent ) return parent;
      
      std::cerr << "Could not find an <Functional type=\"" << _name << "\"> tag in input file" 
                << std::endl;
      return NULL;
    }  // Functional :: find_node


    const std::string& read_file( boost::filesystem::path &_input )
    {
      namespace bfs = boost::filesystem;
      __ROOTCODE
      ( 
        (*::LaDa::mpi::main), 
         __DOASSERT( not bfs::exists( _input ), _input << " does not exist.\n" )
         __DOASSERT( not ( bfs::is_regular( input ) or bfs::is_symlink( input ) ),
                     _input << " is a not a valid file.\n" );
        
         std::ifstream file( _input.string().c_str(), std::ios_base::in );
         std::ostringstream out;
         while( not file.eof() ) file >> out;
         file.close();
      )
      __SERIALCODE( return out.str() );
      __MPICODE
      (
        std::string result( out.str() );
        boost::mpi::broadcast( *LaDa::mpi::main, result, 0 );
        return result;
      )
    }

  }
} // namespace LaDa

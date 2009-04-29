//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdexcept>       // std::runtime_error
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define __PROGNAME__ "alloylayers"


#include <revision.h>
#include <opt/debug.h>
#include <opt/bpo_macros.h>
#include <opt/tuple_io.h>
#include <mpi/mpi_object.h>
#ifdef _MPI
# include <boost/mpi/environment.hpp>
#endif

#include "structure.h"
#include "lattice.h"
#include "layerdepth.h"


namespace Crystal = LaDa :: Crystal;
boost::shared_ptr<Crystal::Lattice>
  load_structure( Crystal::Structure& _structure, const std::string &_input )
  {
    TiXmlDocument doc( _input ); 
    TiXmlHandle docHandle( &doc ); 
    __DOASSERT( not doc.LoadFile(),
                   doc.ErrorDesc() << "\n" 
                << "Could not load input file " << _input 
                << " in  Darwin<T_EVALUATOR>.\nAborting.\n" )
 
    const TiXmlElement *node = docHandle.FirstChildElement( "Job" ).Element();
    __DOASSERT( not node, "No structure found in _input file.\n" )
    boost::shared_ptr< Crystal::Lattice > lattice 
      = Crystal::read_lattice( *node );
    Crystal::Structure::lattice = lattice.get();
    _structure.Load( *node );
    _structure.name = "Generated by " __PROGNAME__;
    return lattice;
  }

  void printstructure( const Crystal::Structure &_structure, 
                       const std::string &_filename )
  {
    std::ofstream file; 
    const std::string fxsf( _filename + ".xsf" );
    file.open( fxsf.c_str(), std::ios_base::trunc );
    if (file.fail() )   
      std::cerr << "Could not write to " << _filename << ".xsf" << std::endl;
    else
    {
      _structure.print_xcrysden( file );
      file.close();
    }
    const std::string fxyz( _filename + ".xyz" );
    file.open( fxyz.c_str(), std::ios_base::trunc );
    if (file.fail() )   
      std::cerr << "Could not write to " << _filename << ".xyz" << std::endl;
    else
    {
      _structure.print_xyz( file );
      file.close();
    }
    TiXmlDocument docout; docout.SetTabSize(1);
    docout.LinkEndChild( new TiXmlDeclaration("1.0", "", "") );
    TiXmlElement *jobnode = new TiXmlElement("Job");
    if ( not jobnode )
      { std::cerr << "memory error.\n"; return; }
    docout.LinkEndChild( jobnode );
    _structure.print_xml( *jobnode );
    const std::string fxml( _filename + ".xml" );
    if ( not docout.SaveFile( fxml ) )
      std::cerr << "Could not write to " << _filename << ".xml" << std::endl;
  }

int main(int argc, char *argv[]) 
{
  namespace fs = boost::filesystem;
  namespace bl = boost::lambda;

  __MPI_START__
  __TRYBEGIN

  __BPO_START__;
  __BPO_HIDDEN__;
  __BPO_SPECIFICS__( "Alloy Layers Characterization" )
    // extra parameters for alloy layers.
    ("noorder,n", "Do not perform layer ordering.\n" )
    ("structure", po::value<std::string>(), 
     "Prints epitaxial structure to xml, xyz, xsf files. Does not perform GA." )
    ("direction", po::value<std::string>(), 
     "If growth direction is NOT the first cell vector/column, then set it here." 
     " Note that the periodicity along the direction is implicitely set"
     " by the norm of the direction." );
  __BPO_GENERATE__()
  __BPO_MAP__
  __BPO_PROGNAME__
  __BPO_VERSION__
  if( vm.count("help") )
  {
    __MPICODE( boost::mpi::communicator world; )
    __ROOTCODE( world, \
      std::cout << argv[0] << " is meant to help in creating"
                   " epitaxial structures for use with GA.\n" 
                   "Usage: " << argv[0] << " [options] file.xml\n" 
                << "  file.xml is an optional filename for XML input.\n" 
                << "  Default input is input.xml.\n\n" 
                << all << "\n";
    ) 
    return 0; 
  }


  const bool do_not_order( vm.count("noorder") > 0 );
  fs::path input( vm["input"].as< std::string >() );
  __DOASSERT( not ( fs::is_regular( input ) or fs::is_symlink( input ) ),
              input << " is a not a valid file.\n" );
  
  const bool print_structure = vm.count("structure") > 0;

  Crystal :: Structure structure;
  boost::shared_ptr<Crystal::Lattice> lattice 
    = load_structure( structure, input.string() );
  if(  do_not_order and print_structure )
  { 
    std::cout << "Creating Structure.\n";
    const std::string filename( vm["structure"].as<std::string>() );
    std::cout << "Printing structure to file and exiting.\n"
              << " xml format in " << filename << ".xml\n"
              << " xyz format in " << filename << ".xyz\n"
              << " xsf format in " << filename << ".xsf\n";
    printstructure( structure, filename );
    return 0; 
  } 
  if( do_not_order ) return 0;

  // prints layer info.
  std::cout << "Layered structure characterization\n";
  Crystal::LayerDepth depth( structure.cell );
  if( vm.count("direction") != 0 )
  {
    boost::tuple<LaDa::types::t_real, LaDa::types::t_real, LaDa::types::t_real> vec;
    LaDa::opt::tuples::read<LaDa::types::t_real>( vm["direction"].as<std::string>(), vec );
    depth.set( vec );
  }
  std::sort( structure.atoms.begin(), structure.atoms.end(), depth );

  if( print_structure )
  {
    const std::string filename( vm["structure"].as<std::string>() );
    std::cout << "Printing structure to file and exiting.\n"
              << " xml format in " << filename << ".xml\n"
              << " xyz format in " << filename << ".xyz\n"
              << " xsf format in " << filename << ".xsf\n";
    printstructure( structure, filename );
  }

  typedef Crystal::Structure::t_Atoms::const_iterator t_iatom;
  t_iatom i_atom = structure.atoms.begin();
  t_iatom i_atom_end = structure.atoms.end();
  LaDa::types::t_real last_depth( depth(i_atom->pos) );
  LaDa::types::t_unsigned layer_size(0);
  LaDa::types::t_unsigned nb_layers(0); 
  std::ostringstream  sstr;
  for( ; i_atom != i_atom_end; )
  {
    LaDa::types::t_real new_depth;
    do
    {
      ++layer_size;
      ++i_atom;
      new_depth = depth(i_atom->pos);
    }
    while( LaDa::Fuzzy::eq( last_depth, new_depth ) and i_atom != i_atom_end );
    ++nb_layers;

    sstr << "  _ layer at depth "
         << std::fixed << std::setw(8) << std::setprecision(4)
         << last_depth 
         << " has " << layer_size << " atom"
         << ( layer_size > 1 ? "s.\n": ".\n" );
    
    // Reinitializing layer discovery.
    layer_size = 0;
    last_depth = new_depth;
  }
  if( vm.count("direction") )
    std::cout << "Growth direction: " << vm["direction"].as<std::string>() << "\n";
  else 
    std::cout << "Growth direction: " << structure.cell.get_column(0) << "\n";
  std::cout << "Number of layers: " << nb_layers << "\n"
            << sstr.str() << "\n";

  __BPO_CATCH__( __MPICODE( MPI_Finalize() ) )

  return 0;
}

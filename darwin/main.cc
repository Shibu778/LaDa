//
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdexcept>       // std::runtime_error
#ifdef _MPI
# include <boost/mpi/environment.hpp>
#endif
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <opt/initial_path.h>



#ifdef _PESCAN
# include "bandgap.h"
  typedef LaDa::BandGap :: Evaluator t_Evaluator;
# define __PROGNAME__ "Band-Gap Optimization"
#elif defined(_CE)
# include "groundstate.h"
  typedef LaDa::GroundState :: Evaluator t_Evaluator;
# define __PROGNAME__ "Cluster Expansion Optimization"
#elif defined(_MOLECULARITY)
# include "molecularity.h"
  typedef LaDa::Molecularity :: Evaluator t_Evaluator;
# define __PROGNAME__ "Band-Gap Optimization for Epitaxial Structure"
#elif defined(_EMASS)
# include "emass.h"
  typedef LaDa::eMassSL :: Evaluator t_Evaluator;
# define __PROGNAME__ "emass_opt"
#elif defined( _ALLOY_LAYERS_ )
# include "alloylayers/main.extras.h"
#else 
# error Need to define _CE or _PESCAN or _MOLECULARITY or _ALLOY_LAYERS_
#endif

#include <revision.h>
#include <print/xmg.h>
#include <print/stdout.h>
#include <opt/debug.h>
#include <mpi/mpi_object.h>
#include <opt/bpo_macros.h>
#include <opt/initial_path.h>

#include "individual.h"
#include "darwin.h"

int main(int argc, char *argv[]) 
{
  namespace fs = boost::filesystem;
  namespace bl = boost::lambda;
  LaDa::opt::InitialPath::init();

  __MPI_START__
  __TRYBEGIN

  __BPO_START__;
  __BPO_HIDDEN__;
  __BPO_SPECIFICS__( "GA Options" )
    // extra parameters for alloy layers.
#   include "alloylayers/main.extras.h" 
    __BPO_RERUNS__;
  __BPO_GENERATE__()
  __BPO_MAP__
  __BPO_HELP_N_VERSION__


  fs::path input( vm["input"].as< std::string >() );
  __DOASSERT( not ( fs::is_regular( input ) or fs::is_symlink( input ) ),
              input << " is a not a valid file.\n" );
  const unsigned reruns = vm["reruns"].as< unsigned >();
  
  __ROOTCODE
  (
    (*::LaDa::mpi::main),
    std::cout << "Will load input from file: " << input << ".\n";
  )
    
  // Reads program options for alloylayers.
  // Prints program options to standard output.
#   include "alloylayers/main.extras.h" 
 
  __ROOTCODE
  (
    (*::LaDa::mpi::main),
    std::cout << "Will perform " << reruns << " GA runs.\n\n";
  )
  
  for( LaDa::types::t_int i = 0; i < reruns; ++i )
  {
    LaDa::GA::Darwin< t_Evaluator > ga;
    LaDa::Print :: out << "load result: " << ga.Load(input.string()) << LaDa::Print::endl; 

    // Prints program options to Print::out and Print::xmg.
#   include "alloylayers/main.extras.h" 
    // Connects assignement and print functors for alloylayers config. space.
#   include "alloylayers/main.extras.h" 
    
    LaDa::Print::out << "Rerun " << i+1 << " of " << reruns << LaDa::Print::endl;
    __ROOTCODE
    (
      (*::LaDa::mpi::main),
      std::cout << "Rerun " << i+1 << " of " << reruns << ".\n";
    )
    ga.run();
    // for reruns.
    LaDa::Print::xmg.dont_truncate();
    LaDa::Print::out.dont_truncate();
  }
  
  __BPO_CATCH__( __MPICODE( MPI_Finalize() ) )

  return 0;
}

# //
//  Version: $Id$
//
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fstream>
#include <sstream>
#include <string>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>

#include <tinyxml/tinyxml.h> 

#include <minimizer/allsq.h>
#include <minimizer/cgs.h>
#include <opt/types.h>
#include <math/fuzzy.h>
#include <opt/debug.h>
#include <opt/errors.h>

#include "cefitting.h"
#include "cecollapse.h"
#include "bestof.h"
#include <opt/leave_many_out.h>

#define __PROGNAME__ "Fixed-Lattice Sum of Separable functions" 

const LaDa::types::t_unsigned print_reruns   = 1;
const LaDa::types::t_unsigned print_checks   = 2;
const LaDa::types::t_unsigned print_function = 3;
const LaDa::types::t_unsigned print_allsq    = 4;
const LaDa::types::t_unsigned print_data     = 5;
const LaDa::types::t_unsigned print_llsq     = 6;

int main(int argc, char *argv[]) 
{
  try
  {
    namespace po = boost::program_options;

    LaDa::Fitting::LeaveManyOut leavemanyout;

    po::options_description generic("Generic Options");
    generic.add_options()
           ("help,h", "produces this help message.")
           ("version,v", "prints version string.")
           ("verbose,p", po::value<LaDa::types::t_unsigned>()->default_value(0),
                         "Level of verbosity.\n"  )
           ("seed", po::value<LaDa::types::t_unsigned>()->default_value(0),
                    "Seed of the random number generator.\n"  )
           ("reruns", po::value<LaDa::types::t_unsigned>()->default_value(1),
                      "number of times to run the algorithm.\n" 
                      "Is equivalent to manually re-launching the program.\n");
    po::options_description specific("Separables Options");
    specific.add_options()
        ("cross,c", "Performs leave-one-out"
                    " cross-validation, rather than simple fit.\n"  )
        ("size,s", po::value<LaDa::types::t_unsigned>()->default_value(3),
                   "Size of the cubic basis." )
        ("rank,r", po::value<LaDa::types::t_unsigned>()->default_value(3),
                   "Rank of the sum of separable functions." )
        ("basis,r", po::value<std::string>(),
                   "Description of the ranks/size of the figure used\n." )
        ("tolerance", po::value<LaDa::types::t_real>()->default_value(1e-4),
                      "Tolerance of the alternating linear-least square fit.\n"  )
        ("maxiter,m", po::value<LaDa::types::t_unsigned>()->default_value(40),
                      "Maximum number of iterations for"
                      " Alternating linear-least square fit.\n"  )
        ("1dtolerance", po::value<LaDa::types::t_real>()->default_value(1e-4),
                        "Tolerance of the 1d linear-least square fit.\n" )
        ("noupdate", "Whether to update during 1d least-square fits.\n" )
        ("conv", "Use conventional cell rather than unit-cell.\n"
                 "Should work for fcc and bcc if lattice is inputed right.\n" )
        ("random", po::value<LaDa::types::t_real>()->default_value(5e-1),
                   "Coefficients will be chosen randomly in the range [random,-random].\n" )
#       ifdef __DOHALFHALF__
          ("lambda,l", po::value<LaDa::types::t_real>()->default_value(0),
                       "Regularization factor.\n" )
#       endif
        ("nbguesses", po::value<LaDa::types::t_unsigned>()->default_value(1),
                      "Number of initial guesses to try prior to (any) fitting.\n" );
    leavemanyout.add_cmdl( specific );
    po::options_description hidden("hidden");
    hidden.add_options()
        ("offset", po::value<LaDa::types::t_real>()->default_value(0e0), 
                   "Adds an offset to the energies.\n" )
        ("prerun", "Wether to perform real-runs, or small pre-runs followed"
                   " by a a longer, converged run.\n" )
        ("datadir", po::value<std::string>()->default_value("./"))
        ("latticeinput", po::value<std::string>()->default_value("input.xml"));
 
    po::options_description all;
    all.add(generic).add(specific);
    po::options_description allnhidden;
    allnhidden.add(all).add(hidden);
 
    po::positional_options_description p;
    p.add("datadir", 1);
    p.add("latticeinput", 2);
 
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(allnhidden).positional(p).run(), vm);
    po::notify(vm);
 
    std::cout << "\n" << __PROGNAME__
              << " from the " << PACKAGE_STRING << " package.\n";
    if ( vm.count("version") ) return 1;
    if ( vm.count("help") )
    {
      std::cout << "Usage: " << argv[0] << " [options] DATADIR LATTICEINPUT\n"
                   "  _ DATATDIR (=./) is an optional path to the"
                   " training set input.\n"
                   "  _ LATTICEINPUT (=input.xml) is an optional filename"
                   " for the file\n"
                   "                 from which to load the lattice. "
                   "LATTICEINPUT should be\n"
                   "                 a full path or a relative path "
                   "starting from the current\n"
                   "                 directory, or a relative path "
                   "starting from the DATADIR\n"
                   "                 directory (checked in that order.)\n\n" 
                << all << "\n"; 
      return 1;
    }

    std::string dir(".");
    if( vm.count("datadir") ) dir = vm["datadir"].as< std::string >();
    std::string filename("input.xml");
    if( vm.count("latticeinput") ) filename = vm["latticeinput"].as< std::string >();

    LaDa::types::t_unsigned verbose = vm["verbose"].as<LaDa::types::t_unsigned>();
    LaDa::types::t_unsigned seed = vm["seed"].as<LaDa::types::t_unsigned>();
    seed = LaDa::opt::math::seed( seed );
    LaDa::types::t_unsigned reruns(1);
    if( vm.count("reruns") ) reruns = vm["reruns"].as< LaDa::types::t_unsigned >();
    __DOASSERT( reruns == 0, "0 number of runs performed... As required on input.\n" )
    bool cross = vm.count("cross");
    LaDa::types::t_unsigned rank( vm["rank"].as< LaDa::types::t_unsigned >() );
    __DOASSERT( rank == 0, "Separable function of rank 0 is obnoxious.\n" )
    LaDa::types::t_unsigned size( vm["size"].as< LaDa::types::t_unsigned >() );
    __DOASSERT( size == 0, "Separable function of dimension 0 is obnoxious.\n" )
    LaDa::types::t_real tolerance( vm["tolerance"].as< LaDa::types::t_real >() );
    LaDa::types::t_unsigned maxiter( vm["maxiter"].as< LaDa::types::t_unsigned >() );
    LaDa::types::t_real dtolerance( vm["1dtolerance"].as< LaDa::types::t_real >() );
    bool doupdate = not vm.count("noupdate");
    bool convcell = vm.count("conv");
    LaDa::types::t_real offset ( vm["offset"].as< LaDa::types::t_real >() );
    if( LaDa::math::eq( offset, LaDa::types::t_real(0) ) ) offset = LaDa::types::t_real(0);
    bool prerun ( vm.count("prerun") != 0 );
    LaDa::types::t_real howrandom( vm["random"].as<LaDa::types::t_real>() );
    std::string bdesc("");
    if( vm.count("basis") == 1 )
      bdesc = vm["basis"].as<std::string>();
#   ifdef __DOHALFHALF__
      LaDa::types::t_real lambda( vm["lambda"].as<LaDa::types::t_real>() );
#   endif
    LaDa::types::t_unsigned nbguesses( vm["nbguesses"].as<LaDa::types::t_unsigned>() );
    __ASSERT( nbguesses == 0, "Invalid input nbguesses = 0.\n" )

    // Loads lattice
    boost::shared_ptr< LaDa::Crystal::Lattice > lattice( new LaDa::Crystal::Lattice );
    { 
      TiXmlDocument doc;
      if( boost::filesystem::exists( filename ) )
      {
        __DOASSERT( not doc.LoadFile( filename ), 
                     "Found " << filename << " but could not parse.\n"
                  << "Possible incorrect XML syntax.\n" 
                  << doc.ErrorDesc()  )
      }
      else 
      {
        boost::filesystem::path path( dir );
        boost::filesystem::path fullpath = path / filename;
        __DOASSERT( not boost::filesystem::exists( fullpath ),
                     "Could not find "<< filename 
                  << " in current directory, nor in " <<  path )
        __DOASSERT( not doc.LoadFile( fullpath.string() ),
                     "Could not parse " << fullpath 
                  << ".\nPossible incorrect XML syntax.\n"
                  << doc.ErrorDesc()  )
      }
      TiXmlHandle handle( &doc );
      TiXmlElement *child = handle.FirstChild( "Job" )
                                  .FirstChild( "Lattice" ).Element();
      __DOASSERT( not child, "Could not find Lattice in input." )
      LaDa::Crystal::Structure::lattice = &( *lattice );
      __DOASSERT( not lattice->Load(*child),
                  "Error while reading Lattice from input.")
#     if defined (_TETRAGONAL_CE_)
        // Only Constituent-Strain expects and space group determination
        // expect explicitely tetragonal lattice. 
        // Other expect a "cubic" lattice wich is implicitely tetragonal...
        // Historical bullshit from input structure files @ nrel.
        for( LaDa::types::t_int i=0; i < 3; ++i ) 
          if( LaDa::math::eq( lattice->cell.x[i][2], 0.5e0 ) )
            lattice->cell.x[i][2] = 0.6e0;
#     endif
      lattice->find_space_group();
#     if defined (_TETRAGONAL_CE_)
        // Only Constituent-Strain expects and space group determination
        // expect explicitely tetragonal lattice. 
        // Other expect a "cubic" lattice wich is implicitely tetragonal...
        // Historical bullshit from input structure files @ nrel.
        for( LaDa::types::t_int i=0; i < 3; ++i ) 
          if( LaDa::math::eq( lattice->cell.x[i][2], 0.6e0 ) )
            lattice->cell.x[i][2] = 0.5e0;
#     endif
    }

    // Initializes fitting.
    typedef LaDa::Fitting::Allsq<LaDa::Fitting::Cgs> t_Fitting;
    t_Fitting allsq;

    LaDa::Separables::BestOf< t_Fitting > bestof;
    bestof.n = reruns;
    bestof.verbose = verbose >= print_reruns;
    bestof.prerun = prerun;

    bestof.t_Fitting::itermax = maxiter;
    bestof.t_Fitting::tolerance = tolerance;
    bestof.t_Fitting::verbose = verbose >= print_allsq;
    bestof.t_Fitting::do_update = doupdate;
    bestof.t_Fitting::linear_solver.tolerance = dtolerance;
    bestof.t_Fitting::linear_solver.verbose = verbose >= print_llsq;

    // Initializes the symmetry-less separable function.
    typedef LaDa::CE::Separables t_Function;
    t_Function separables( rank, size, convcell ? "conv": "cube" );
    if( not bdesc.empty() )
    {
      const boost::regex re("(\\d+)(?:\\s+)?x(?:\\s+)?(\\d+)(?:\\s+)?x(?:\\s+)?(\\d+)" );
      boost::match_results<std::string::const_iterator> what;
      __DOASSERT( not boost::regex_search( bdesc, what, re ),
                  "Could not parse --basis input: " << bdesc << "\n" )
      LaDa::Eigen::Matrix3d cell;
      cell.set_diagonal( boost::lexical_cast<LaDa::types::t_real>(what.str(1)),
                         boost::lexical_cast<LaDa::types::t_real>(what.str(2)),
                         boost::lexical_cast<LaDa::types::t_real>(what.str(3)) );
      separables.set_basis( cell );
    }
    
    // Initializes cum-symmetry separable function.
    LaDa::CE::SymSeparables symsep( separables );

    // Initializes collapse functor.
    LaDa::Separable::EquivCollapse< t_Function > collapse( separables );
#   ifdef __DOHALFHALF__
      collapse.regular_factor = lambda;
#   endif

    // Initializes Interface to allsq.
      LaDa::Fitting::SepCeInterface interface;
    interface.howrandom = howrandom;
    interface.nb_initial_guesses = nbguesses;
    interface.verbose = verbose >= 4;
    interface.set_offset( offset );
    interface.read( symsep, dir, "LDAs.dat", verbose >= print_data );
#   if defined (_TETRAGONAL_CE_)
      // From here on, lattice should be explicitely tetragonal.
      for( LaDa::types::t_int i=0; i < 3; ++i ) 
        if( LaDa::math::eq( lattice->cell.x[i][2], 0.5e0 ) )
          lattice->cell.x[i][2] = 0.6e0;
#   endif

    // extract leave-many-out commandline
    leavemanyout.extract_cmdl( vm );
    leavemanyout.verbosity = verbose;

    LaDa::opt::NErrorTuple nerror( interface.mean_n_var() ); 
    

    typedef LaDa::Fitting::SepCeInterface::t_PairErrors t_PairErrors;

    std::cout << "Performing " << (cross ? "Cross-Validation" : "Fitting" ) << ".\n"
              << "Using " << ( convcell ? "conventional ": "unit-" )
                 << "cell for basis determination.\n";
    if( not bdesc.empty() )
      std::cout << "Shape of separable function: " << bdesc << "\n";
    else std::cout << "Size of a separable function " << separables.size() << "\n";
    std::cout << "Rank of the sum of separable functions: " << rank << "\n"
              << "d.o.f.: " << separables.size() * rank << "\n"
              << "Data directory: " << dir << "\n";
    if( reruns <= 1 )  std::cout << "single";
    else               std::cout << reruns;
    std::cout << " Run" << (reruns <= 1 ? ".": "s." )  << "\n";
    if( not verbose ) std::cout << "Quiet output.\n";
    else std::cout << "Level of verbosity: " << verbose << "\n";
    std::cout << "Alternating linear-least square tolerance: " 
                 << tolerance << "\n"
              << "Maximum number of iterations for alternating least-square fit: "
                 << maxiter << "\n"
              << "1d linear-least square tolerance: " 
                 << dtolerance << "\n"
              << "Will" << ( doupdate ? " ": " not " )
                 << "update between dimensions.\n"
              << "Data mean: " << nerror.nmean() << "\n"
              << "Data Variance: " << nerror.nvariance() << "\n"
              << "Range of initial guesses:[ " <<  howrandom << ", " << howrandom << "].\n"
              << "Number of initial guesses: " <<  nbguesses << ".\n";
    if( prerun )
     std::cout << "Performing prerun.\n";
    std::cout << "Random Seed: " << seed << "\n";
#   ifdef __DOHALFHALF__
      if( LaDa::math::gt( lambda, 0e0 ) )
        std::cout << "Regularizing with factor: " << lambda << "\n";
      std::cout << "Using True/False and True/True inner basis.\n"; 
#   else
      std::cout << "Using True/False and False/True inner basis.\n"; 
#   endif

    if( LaDa::Fuzzy :: neq( offset, 0e0 ) ) std::cout << "Offset: " << offset << "\n";

    // fitting.
    if( leavemanyout.do_perform )
    {
      std::cout << "\nStarting leave-many out predictive fit." << std::endl;
      LaDa::Fitting::LeaveManyOut::t_Return result;
      result = leavemanyout( interface, bestof, collapse );
      std::cout << " Training errors:\n" << ( nerror = result.first ) << "\n"
                << " Prediction errors:\n" << ( nerror = result.second ) << "\n";
    }
    else if( not cross )
    {
      std::cout << "\nFitting using whole training set:" << std::endl;
      interface.fit( bestof, collapse );
      nerror = interface.check_training( separables, verbose >= print_checks );
      std::cout << nerror << "\n"; 
    }
    else
    {
      std::cout << "\nLeave-one-out prediction:" << std::endl;
      std::pair< LaDa::opt::ErrorTuple, LaDa::opt::ErrorTuple > result;
      result = LaDa::Fitting::leave_one_out( interface, bestof, collapse,
                                       verbose >= print_checks );
      std::cout << " Training errors:\n" << ( nerror = result.first ) << "\n"
                << " Prediction errors:\n" << ( nerror = result.second ) << "\n";
    }
    if( verbose >= print_function ) std::cout << separables << "\n";

    std::cout << "\n\n\nEnd of " << __PROGNAME__ << ".\n" << std::endl;

  }
  catch ( boost::program_options::invalid_command_line_syntax &_b)
  {
    std::cout << "Caught error while running " << __PROGNAME__ << "\n"
              << "Something wrong with the command-line input.\n"
              << _b.what() << std::endl;
    return 0;
  }
  catch ( boost::program_options::invalid_option_value &_i )
  {
    std::cout << "Caught error while running " << __PROGNAME__ << "\n"
              << "Argument of option in command-line is invalid.\n"
              << _i.what() << std::endl;
    return 0;
  }
  catch ( boost::program_options::unknown_option &_u)
  {
    std::cout << "Caught error while running " << __PROGNAME__ << "\n"
              << "Unknown option in command-line.\n"
              << _u.what() << std::endl;
    return 0;
  }
  catch (  boost::program_options::too_many_positional_options_error &_e )
  {
    std::cout << "Caught error while running " << __PROGNAME__ << "\n"
              << "Too many arguments in command-line.\n"
              << _e.what() << std::endl;
    return 0;
  }
  catch ( std::exception &e )
  {
    std::cout << "Caught error while running " << __PROGNAME__ 
              << "\n" << e.what() << std::endl;
    return 0;
  }
  return 1;
}


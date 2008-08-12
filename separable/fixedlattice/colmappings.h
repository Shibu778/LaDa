//
//  Version: $Id$
//
#ifndef _CE_COLMAPPINGS_H_
#define _CE_COLMAPPINGS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include<boost/lambda/bind.hpp>
#include<boost/numeric/ublas/vector.hpp>
#include<vector>

#include <opt/types.h>
#include <opt/debug.h>

namespace CE
{
  namespace Mapping
  {
    class SymEquiv
    {
      public:
        //! Type of vectors.
        typedef std::vector<types::t_real> t_Vector;

        //! Constructor.
        SymEquiv() {}
        //! Copy Constructor.
        SymEquiv(const SymEquiv& ) { __DOASSERT(true, "No Copy.\n" ) }
        //! Destructor
        ~SymEquiv() {}
        //! Returns weights.
        t_Vector& weights() { return weights_; }
        //! Returns weights.
        const t_Vector& weights() const { return weights_; }
        //! Returns one weight.
        t_Vector::value_type weight( size_t i ) const { return weights_[i]; }
        //! Returns weights.
        t_Vector& targets() { return targets_; }
        //! Returns weights.
        const t_Vector &targets() const { return targets_; }
        //! Returns one weight.
        t_Vector::value_type target( size_t i ) const { return targets_[i]; }
        //! Returns the number of structures.
        size_t size() const { return N; }
        //! Returns the range of equivalent configurations for structure \a _i.
        boost::numeric::ublas::range range( size_t _i ) const 
          { return boost::numeric::ublas::range( nb_[_i], nb_[_i+1] ); }
        //! Returns the weight for an equivalent structure.
        t_Vector::value_type eweight( size_t _i, size_t _c ) const
          { return equiweights[ nb_[_i] + _c ]; }
        //! Initializes the mapping.
       template< class T_STRUCTURES, class T_CONFIGURATIONS >
         void init( const T_STRUCTURES& _strs, 
                    const T_CONFIGURATIONS& _confs );

        //! Allows to skip out on a structure for leave-one or many-out.
        bool do_skip( size_t _i ) const { return false; }

      protected:
        //! Number of structures.
        size_t N;
        //! Weights of structures.
        t_Vector weights_;
        //! Weights of structures.
        t_Vector equiweights;
        //! Target values of structures.
        t_Vector targets_;
        //! Structure ranges.
        t_Vector nb_;
    };

  template< class T_BASE >
    class ExcludeOne : public T_BASE
    {
      public:
        //! Type of the base class.
        typedef T_BASE t_Base;
        //! Index of structure to exclude.
        size_t n;
        //! Wether to exclude at all.
        bool do_exclude;
        //! Constructor.
        ExcludeOne() : T_BASE(), n(0), do_exclude( false ) {}
        //! Destructor.
        ~ExcludeOne() {}
        //! Returns true if \a _i == ExcludeOne::n and ExcludeOne::do_exclude is true.
        bool do_skip( size_t _i ) const { return do_exclude and _i == n; }
    };

  template< class T_STRUCTURES, class T_CONFIGURATIONS >
    void SymEquiv::init( const T_STRUCTURES& _strs, 
                         const T_CONFIGURATIONS& _confs )
    {
      namespace bl = boost::lambda;
      // Copy structural weights first.
      weights_.clear();
      std::transform
      (
        _strs.begin(), _strs.end(), std::back_inserter(weights_),
        bl::bind( &T_STRUCTURES :: value_type :: weight, bl::_1 )
      );

      // Copy structural energies second.
      targets_.clear();
      std::transform
      (
        _strs.begin(), _strs.end(), std::back_inserter(targets_),
        bl::bind( &T_STRUCTURES :: value_type :: energy, bl::_1 )
      );
      N = _strs.size();
      
      // then construct internal weights (between equivalent confs) 
      // and initializes the nb_ structure.
      nb_.resize(1, 0);
      equiweights.clear();
      size_t sum(0);
      typename T_CONFIGURATIONS :: const_iterator i_confs = _confs.begin();
      typename T_CONFIGURATIONS :: const_iterator i_confs_end = _confs.end();
      for(; i_confs != i_confs_end; ++i_confs )
      {
        sum += i_confs->size();
        nb_.push_back( sum );
        typename T_CONFIGURATIONS :: value_type 
                                  :: const_iterator i_conf = i_confs->begin();
        typename T_CONFIGURATIONS :: value_type 
                                  :: const_iterator i_conf_end = i_confs->end();
        for(; i_conf != i_conf_end; ++i_conf )
          equiweights.push_back( i_conf->second );
      }
    }

  } // end of Mapping namespace.

} // end of CE namespace
#endif

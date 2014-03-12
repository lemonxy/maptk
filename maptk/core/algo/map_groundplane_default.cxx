/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

/**
 * \file
 * \brief Implementation of \link maptk::algo::map_groundplane_default
 *        map_groundplane_default \endlink
 */

#include "map_groundplane_default.h"

#include <maptk/core/algo/estimate_homography.h>

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

#include <boost/foreach.hpp>


namespace maptk
{

namespace algo
{


// Extra data stored for every active track
struct extra_track_info
{
  // Track ID for the given track this struct extends
  track_id_t tid;

  // Location of this track in the reference frame
  homography_point ref_loc;

  // Is the ref loc valid?
  bool ref_loc_valid;

  // Should this point be used in homography regression?
  bool is_good;

  // The number of times we haven't seen this track as active
  unsigned missed_count;

  // Constructor.
  extra_track_info()
  : ref_loc( 0.0, 0.0 ),
    ref_loc_valid( false ),
    is_good( false ),
    missed_count( 0 )
  {}
};


// Buffer type for the extra track info
typedef std::vector< extra_track_info > track_ext_buffer_t;

// Internal homography estimator type
typedef maptk::algo::estimate_homography_sptr estimator_sptr;


// Private implementation class
class map_groundplane_default::priv
{
public:

  priv()
  : use_backproject_error( false ),
    backproject_threshold_sqr( 16.0 ),
    forget_track_threshold( 10 )
  {
  }

  priv( const priv& other )
  : use_backproject_error( other.use_backproject_error ),
    backproject_threshold_sqr( other.backproject_threshold_sqr ),
    forget_track_threshold( other.forget_track_threshold )
  {
  }

  ~priv()
  {
  }

  /// Should we remove extra points if the backproject error is high?
  bool use_backproject_error;

  /// Backprojection threshold in terms of L2 distance (number of pixels)
  double backproject_threshold_sqr;

  /// After how many frames should we forget all info about a track?
  unsigned forget_track_threshold;

  /// Buffer storing track extensions
  track_ext_buffer_t buffer;

  /// Pointer to homography estimator
  estimator_sptr h_estimator;
};


map_groundplane_default
::map_groundplane_default()
: d_( new priv() )
{
}


map_groundplane_default
::map_groundplane_default( const map_groundplane_default& other )
: d_( new priv( *other.d_ ) )
{
}


map_groundplane_default
::~map_groundplane_default()
{
}


config_block_sptr
map_groundplane_default
::get_configuration() const
{
  // get base config from base class
  config_block_sptr config = algorithm::get_configuration();

  // Sub-algorithm implementation name + sub_config block
  // - Homography estimator algorithm
  estimate_homography::get_nested_algo_configuration
    ( "homography_estimator", config, d_->h_estimator );

  return config;
}


void
map_groundplane_default
::set_configuration( config_block_sptr in_config )
{
  // Starting with our generated config_block to ensure that assumed values are present
  // An alternative is to check for key presence before performing a get_value() call.
  config_block_sptr config = this->get_configuration();
  config->merge_config( in_config );

  // Setting nested algorithm instances via setter methods instead of directly
  // assigning to instance property.
  estimate_homography::set_nested_algo_configuration
    ( "homography_estimator", config, d_->h_estimator );
}


bool
map_groundplane_default
::check_configuration(config_block_sptr config) const
{
  return
  (
    estimate_homography::check_nested_algo_configuration
      ( "homography_estimator", config )
  );
}


// Helper function for sorting etis
bool compare_eti( const extra_track_info& c1, const extra_track_info& c2 )
{
  return c1.tid < c2.tid;
}


// Helper function for finding an ati for a given track
bool compare_eti_to_track( const track_sptr& c1, const extra_track_info& c2 )
{
  return c1->id() == c2.tid;
}


homography_collection_sptr
map_groundplane_default
::measure( frame_id_t frame_number,
           track_set_sptr tracks ) const
{
  // Get active tracks for the current frame
  track_set_sptr active_tracks = tracks->active_tracks( frame_number );

  // Process new tracks, add to list, and remove old tracks.
  std::vector< track_sptr > new_tracks;

  for( unsigned i = 0; i < active_tracks->size(); i++ )
  {

  }

  // Ensure that the vector is still sorted. Chances are it still is and
  // this is a simple scan of the vector.
  std::sort( d_->buffer.begin(), d_->buffer.end(), compare_eti );

  // Compute homography if possible
  // []

  // Update reference locations for existing tracks using homography
  // []

  return homography_collection_sptr();
}

} // end namespace algo

} // end namespace maptk

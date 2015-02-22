/*
 * PROJECT: GEMMapper
 * FILE: approximate_search.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef APPROXIMATE_SEARCH_H_
#define APPROXIMATE_SEARCH_H_

#include "essentials.h"
#include "quality_model.h"

#include "locator.h"
#include "graph_text.h"
#include "fm_index.h"
#include "matches.h"
#include "sequence.h"
#include "pattern.h"

#include "region_profile.h"
#include "interval_set.h"
#include "filtering_candidates.h"

#include "approximate_search_parameters.h"

/*
 * Approximate Search
 */
//typedef enum { asearch_init, asearch_filtering, asearch_neighborhood, asearch_end } approximate_search_stage_t;
typedef enum {
  asearch_begin,                // Beginning of the search
  asearch_no_regions,           // While doing the region profile no regions were found
  asearch_exact_matches,        // One maximum region was found (exact results)
  asearch_exact_filtering,      // Region-Minimal Profile + Exact candidate generation
  asearch_verify_candidates,    // Verify candidates
  asearch_candidates_verified,  // Candidates verified
  asearch_inexact_filtering,    // Region-Delimit Profile + Approximate candidate generation
  asearch_neighborhood,         // Neighborhood search
  asearch_end,                  // End of the current workflow
  asearch_probe_candidates      // Probe candidates (try to lower max-differences) // TODO
} approximate_search_state_t;
typedef enum {
  region_profile_fixed,
  region_profile_adaptive
} region_profile_type;
typedef enum {
  region_filter_fixed,
  region_filter_adaptive_static,
  region_filter_adaptive_dynamic
} region_filter_type;
typedef struct {
  /* Index Structures, Pattern & Parameters */
  archive_t* archive;                                   // Archive
  pattern_t pattern;                                    // Search Pattern
  search_actual_parameters_t* search_actual_parameters; // Search Parameters (Evaluated to read-length)
  /* Search State */
  strand_t search_strand;                               // Current search strand
  bool do_quality_search;                               // Quality search
  approximate_search_state_t search_state;              // Current State of the search
  bool verify_candidates;                               // Compute candidate verification
  bool stop_before_neighborhood_search;                 // Stop before Neighborhood Search
  uint64_t max_differences;
  uint64_t max_complete_stratum;                        // Maximum complete stratum reached by the search
  uint64_t max_matches_reached;                         // Quick abandon due to maximum matches found
  uint64_t lo_exact_matches;                            // Interval Lo (Exact matching)
  uint64_t hi_exact_matches;                            // Interval Hi (Exact matching)
  /* Search Structures */
  region_profile_t region_profile;                      // Region Profile
  filtering_candidates_t filtering_candidates;          // Filtering Candidates
  /* BPM Buffer */
  uint64_t bpm_buffer_offset;
  uint64_t bpm_buffer_candidates;
  /* Search Auxiliary Structures (external) */
  text_collection_t* text_collection;                   // Stores text-traces
  interval_set_t* interval_set;                         // Interval Set
  /* MM */
  mm_stack_t* mm_stack;                                 // MM-Stack
} approximate_search_t;

/*
 * Setup
 */
GEM_INLINE void approximate_search_init(
    approximate_search_t* const search,archive_t* const archive,
    search_actual_parameters_t* const search_actual_parameters);
GEM_INLINE void approximate_search_configure(
    approximate_search_t* const search,text_collection_t* text_collection,
    interval_set_t* const interval_set,mm_stack_t* const mm_stack);
GEM_INLINE void approximate_search_reset(approximate_search_t* const search);
GEM_INLINE void approximate_search_destroy(approximate_search_t* const search);

/*
 * Accessors
 */
GEM_INLINE uint64_t approximate_search_get_num_potential_candidates(const approximate_search_t* const search);

/*
 * Pattern
 */
GEM_INLINE void approximate_search_pattern_prepare(
    approximate_search_t* const search,sequence_t* const sequence);
GEM_INLINE void approximate_search_pattern_clear(approximate_search_t* const search);
GEM_INLINE bool approximate_search_pattern_is_null(approximate_search_t* const search);

/*
 * ASM-Search!!
 */
GEM_INLINE void approximate_search(approximate_search_t* const search,matches_t* const matches);
GEM_INLINE void approximate_search_verify_using_bpm_buffer(
    approximate_search_t* const search,
    matches_t* const matches,bpm_gpu_buffer_t* const bpm_gpu_buffer,
    const uint64_t candidate_offset_begin,const uint64_t candidate_offset_end);

/*
 * Error Msg
 */
//#define GEM_ERROR_ASM_

#endif /* APPROXIMATE_SEARCH_H_ */

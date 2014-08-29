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
#include "dna_string.h"
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

// Approximate Search
typedef enum { asearch_init, asearch_filtering, asearch_neighborhood, asearch_end } approximate_search_stage_t;
typedef struct {
  /* Index Structures & Parameters */
  locator_t* locator;                                 // Sequence Locator
  graph_text_t* graph;                                // Graph (text + graph = hypertext)
  dna_text_t* enc_text;                               // Index-Text (Encoded)
  fm_index_t* fm_index;                               // FM-Index
  pattern_t pattern;                                  // Search Pattern
  approximate_search_parameters_t* search_parameters; // Search Parameters
  /* Search State */
  strand_t search_strand;                             // Current search strand
  bool do_quality_search;                             // Quality search
  approximate_search_stage_t current_search_stage;    // Current Stage of the search
  approximate_search_stage_t stop_search_stage;       // Search stage to stop at
  uint64_t max_differences;
  uint64_t max_complete_stratum;
  bool max_matches_reached;                           // Quick abandon due to maximum matches reached
  /* Search Auxiliary Structures */
  region_profile_t region_profile;                    // Region Profile
  uint64_t num_potential_candidates;                  // Total number of candidates generated
  filtering_candidates_t filtering_candidates;        // Filtering Candidates
  interval_set_t intervals_result;                    // Interval Set (to hold intervals from neighborhood searches)
  // TODO
  /* MM */
  mm_stack_t* mm_stack;  // Memory stack
} approximate_search_t;

/*
 * Setup
 */
GEM_INLINE approximate_search_t* approximate_search_new(
    locator_t* const locator,graph_text_t* const graph,dna_text_t* const enc_text,fm_index_t* const fm_index,
    approximate_search_parameters_t* const parameters,mm_stack_t* const mm_stack);
GEM_INLINE void approximate_search_clear(approximate_search_t* const search);
GEM_INLINE void approximate_search_delete(approximate_search_t* const search);

/*
 * Accessors
 */
GEM_INLINE uint64_t approximate_search_get_num_potential_candidates(approximate_search_t* const search);

/*
 * Pattern
 */
GEM_INLINE void approximate_search_prepare_pattern(
    approximate_search_t* const search,
    const approximate_search_parameters_t* const parameters,sequence_t* const sequence);

// ASM-Search!!
GEM_INLINE void approximate_search(approximate_search_t* const search,matches_t* const matches);



//GEM_INLINE uint64_t fmi_mismatched_search_extend(
//    const fm_index_t* const fm_index,
//    fmi_extend_parameters* const extend_parameters,
//    const idx_t init_position,const idx_t end_position,
//    matches_t* const matches,vector_pool* const mpool);

//void fmi_check_pos_correctness(
//    const _FMI_* const fmi,fmi_search_parameters* const search_params,
//    matches* const matches,bool dump_results,vector_pool* const mpool);
//void fmi_check_int_correctness(
//    const _FMI_* const fmi,fmi_search_parameters* const search_params,
//    matches* const matches,bool dump_results,vector_pool* const mpool);
//void fmi_check_completeness(
//    const _FMI_* const fmi,fmi_search_parameters* const search_params,
//    matches* const matches,bool dump_results,vector_pool* const mpool);

/*
 * Error Msg
 */
//#define GEM_ERROR_ASM_

#endif /* APPROXIMATE_SEARCH_H_ */

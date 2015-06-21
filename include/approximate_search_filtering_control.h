/*
 * PROJECT: GEMMapper
 * FILE: approximate_search_filtering_control.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef APPROXIMATE_SEARCH_FILTERING_CONTROL_H_
#define APPROXIMATE_SEARCH_FILTERING_CONTROL_H_

#include "essentials.h"
#include "approximate_search.h"

GEM_INLINE void approximate_search_adjust_max_differences_using_strata(
    approximate_search_t* const search,matches_t* const matches) ;

GEM_INLINE bool asearch_fulfilled(approximate_search_t* const search,matches_t* const matches);

GEM_INLINE bool asearch_filter_ahead_candidates(approximate_search_t* const search,matches_t* const matches);

GEM_INLINE void asearch_control_next_state(approximate_search_t* const search,
    const approximate_search_state_t processing_step,matches_t* const matches);

#endif /* APPROXIMATE_SEARCH_FILTERING_CONTROL_H_ */
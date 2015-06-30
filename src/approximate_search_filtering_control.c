/*
 * PROJECT: GEMMapper
 * FILE: approximate_search_filtering.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#include "approximate_search_filtering.h"
#include "archive_score.h"
#include "matches_classify.h"

/*
 * Approximate Search Control
 */
GEM_INLINE void approximate_search_adjust_max_differences_using_strata(
    approximate_search_t* const search,matches_t* const matches) {
  const uint64_t max_differences = search->max_differences;
  const uint64_t delta = search->as_parameters->complete_strata_after_best_nominal;
  /*
   * If delta parameter is set (and is below the maximum number of mismatches),
   * finds the minimum non zero stratum (mnzs) and adjusts
   * the maximum number of mismatches to (mnzs+delta)
   */
  if (delta < max_differences) {
    const int64_t fms = matches_metrics_get_min_edit_distance(&matches->metrics);
    if (fms>=0 && fms+delta < max_differences) {
      search->max_differences = fms+delta;
    }
  }
}
// Control Fulfilled-search
GEM_INLINE bool asearch_fulfilled(approximate_search_t* const search,matches_t* const matches) {
  // Parameters
  const search_parameters_t* const search_parameters = search->as_parameters->search_parameters;
  if (matches==NULL) return false;
  // Determines when the search is done following the mapping criteria
  switch (search_parameters->mapping_mode) {
    case mapping_adaptive_filtering_fast: {
      if (!matches) return false;
      // Classify
      const matches_class_t matches_class = matches_classify(matches);
      switch (matches_class) {
        case matches_class_unmapped:
          return false;
        case matches_class_tie_swg_score:
        case matches_class_tie_edit_distance:
        case matches_class_tie_event_distance:
          return (matches->metrics.min1_edit_distance <= 1);
        case matches_class_mmap:
          return false;
        case matches_class_unique: {
          const uint64_t read_length = search->pattern.key_length;
          const swg_penalties_t* const swg_penalties = &search->as_parameters->search_parameters->swg_penalties;
          const uint64_t max_region_length = search->region_profile.max_region_length;
          const uint64_t proper_length = fm_index_get_proper_length(search->archive->fm_index);
          matches_predictors_t predictors;
          matches_classify_compute_predictors(matches,&predictors,swg_penalties,
              read_length,max_region_length,proper_length,search->max_complete_stratum);
          return (archive_score_matches_gem_se_unique(&predictors)>=55);
        }
        default:
          return false;
      }
      break;
    }
    case mapping_adaptive_filtering_match: {
      // FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
      // FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
      // FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
      // FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME

//      // Unmapped
//      if (matches_counters_get_total_count(matches)==0) return false;
//      // Ties
//      const uint64_t min_distance = matches_classify_metrics_get_min_distance(matches);
//      if (matches_counters_get_count(matches,min_distance) > 1) return true;
      // Otherwise
      return false;
    }
    case mapping_adaptive_filtering_complete:
      return search->max_complete_stratum > search->max_differences;
    default:
      GEM_INVALID_CASE();
      break;
  }
  return true;
}
// Control Filter-ahead
GEM_INLINE bool asearch_filter_ahead_candidates(approximate_search_t* const search,matches_t* const matches) {
  // Parameters
  const as_parameters_t* const actual_parameters = search->as_parameters;
  const search_parameters_t* const search_parameters = actual_parameters->search_parameters;
  // Determines when the search is done following the mapping criteria
  switch (search_parameters->mapping_mode) {
    case mapping_adaptive_filtering_fast:
      return true;
    case mapping_adaptive_filtering_match:
      return false;
    case mapping_adaptive_filtering_complete:
      return actual_parameters->complete_strata_after_best_nominal < search->max_differences;
    default:
      GEM_INVALID_CASE();
      break;
  }
  return false;
}
// Control Filtering-Fast
GEM_INLINE void asearch_control_fast_next_state(
    approximate_search_t* const search,const approximate_search_state_t processing_step,matches_t* const matches) {
  // Filtering-Fast: Best efficiency, keeping good-quality results (search for a "good" match)
  switch (processing_step) {
    case asearch_exact_filtering_adaptive:
      if (search->search_state==asearch_no_regions) return;
      if (search->search_state==asearch_exact_matches) return;
      search->search_state = (matches_get_num_match_traces(matches)==0) ? asearch_local_alignment : asearch_end;
      break;
    case asearch_local_alignment:
      search->search_state = asearch_end;
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
}
// Control Filtering-Match
GEM_INLINE void asearch_control_match_next_state(
    approximate_search_t* const search,
    const approximate_search_state_t processing_step,matches_t* const matches) {
  // Filtering-Match: Spend more computing resources as to guarantee good-quality (increasing likelihood of results)
  switch (processing_step) {
    case asearch_exact_filtering_adaptive:
      if (search->search_state==asearch_exact_matches) return;
      if (search->search_state==asearch_no_regions) {
        search->search_state = asearch_exact_filtering_boost;
        return;
      }
      if (asearch_fulfilled(search,matches)) {
        search->search_state = asearch_end;
      } else {
        search->search_state = asearch_exact_filtering_boost;
      }
      break;
    case asearch_exact_filtering_boost:
      search->search_state = (matches_get_num_match_traces(matches)==0) ? asearch_local_alignment : asearch_end;
      break;
    case asearch_local_alignment:
      search->search_state = asearch_end;
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
}
// Control Complete
GEM_INLINE void asearch_control_complete_next_state(
    approximate_search_t* const search,const approximate_search_state_t processing_step,
    matches_t* const matches) {
  GEM_NOT_IMPLEMENTED();
}
// Control HUB
GEM_INLINE void asearch_control_next_state(
    approximate_search_t* const search,const approximate_search_state_t processing_step,
    matches_t* const matches) {
  // Parameters
  const search_parameters_t* const search_parameters = search->as_parameters->search_parameters;
  // Determines when the search is done following the mapping criteria
  switch (search_parameters->mapping_mode) {
    case mapping_adaptive_filtering_fast:
      asearch_control_fast_next_state(search,processing_step,matches);
      break;
    case mapping_adaptive_filtering_match:
      asearch_control_match_next_state(search,processing_step,matches);
      break;
    case mapping_adaptive_filtering_complete:
      asearch_control_complete_next_state(search,processing_step,matches);
      break;
    default:
      GEM_INVALID_CASE();
      break;
  }
}

/*
 * PROJECT: GEMMapper
 * FILE: approximate_search_parameters.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#include "approximate_search_parameters.h"

/*
 * Region profile default parameters
 */
GEM_INLINE void approximate_search_initialize_replacements(search_parameters_t* const search_parameters) {
  // Reset
  memset(search_parameters->allowed_chars,0,256*sizeof(bool));
  memset(search_parameters->allowed_enc,0,DNA_EXT_RANGE*sizeof(bool));
  search_parameters->mismatch_alphabet[0] = DNA_CHAR_A;
  search_parameters->mismatch_alphabet[1] = DNA_CHAR_C;
  search_parameters->mismatch_alphabet[2] = DNA_CHAR_G;
  search_parameters->mismatch_alphabet[3] = DNA_CHAR_T;
  search_parameters->mismatch_alphabet_length = 4;
  search_parameters->allowed_chars[DNA_CHAR_A] = true;
  search_parameters->allowed_chars[DNA_CHAR_C] = true;
  search_parameters->allowed_chars[DNA_CHAR_G] = true;
  search_parameters->allowed_chars[DNA_CHAR_T] = true;
  search_parameters->allowed_chars[DNA_CHAR_N] = false;
  search_parameters->allowed_enc[ENC_DNA_CHAR_A] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_C] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_G] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_T] = true;
  search_parameters->allowed_enc[ENC_DNA_CHAR_N] = false;
}
GEM_INLINE void approximate_search_parameters_init(search_parameters_t* const search_parameters) {
  /*
   * Single End
   */
  // Mapping strategy
  search_parameters->mapping_mode = mapping_adaptive_filtering;
  search_parameters->filtering_degree = 0;
  // Qualities
  search_parameters->quality_model = quality_model_type_gem;
  search_parameters->quality_format = qualities_ignore;
  search_parameters->quality_threshold = 26;
  // Mismatch/Indels Parameters
  search_parameters->max_search_error = 0.04;
  search_parameters->max_filtering_error = 0.08;
  search_parameters->complete_strata_after_best = 0.0;
  search_parameters->min_matching_length = 0.20;
  // Matches search
  search_parameters->max_search_matches = ALL;
  // Replacements
  approximate_search_initialize_replacements(search_parameters);
  // Regions handling
  search_parameters->candidate_chunk_max_length = UINT64_MAX;
  search_parameters->allow_region_chaining = true;
  // Alignment Model/Score
  search_parameters->alignment_model = alignment_model_gap_affine;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_A] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_C] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_G] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_T] = +1;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_A] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_C] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_G] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_T] = -4;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_N][ENC_DNA_CHAR_N] = -4;
  search_parameters->swg_penalties.gap_open_penalty = 6;
  search_parameters->swg_penalties.gap_extension_penalty = 1;
  /*
   * Paired End
   */
  /* Paired-end mode/alg */
  search_parameters->paired_end = false;
  search_parameters->map_both_ends = false;
  search_parameters->max_extendable_candidates = 20;
  search_parameters->max_matches_per_extension = 2;
  /* Template allowed length */
  search_parameters->min_template_length = 0;
  search_parameters->max_template_length = UINT64_MAX;
  /* Concordant Orientation */
  search_parameters->pair_orientation_FR = true;
  search_parameters->pair_orientation_RF = false;
  search_parameters->pair_orientation_FF = false;
  search_parameters->pair_orientation_RR = false;
  /* Discordant Orientation */
  search_parameters->discordant_pair_orientation_FR = false;
  search_parameters->discordant_pair_orientation_RF = true;
  search_parameters->discordant_pair_orientation_FF = true;
  search_parameters->discordant_pair_orientation_RR = true;
  /* Pair allowed lay-outs */
  search_parameters->pair_layout_separate = true;
  search_parameters->pair_layout_overlap = true;
  search_parameters->pair_layout_contain = true;
  search_parameters->pair_layout_dovetail = true;
  /*
   * Internals
   */
  // Soft RP. Loose Scheme = (20,4,2,2)
  search_parameters->rp_soft.region_th = 20;
  search_parameters->rp_soft.max_steps = 4;
  search_parameters->rp_soft.dec_factor = 2;
  search_parameters->rp_soft.region_type_th = 2;
  // Hard RP. Tight Scheme = (50,10,4,2)
  search_parameters->rp_hard.region_th = 50;
  search_parameters->rp_hard.max_steps = 10;
  search_parameters->rp_hard.dec_factor = 4;
  search_parameters->rp_hard.region_type_th = 2;
//  // Probing RP. Probing Scheme = (1000,3,3,2)
//  search_parameters->rp_probing.region_th = 1000;
//  search_parameters->rp_probing.max_steps = 3;
//  search_parameters->rp_probing.dec_factor = 3;
//  search_parameters->rp_probing.region_type_th = 2;
  // Recovery RP. Recovery Scheme = (200,1,8,2)
  search_parameters->rp_recovery.region_th = 200;
  search_parameters->rp_recovery.max_steps = 1;
  search_parameters->rp_recovery.dec_factor = 8;
  search_parameters->rp_recovery.region_type_th = 2;
  // Filtering Thresholds
  search_parameters->filtering_region_factor = 1.0;
  search_parameters->filtering_threshold = 350;
  search_parameters->pa_filtering_threshold = 2500;
}
GEM_INLINE void approximate_search_configure_mapping_strategy(
    search_parameters_t* const search_parameters,
    const mapping_mode_t mapping_mode,const float filtering_degree) {
  search_parameters->mapping_mode = mapping_mode;
  search_parameters->filtering_degree = filtering_degree;
}
GEM_INLINE void approximate_search_configure_quality_model(
    search_parameters_t* const search_parameters,
    const quality_model_t quality_model,const quality_format_t quality_format,const uint64_t quality_threshold) {
  search_parameters->quality_model = quality_model;
  search_parameters->quality_format = quality_format;
  search_parameters->quality_threshold = quality_threshold;
}
GEM_INLINE void approximate_search_configure_error_model(
    search_parameters_t* const search_parameters,
    float max_search_error,float max_filtering_error,
    float complete_strata_after_best,float min_matching_length) {
  search_parameters->max_search_error = max_search_error;
  search_parameters->max_filtering_error = max_filtering_error;
  search_parameters->complete_strata_after_best = complete_strata_after_best;
  search_parameters->min_matching_length = min_matching_length;
}
GEM_INLINE void approximate_search_configure_matches(
    search_parameters_t* const search_parameters,const uint64_t max_search_matches) {
  search_parameters->max_search_matches = max_search_matches;
}
GEM_INLINE void approximate_search_configure_replacements(
    search_parameters_t* const search_parameters,
    const char* const mismatch_alphabet,const uint64_t mismatch_alphabet_length) {
  // Reset
  approximate_search_initialize_replacements(search_parameters);
  // Filter replacements
  uint64_t i, count;
  for (i=0,count=0;i<mismatch_alphabet_length;i++) {
    if (is_dna(mismatch_alphabet[i])) {
      const char c = dna_normalized(mismatch_alphabet[i]);
      search_parameters->mismatch_alphabet[count] = c;
      search_parameters->allowed_chars[(uint8_t)c] = true;
      search_parameters->allowed_enc[dna_encode(c)] = true;
      ++count;
    }
  }
  gem_cond_fatal_error(count==0,ASP_REPLACEMENT_EMPTY);
  search_parameters->mismatch_alphabet_length = count;
}
GEM_INLINE void approximate_search_configure_region_handling(
    search_parameters_t* const search_parameters,
    const uint64_t candidate_chunk_max_length,const bool allow_region_chaining) {
  // Regions treatment
  search_parameters->candidate_chunk_max_length = candidate_chunk_max_length;
  search_parameters->allow_region_chaining = allow_region_chaining;
}
GEM_INLINE void approximate_search_configure_alignment_model(
    search_parameters_t* const search_parameters,const alignment_model_t alignment_model) {
  search_parameters->alignment_model = alignment_model;
}
GEM_INLINE void approximate_search_configure_alignment_scores(
    search_parameters_t* const search_parameters,const uint64_t matching_score,
    const uint64_t gap_open_penalty,const uint64_t gap_extension_penalty) {
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_A][ENC_DNA_CHAR_A] = matching_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_C][ENC_DNA_CHAR_C] = matching_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_G][ENC_DNA_CHAR_G] = matching_score;
  search_parameters->swg_penalties.matching_score[ENC_DNA_CHAR_T][ENC_DNA_CHAR_T] = matching_score;
  search_parameters->swg_penalties.gap_open_penalty = gap_open_penalty;
  search_parameters->swg_penalties.gap_extension_penalty = gap_extension_penalty;
}
GEM_INLINE void approximate_search_instantiate_values(
    search_actual_parameters_t* const search_actual_parameters,const uint64_t pattern_length) {
  const search_parameters_t* const search_parameters = search_actual_parameters->search_parameters;
  search_actual_parameters->filtering_degree_nominal = integer_proportion(search_parameters->filtering_degree,pattern_length);
  search_actual_parameters->max_search_error_nominal = integer_proportion(search_parameters->max_search_error,pattern_length);
  search_actual_parameters->max_filtering_error_nominal = integer_proportion(search_parameters->max_filtering_error,pattern_length);
  search_actual_parameters->complete_strata_after_best_nominal = integer_proportion(search_parameters->complete_strata_after_best,pattern_length);
  search_actual_parameters->min_matching_length_nominal = integer_proportion(search_parameters->min_matching_length,pattern_length);
}

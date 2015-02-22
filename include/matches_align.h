/*
 * PROJECT: GEMMapper
 * FILE: matches_align.h
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 */

#ifndef MATCHES_ALIGN_H_
#define MATCHES_ALIGN_H_

#include "essentials.h"
#include "matches.h"
#include "bpm_align.h"
#include "swg_align.h"

typedef enum {
  alignment_model_none,
  alignment_model_hamming,
  alignment_model_levenshtein,
  alignment_model_gap_affine
} alignment_model_t;

GEM_INLINE void matches_align_exact(
    matches_t* const matches,match_trace_t* const match_trace,
    const strand_t strand,const swg_penalties_t* const swg_penalties,
    const uint64_t key_length,const uint64_t text_trace_offset,const uint64_t match_position,
    const uint64_t text_begin_offset,const uint64_t text_end_offset,const uint64_t match_distance);
GEM_INLINE void matches_align_hamming(
    matches_t* const matches,match_trace_t* const match_trace,
    const strand_t strand,const bool* const allowed_enc,
    const uint8_t* const key,const uint64_t key_length,
    const uint64_t text_trace_offset,const uint64_t match_position,
    const uint8_t* const text,const uint64_t text_begin_offset);
GEM_INLINE void matches_align_levenshtein(
    matches_t* const matches,match_trace_t* const match_trace,const strand_t strand,
    const uint8_t* const key,const bpm_pattern_t* const bpm_pattern,
    const uint64_t text_trace_offset,const uint64_t match_position,
    uint8_t* const text,const uint64_t text_begin_offset,const uint64_t text_end_offset,
    const uint64_t max_distance,const region_matching_t* const regions_matching,
    const uint64_t num_regions_matching,mm_stack_t* const mm_stack);
GEM_INLINE void matches_align_smith_waterman_gotoh(
    matches_t* const matches,match_trace_t* const match_trace,const strand_t strand,const bool* const allowed_enc,
    const swg_query_profile_t* const swg_query_profile,const swg_penalties_t* swg_penalties,
    const uint8_t* const key,const uint64_t key_length,const uint64_t text_trace_offset,
    const uint64_t match_position,uint8_t* const text,const uint64_t text_length,
    const uint64_t text_begin_offset,const uint64_t text_end_offset,
    const uint64_t max_bandwidth,const region_matching_t* const regions_matching,
    const uint64_t num_regions_matching,mm_stack_t* const mm_stack);

#endif /* MATCHES_ALIGN_H_ */

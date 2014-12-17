/*
 * PROJECT: GEMMapper
 * FILE: archive_select.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "archive_select.h"
#include "archive_text_retrieve.h"
#include "matches_align.h"
#include "output_map.h"

/*
 * Realigning Matches
 */
GEM_INLINE void archive_select_realign_match_interval(
    archive_search_t* const archive_search,matches_t* const matches,
    match_interval_t* const match_interval,match_trace_t* const match_trace,mm_stack_t* const mm_stack) {
  const search_parameters_t* const search_parameters = archive_search->search_actual_parameters.search_parameters;
  const alignment_model_t alignment_model = search_parameters->alignment_model;
  if (match_interval->distance==0 || alignment_model==alignment_model_none) {
    const uint64_t key_length = sequence_get_length(&archive_search->sequence);
    matches_align_exact(matches,match_trace,match_interval->strand,key_length,
        match_trace->trace_offset,match_trace->position,
        match_interval->distance,match_interval->length);
  } else {
    // Search-state (strand-based)
    const approximate_search_t* search_state;
    search_state = (match_interval->strand == Forward) ?
        &archive_search->forward_search_state : &archive_search->reverse_search_state;
    // Align
    switch (alignment_model) {
      case alignment_model_hamming:
        matches_align_hamming(
            matches,match_trace,match_interval->strand,search_parameters->allowed_enc,
            search_state->pattern.key,search_state->pattern.key_length,
            match_trace->trace_offset,match_trace->position,match_interval->text);
        break;
      case alignment_model_levenshtein:
        matches_align_levenshtein(
            matches,match_trace,
            match_interval->strand,search_state->pattern.key,&search_state->pattern.bpm_pattern,
            match_trace->trace_offset,match_trace->position,match_interval->distance,
            match_interval->text,match_interval->length,NULL,0,mm_stack);
        break;
      case alignment_model_gap_affine:
        matches_align_smith_waterman_gotoh(
            matches,match_trace,match_interval->strand,search_parameters->allowed_enc,
            &search_parameters->swg_penalties,search_state->pattern.key,search_state->pattern.key_length,
            match_trace->trace_offset,match_trace->position,match_interval->distance,
            match_interval->text,match_interval->length,NULL,0,mm_stack);
        break;
      default:
        GEM_INVALID_CASE();
        break;
    }
  }
}
/*
 * Calculate the number of matches to be decoded
 */
GEM_INLINE void archive_select_calculate_matches_to_decode(
    matches_t* const matches,const uint64_t max_decoded_matches,const uint64_t min_decoded_strata,
    const uint64_t min_reported_matches,const uint64_t max_reported_matches,
    uint64_t* const total_strata_to_decode,uint64_t* const matches_to_decode_last_stratum_out) {
  // Compact counters (Shrink the counters to the last non-zero stratum)
  const uint64_t max_nz_stratum = matches_counters_compact(matches); // Maximum reachable stratum w.r.t counters
  if (max_nz_stratum==0) return;
  // Maximum stratum to decode (increased by @max_decoded_matches)
  const uint64_t* const counters = vector_get_mem(matches->counters,uint64_t);
  uint64_t strata_to_decode, total_matches;
  for (strata_to_decode=0,total_matches=0;strata_to_decode<max_nz_stratum;++strata_to_decode) {
    total_matches += counters[strata_to_decode];
    if (total_matches > max_decoded_matches) {
      total_matches -= counters[strata_to_decode];
      break;
    }
  }
  // Maximum stratum to decode (increased by @min_decoded_strata)
  if (min_decoded_strata > 0) {
    const uint64_t min_nz_stratum = matches_counters_get_min_matching_stratum(matches);
    const uint64_t mandatory_strata = min_nz_stratum + min_decoded_strata;
    for (;strata_to_decode<max_nz_stratum && strata_to_decode<mandatory_strata;++strata_to_decode) {
      total_matches += counters[strata_to_decode];
    }
  }
  // Maximum stratum to decode (increased by @min_reported_matches)
  for (;strata_to_decode<max_nz_stratum && total_matches<min_reported_matches;++strata_to_decode) {
    total_matches += counters[strata_to_decode];
  }
  // Maximum stratum to decode (lowered by @max_reported_matches & @min_reported_matches)
  for (;strata_to_decode>0;--strata_to_decode) {
    const uint64_t prev_acc = total_matches - counters[strata_to_decode-1];
    if (total_matches <= max_reported_matches || prev_acc < min_reported_matches) break;
    total_matches = prev_acc;
  }
  // Decode matches
  if (total_matches!=0) { // => (strata_to_decode > 0)
    *total_strata_to_decode = strata_to_decode;
    *matches_to_decode_last_stratum_out = UINT64_MAX;
    if (total_matches > max_reported_matches) {
      const uint64_t prev_acc = total_matches - counters[strata_to_decode-1];
      const uint64_t max_matches_from_last_stratum = max_reported_matches - prev_acc;
      *matches_to_decode_last_stratum_out = max_matches_from_last_stratum;
    }
  } else {
    *total_strata_to_decode = 0;
    *matches_to_decode_last_stratum_out = 0;
  }
}
GEM_INLINE void archive_select_locate_match_trace(
    const archive_t* const archive,matches_t* const matches,match_trace_t* const match_trace,
    const uint64_t seq_length,const int64_t match_length,const strand_t search_strand) {
  GEM_INTERNAL_CHECK(match_length >= 0,"Match effective length must be positive");
  location_t location;
  locator_map(archive->locator,match_trace->position,&location);
  match_trace->position = location.position;
  match_trace->sequence_name = location.tag;
  if (location.direction == Reverse) { // Adjust position by the effective length
    match_trace->position -= (uint64_t) match_length;
    match_trace->strand = Reverse;
    GEM_INTERNAL_CHECK(search_strand == Forward,
        "Archive-Select. Locating match-trace. "
        "Impossible combination (search_strand==Reverse-emulated searching FR-index)");
  } else {
    match_trace->strand = search_strand;
  }
}
GEM_INLINE void archive_select_correct_CIGAR(
    const archive_t* const archive,matches_t* const matches,
    const strand_t strand,const uint64_t cigar_buffer_offset,const uint64_t cigar_length) {
  if (strand == Reverse) {
    if (archive->filter_type==Iupac_dna) {
      matches_reverse_CIGAR(matches,cigar_buffer_offset,cigar_length);
    } else { // Iupac_colorspace_dna
      matches_reverse_CIGAR_colorspace(matches,cigar_buffer_offset,cigar_length);
//    /* In this case, we also have to re-encode the mismatches */
//    if (in_colorspace) { // TODO
//      for (i=0;i<mism_num;++i) {
//        misms[i].mismatch=misms_colorspace[misms[i].mismatch];
//          // const ch_t misms_colorspace[CH_T_RANGE]= { ['A']='a', ['C']='b', ['G']='c', ['T']='d', ['N']='e' };
//      }
//    }
    }
  }
}
/*
 * Decoding Matches (Retrieving & Processing matches)
 *   - 1. Expand interval-matches (compacted)
 *   - 2. Sort matches wrt distance
 *   - 3. Transform CIGAR of reverse matches
 */
GEM_INLINE void archive_select_decode_matches(
    archive_search_t* const archive_search,matches_t* const matches,
    const uint64_t strata_to_decode,const uint64_t matches_to_decode_last_stratum) {
  const archive_t* const archive = archive_search->archive;
  text_collection_t* const text_collection = archive_search->text_collection;
  const uint64_t seq_length = sequence_get_length(&archive_search->sequence);
  const uint64_t last_stratum_distance = strata_to_decode-1;
  /*
   * Match-Trace
   *   Count already decoded matches & discard unwanted matches
   */
  uint64_t num_matches_last_stratum = 0;
  match_trace_t* global_matches_it = vector_get_mem(matches->global_matches,match_trace_t);
  VECTOR_ITERATE(matches->global_matches,match_trace,match_trace_num,match_trace_t) {
    if (match_trace->distance <= last_stratum_distance) {
      // Count matches last stratum
      if (match_trace->distance == last_stratum_distance) {
        if (num_matches_last_stratum < matches_to_decode_last_stratum) {
          ++num_matches_last_stratum;
        } else {
          continue; // Too many matches decoded in the last stratum
        }
      }
      // 1.- (Re)Align match [Already DONE]
      // 2.- Correct CIGAR (Reverse it if the search was performed in the reverse strand, emulated)
      archive_select_correct_CIGAR(archive,matches,
          match_trace->strand,match_trace->cigar_buffer_offset,match_trace->cigar_length);
      // 3.- Locate-map the match
      archive_select_locate_match_trace(archive,matches,match_trace,
          seq_length,match_trace->effective_length,match_trace->strand);
      // 4.- Add the match (Store it in the vector, removing unwanted ones)
      if (global_matches_it != match_trace) *global_matches_it = *match_trace;
      ++global_matches_it;
    }
  }
  vector_update_used(matches->global_matches,global_matches_it); // Update used
  /*
   * Match-Interval
   *   Decode interval matches until we reach @max_decoded_matches
   */
  VECTOR_ITERATE(matches->interval_matches,match_interval,match_interval_num,match_interval_t) {
    if (num_matches_last_stratum >= matches_to_decode_last_stratum) break;
    // Get next match-interval
    if ((match_interval->lo >= match_interval->hi) || (match_interval->distance > last_stratum_distance)) continue;
    // 0.- Decode position (Use the first position of the interval, lo)
    match_trace_t match_trace;
    match_trace.position = fm_index_lookup(archive->fm_index,match_interval->lo);
    // 1.- (Re)Align match (retrieved with the seq-read(pattern))
    if (match_interval->distance > 0) {
      archive_text_retrieve(archive->locator,archive->graph,archive->enc_text,text_collection,
          match_trace.position,match_interval->length /* + delta TODO */ ,
          &match_trace.trace_offset,archive_search->mm_stack);
      // Set interval text
      const text_trace_t* const text_trace = text_collection_get_trace(text_collection,match_trace.trace_offset);
      match_interval->text = text_trace->text;
    }
    archive_select_realign_match_interval(archive_search,matches,match_interval,&match_trace,archive_search->mm_stack);
    // 2.- Correct CIGAR (Reverse it if the search was performed in the reverse strand, emulated)
    archive_select_correct_CIGAR(archive,matches,
        match_interval->strand,match_trace.cigar_buffer_offset,match_trace.cigar_length);
    // 3.- Locate-map the match
    archive_select_locate_match_trace(archive,matches,&match_trace,
        seq_length,match_trace.effective_length,match_interval->strand);
    // 4.- Add the match
    matches_add_match_trace_t(matches,&match_trace,false);
    const bool last_stratum_match = (match_interval->distance == last_stratum_distance);
    if (last_stratum_match) ++num_matches_last_stratum;
    // 5.- Build the rest of the interval
    uint64_t sa_position;
    for (sa_position=match_interval->lo+1;sa_position<match_interval->hi;++sa_position) {
      // Check number of matches (only last stratum)
      if (last_stratum_match && (++num_matches_last_stratum >= matches_to_decode_last_stratum)) break;
      // 0.- Decode position
      match_trace.position = fm_index_lookup(archive->fm_index,sa_position);
      // 1.- (Re)Align the match [Already DONE]
      // 2.- Correct CIGAR [Already DONE]
      // 3.- Locate-map the match
      archive_select_locate_match_trace(archive,matches,&match_trace,
          seq_length,match_trace.effective_length,match_interval->strand);
      // 4.- Add the match
      matches_add_match_trace_t(matches,&match_trace,false);
    }
  }
}
GEM_INLINE void archive_select_matches(archive_search_t* const archive_search,matches_t* const matches) {
  PROF_START(GP_ARCHIVE_SELECT_MATCHES);
  // Instantiate Search Parameters Values
  select_parameters_t* const select_parameters = archive_search->select_parameters;
  archive_select_instantiate_values(select_parameters,sequence_get_length(&archive_search->sequence));
  // Check if we need to decode sth
  if (select_parameters->max_decoded_matches==0 &&
      select_parameters->min_decoded_strata==0 &&
      select_parameters->min_reported_matches==0) return;
  if (select_parameters->min_reported_matches==0 && select_parameters->max_reported_matches==0) return;
  // Calculate the number of matches to decode wrt input parameters
  uint64_t strata_to_decode = 0, matches_to_decode_last_stratum = 0;
  archive_select_calculate_matches_to_decode(
      matches,select_parameters->max_decoded_matches,
      select_parameters->min_decoded_strata,select_parameters->min_reported_matches,
      select_parameters->max_reported_matches,&strata_to_decode,&matches_to_decode_last_stratum);
  // Decode matches
  if (strata_to_decode > 0) {
    archive_select_decode_matches(archive_search,matches,strata_to_decode,matches_to_decode_last_stratum);
    // Sort all matches
    matches_sort_by_distance(matches);
  } else {
    // Remove all matches
    vector_clear(matches->global_matches);
  }
  PROF_STOP(GP_ARCHIVE_SELECT_MATCHES);
}
/*
 * Check Matches
 */
GEM_INLINE void archive_check_matches_correctness(
    archive_t* const archive,sequence_t* const sequence,
    matches_t* const matches,mm_stack_t* const mm_stack) {
  // Prepare key(s)
  mm_stack_push_state(mm_stack);
  FILE* const stream = gem_error_get_stream();
  const char* const sequence_buffer = string_get_buffer(&sequence->read);
  const uint64_t key_length = sequence_get_length(sequence);
  uint8_t* const key = mm_stack_calloc(mm_stack,key_length,uint8_t,false);
  uint64_t i;
  for (i=0;i<key_length;++i) key[i] = dna_encode(sequence_buffer[i]);
  // Traverse all matches and check their CIGAR
  bool valid_alignment = true;
  VECTOR_ITERATE(matches->global_matches,match_trace,match_trace_num,match_trace_t) {
    // Retrieve text
    bool valid_map;
    const uint64_t position = inverse_locator_map(archive->locator,
        (uint8_t*)match_trace->sequence_name,match_trace->strand,match_trace->position);
    const uint64_t text_length = match_trace->effective_length;
    uint8_t* reverse_text = NULL;
    uint8_t* const forward_text = dna_text_retrieve_sequence(archive->enc_text,position,text_length,mm_stack);
    if (match_trace->strand==Forward) { // Forward
      valid_map = align_check_match(stream,key,key_length,forward_text,text_length,
          matches->cigar_buffer,match_trace->cigar_buffer_offset,match_trace->cigar_length,true);
    } else { // Reverse
      reverse_text = mm_stack_calloc(mm_stack,text_length,uint8_t,false);
      for (i=0;i<text_length;++i) reverse_text[text_length-i-1] = dna_encoded_complement(forward_text[i]);
      valid_map = align_check_match(stream,key,key_length,reverse_text,text_length,
          matches->cigar_buffer,match_trace->cigar_buffer_offset,match_trace->cigar_length,true);
    }
    // Check result
    if(!valid_map) {
      valid_alignment = false;
      fprintf(stream,"Match check failed. Global Match alignment => ");
      output_map_alignment_pretty(stream,match_trace,matches,key,key_length,
          (match_trace->strand==Forward)?forward_text:reverse_text,text_length,mm_stack);
    }
  }
  // Print Read
  if (!valid_alignment) {
    if (sequence_has_qualities(sequence)) {
      fprintf(stream,"@%s\n",sequence_get_tag(sequence));
      fprintf(stream,"%s\n+\n",sequence_get_read(sequence));
      fprintf(stream,"%s\n",sequence_get_qualities(sequence));
    } else {
      fprintf(stream,">%s\n",sequence_get_tag(sequence));
      fprintf(stream,"%s\n",sequence_get_read(sequence));
    }
  }
  mm_stack_pop_state(mm_stack,false);
}
GEM_INLINE void archive_check_matches_optimum(
    archive_t* const archive,sequence_t* const sequence,
    matches_t* const matches,mm_stack_t* const mm_stack) {
  GEM_NOT_IMPLEMENTED();
}
GEM_INLINE void archive_check_matches_completness(
    archive_t* const archive,sequence_t* const sequence,
    matches_t* const matches,mm_stack_t* const mm_stack) {
  GEM_NOT_IMPLEMENTED();
}



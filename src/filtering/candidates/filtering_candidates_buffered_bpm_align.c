/*
 *  GEM-Mapper v3 (GEM3)
 *  Copyright (c) 2011-2017 by Santiago Marco-Sola  <santiagomsola@gmail.com>
 *
 *  This file is part of GEM-Mapper v3 (GEM3).
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * PROJECT: GEM-Mapper v3 (GEM3)
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 * DESCRIPTION:
 *   Filtering candidates module provides functions to verify filtering-regions
 *   against its corresponding region of text in the index and compute the
 *   distance of the alignment between both
 *   This "buffered" module operates in batches of filtering-regions and
 *   makes use of GPU-buffers to offload the verification/alignment of
 *   regions to a GPU
 */

#include "filtering/candidates/filtering_candidates_buffered_bpm_align.h"
#include "align/alignment.h"
#include "filtering/region/filtering_region_verify.h"
#include "align/align_bpm_pattern.h"
#include "align/align_bpm_distance.h"

/*
 * Debug
 */
#define DEBUG_FILTERING_CANDIDATES GEM_DEEP_DEBUG

/*
 * Profile
 */
#define PROFILE_LEVEL PMED

/*
 * BPM-Buffered Add (Candidates Verification)
 */
void filtering_candidates_buffered_bpm_align_prepare_buffers(
    filtering_candidates_t* const filtering_candidates,
    filtering_candidates_buffered_t* const filtering_candidates_buffered,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align,
    uint64_t* const gpu_buffer_align_offset) {
  // Parameters
  const uint64_t num_filtering_regions = filtering_candidates_get_num_regions(filtering_candidates);
  // Allocate buffered filtering regions
  filtering_candidates_buffered_allocate_regions(filtering_candidates_buffered,num_filtering_regions);
  *gpu_buffer_align_offset = gpu_buffer_bpm_align_get_num_candidates(gpu_buffer_bpm_align);
  filtering_candidates_buffered->num_regions = num_filtering_regions;
}
uint64_t filtering_candidates_buffered_bpm_align_add_region(
    filtering_candidates_t* const filtering_candidates,
    filtering_region_t* const filtering_region,
    pattern_t* const pattern,
    const uint64_t candidate_pos,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align) {
  // Parameters
  const uint64_t max_error = pattern->max_effective_filtering_error;
  // Prepare Alignment-Tiles
  alignment_t* const alignment = &filtering_region->alignment;
  const uint64_t text_length =
      filtering_region->text_end_position - filtering_region->text_begin_position;
  filtering_candidates_init_alignment(
      filtering_candidates,alignment,pattern,text_length,max_error);
  // BPM-GPU put all candidates (tiles)
  const uint64_t num_tiles = alignment->num_tiles;
  uint64_t tile_pos, total_candidates_added=0;
  // Add candidate to GPU BPM-buffer
  for (tile_pos=0;tile_pos<num_tiles;++tile_pos) {
    // Fetch tiles
    alignment_tile_t* const alignment_tile = alignment->alignment_tiles + tile_pos;
    // Add tile to GPU BPM-buffer
    const uint64_t candidate_text_position =
        filtering_region->text_begin_position + alignment_tile->text_begin_offset;
    const uint64_t candidate_length =
        alignment_tile->text_end_offset-alignment_tile->text_begin_offset;
    gpu_buffer_bpm_align_add_candidate(gpu_buffer_bpm_align,
        tile_pos,candidate_text_position,candidate_length);
    ++total_candidates_added;
  }
  PROF_ADD_COUNTER(GP_ASSW_VERIFY_CANDIDATES_TILES_COPIED,num_tiles);
  // Return total candidates added
  return total_candidates_added;
}
uint64_t filtering_candidates_buffered_bpm_align_add_filtering_regions(
    filtering_candidates_t* const filtering_candidates,
    filtering_candidates_buffered_t* const filtering_candidates_buffered,
    pattern_t* const pattern,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align) {
  // Parameters
  filtering_region_t** const regions_in = filtering_candidates_get_regions(filtering_candidates);
  filtering_region_t** const region_buffered = filtering_candidates_buffered->regions;
  const uint64_t num_filtering_regions = filtering_candidates_get_num_regions(filtering_candidates);
  // Add all regions
  uint64_t candidate_pos, total_candidates_added = 0;
  for (candidate_pos=0;candidate_pos<num_filtering_regions;++candidate_pos) {
    // Filtering region
    filtering_region_t* const filtering_region = regions_in[candidate_pos];
    region_buffered[candidate_pos] = filtering_region;
    // Filter out exact-matches & key-trimmed regions
    if (filtering_region->alignment.distance_min_bound==0 || filtering_region->key_trimmed) {
      continue; // Next
    }
    // Add filtering-region
    total_candidates_added += filtering_candidates_buffered_bpm_align_add_region(
        filtering_candidates,filtering_region,pattern,candidate_pos,gpu_buffer_bpm_align);
  }
  gpu_buffer_bpm_align_record_candidates_per_tile(gpu_buffer_bpm_align,total_candidates_added);
  // Clear filtering regions (regions buffered)
  filtering_candidates_clear_regions(filtering_candidates,false);
  // Return
  return total_candidates_added;
}
void filtering_candidates_buffered_bpm_align_add(
    filtering_candidates_t* const filtering_candidates,
    filtering_candidates_buffered_t* const filtering_candidates_buffered,
    pattern_t* const pattern,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align,
    uint64_t* const gpu_buffer_align_offset) {
  // Check number of pending filtering-regions
  const uint64_t num_filtering_regions = filtering_candidates_get_num_regions(filtering_candidates);
  if (num_filtering_regions==0) {
    filtering_candidates_buffered->num_regions = 0;
    return;
  }
  // Prepare buffers
  filtering_candidates_buffered_bpm_align_prepare_buffers(
      filtering_candidates,filtering_candidates_buffered,
      gpu_buffer_bpm_align,gpu_buffer_align_offset);
  // Add the pattern to the buffer (add new queries)
  gpu_buffer_bpm_align_add_pattern(gpu_buffer_bpm_align,pattern);
  gpu_buffer_bpm_align_record_query_length(gpu_buffer_bpm_align,pattern->key_length);
  // Add all filtering regions (candidates in text-space)
  const uint64_t total_candidates_added =
      filtering_candidates_buffered_bpm_align_add_filtering_regions(
          filtering_candidates,filtering_candidates_buffered,pattern,gpu_buffer_bpm_align);
  // PROF
  PROF_ADD_COUNTER(GP_CANDIDATE_TILES,total_candidates_added);
  PROF_ADD_COUNTER(GP_BMP_DISTANCE_NUM_TILES_VERIFIED,total_candidates_added);
}
/*
 * Compute filtering-region distance
 */
void filtering_candidates_buffered_bpm_align_compute_distance(
    filtering_candidates_t* const filtering_candidates,
    filtering_region_t* const filtering_region,
    alignment_t* const alignment,
    pattern_t* const pattern) {
  // Parameters
  archive_text_t* const archive_text = filtering_candidates->archive->text;
  mm_allocator_t* const mm_allocator = filtering_candidates->mm_allocator;
  // Push
  mm_allocator_push_state(mm_allocator);
  // Retrieve text
  const uint64_t candidate_position = filtering_region->text_begin_position;
  const uint64_t candidate_length =
      filtering_region->text_end_position-filtering_region->text_begin_position;
  archive_text_retrieve(
      archive_text,candidate_position,candidate_length,
      false,false,&filtering_region->text_trace,
      filtering_candidates->mm_allocator);
  text_trace_t* const text_trace = &filtering_region->text_trace;
  // Myers's BPM algorithm [EditFilter]
  alignment_verify_edit_bpm(
      alignment,&pattern->pattern_tiled,
      pattern->key,text_trace->text_padded,
      pattern->max_effective_filtering_error);
  // Pop
  mm_allocator_pop_state(mm_allocator);
}
/*
 * Retrieve filtering-region alignment from GPU-buffer
 */
void filtering_candidates_buffered_bpm_align_retrieve_alignment(
    filtering_candidates_t* const filtering_candidates,
    filtering_region_t* const filtering_region,
    alignment_t* const alignment,
    pattern_t* const pattern,
    const uint64_t max_error,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align,
    const uint64_t candidate_tile_idx) {
  // Parameters
  const uint64_t num_tiles = alignment->num_tiles;
  alignment_tile_t* const alignment_tiles = alignment->alignment_tiles;
  // Traverse all tiles
  uint64_t tile_pos;
  alignment->distance_min_bound = 0;
  for (tile_pos=0;tile_pos<num_tiles;++tile_pos) {
    alignment_tile_t* const alignment_tile = alignment_tiles + tile_pos;
    pattern_tile_t* const pattern_tile = pattern->pattern_tiled.tiles + tile_pos;
    if (alignment_tile->distance!=ALIGN_DISTANCE_UNKNOWN) {
      if (alignment_tile->distance==ALIGN_DISTANCE_INF) {
        alignment->distance_min_bound = ALIGN_DISTANCE_INF;
      } else {
        alignment->distance_min_bound += alignment_tile->distance;
      }
    } else {
      // Retrieve alignment distance/column
      uint32_t tile_distance=0, tile_match_column=0;
      gpu_buffer_bpm_align_get_result(gpu_buffer_bpm_align,
          candidate_tile_idx+tile_pos,&tile_distance,&tile_match_column);
      if (tile_distance > pattern_tile->max_error) { // As CPU version
        alignment_tile->distance = ALIGN_DISTANCE_INF;
        alignment->distance_min_bound = ALIGN_DISTANCE_INF;
      }
      // Offsets
      const uint64_t tile_offset = alignment_tile->text_begin_offset;
      const uint64_t tile_end_offset = tile_match_column+1;
      const uint64_t tile_tall = pattern_tile->tile_length;
      const uint64_t tile_begin_offset =
          BOUNDED_SUBTRACTION(tile_end_offset,tile_tall+tile_distance,0);
      alignment_tile->distance = tile_distance;
      alignment_tile->text_end_offset = tile_offset + tile_end_offset;
      alignment_tile->text_begin_offset = tile_offset + tile_begin_offset;
      // DEBUG
      #ifdef GPU_CHECK_BPM_ALIGN
      filtering_candidates_bpm_align_buffered_check_tile_distance(
          filtering_candidates,pattern_tile->bpm_pattern_tile,gpu_buffer_bpm_align,
          candidate_tile_idx+tile_pos,tile_distance,tile_match_column);
      #endif
      // Check global distance
      if (alignment->distance_min_bound != ALIGN_DISTANCE_INF) {
        alignment->distance_min_bound += alignment_tile->distance;
      }
    }
  }
  // Check global distance
  if (alignment->distance_min_bound != ALIGN_DISTANCE_INF &&
      alignment->distance_min_bound > max_error) {
    alignment->distance_min_bound = ALIGN_DISTANCE_INF;
  }
  PROF_ADD_COUNTER(GP_ASSW_VERIFY_CANDIDATES_TILES_RETRIVED,num_tiles);
  // DEBUG
  #ifdef GPU_CHECK_BPM_ALIGN
  filtering_candidates_bpm_align_buffered_check_global_distance(
      filtering_candidates,filtering_region,
      pattern->pattern_tiled.bpm_pattern,
      alignment->distance_min_bound);
  #endif
}
void filtering_candidates_buffered_bpm_align_retrieve_region(
    filtering_candidates_t* const filtering_candidates,
    filtering_region_t* const filtering_region,
    const uint64_t candidate_tile_idx,
    pattern_t* const pattern,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align) {
  // Parameters
  const uint64_t key_length = pattern->key_length;
  const uint64_t max_error = pattern->max_effective_filtering_error;
  alignment_t* const alignment = &filtering_region->alignment;
  // Detect exact-matches
  if (filtering_region->alignment.distance_min_bound == 0) {
    filtering_region->status = filtering_region_accepted; // Accepted candidate
    vector_insert(filtering_candidates->filtering_regions,filtering_region,filtering_region_t*);
    PROF_INC_COUNTER(GP_ACCEPTED_REGIONS);
    return;
  }
  // Detect trimmed matches
  const uint64_t text_length =
      filtering_region->text_end_position - filtering_region->text_begin_position;
  if (key_length > text_length) {
    if (filtering_region_verify(filtering_candidates,filtering_region,false,pattern)) {
      PROF_INC_COUNTER(GP_ACCEPTED_REGIONS);
      vector_insert(filtering_candidates->filtering_regions,filtering_region,filtering_region_t*);
    } else {
      PROF_INC_COUNTER(GP_DISCARDED_REGIONS);
      vector_insert(filtering_candidates->discarded_regions,filtering_region,filtering_region_t*);
    }
    return;
  }
  // Detect already discarded region
  if (alignment->distance_min_bound==ALIGN_DISTANCE_INF) {
    filtering_region->status = filtering_region_verified_discarded;
    vector_insert(filtering_candidates->discarded_regions,filtering_region,filtering_region_t*);
    return;
  }
  // Retrieve & compose verified region
  if (gpu_buffer_bpm_align->bpm_align_enabled) {
    filtering_candidates_buffered_bpm_align_retrieve_alignment(
        filtering_candidates,filtering_region,alignment,pattern,
        max_error,gpu_buffer_bpm_align,candidate_tile_idx);
  } else {
    filtering_candidates_buffered_bpm_align_compute_distance(
        filtering_candidates,filtering_region,alignment,pattern);
  }
  if (alignment->distance_min_bound <= max_error) {
    filtering_region->status = filtering_region_accepted; // Accepted candidate
    vector_insert(filtering_candidates->filtering_regions,filtering_region,filtering_region_t*);
    PROF_INC_COUNTER(GP_ACCEPTED_REGIONS);
  } else {
    filtering_region->status = filtering_region_verified_discarded; // Discarded candidate
    alignment->distance_min_bound = ALIGN_DISTANCE_INF; // To force CPU/GPU same
    vector_insert(filtering_candidates->discarded_regions,filtering_region,filtering_region_t*);
    PROF_INC_COUNTER(GP_DISCARDED_REGIONS);
  }
}
void filtering_candidates_buffered_bpm_align_retrieve(
    filtering_candidates_t* const filtering_candidates,
    filtering_candidates_buffered_t* const filtering_candidates_buffered,
    pattern_t* const pattern,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align,
    const uint64_t gpu_buffer_align_offset) {
  PROFILE_START(GP_FC_VERIFY_CANDIDATES_BUFFERED,PROFILE_LEVEL);
  // Check filtering regions
  const uint64_t num_filtering_regions = filtering_candidates_buffered->num_regions;
  if (num_filtering_regions==0) return; // No filtering regions
  // Traverse all filtering regions buffered
  uint64_t region_pos, candidate_tile_idx = gpu_buffer_align_offset;
  for (region_pos=0;region_pos<num_filtering_regions;++region_pos) {
    // Retrieve region
    filtering_region_t* const filtering_region = filtering_candidates_buffered->regions[region_pos];
    filtering_candidates_buffered_bpm_align_retrieve_region(
        filtering_candidates,filtering_region,candidate_tile_idx,
        pattern,gpu_buffer_bpm_align);
    candidate_tile_idx += filtering_region->alignment.num_tiles;
  }
  // Free buffered regions
  filtering_candidates_buffered_free_regions(filtering_candidates_buffered);
  // DEBUG
  PROFILE_STOP(GP_FC_VERIFY_CANDIDATES_BUFFERED,PROFILE_LEVEL);
  gem_cond_debug_block(DEBUG_FILTERING_CANDIDATES) {
    tab_fprintf(gem_log_get_stream(),"[GEM]>Filtering.Candidates (verify_regions_BPM_buffer)\n");
    tab_global_inc();
    filtering_candidates_print_regions(gem_log_get_stream(),filtering_candidates,false);
    tab_global_dec();
  }
}
/*
 * BPM-Buffered Checkers (Candidates Verification)
 */
void filtering_candidates_buffered_bpm_align_check_tile_distance(
    filtering_candidates_t* const filtering_candidates,
    bpm_pattern_t* const bpm_pattern_tile,
    gpu_buffer_bpm_align_t* const gpu_buffer_bpm_align,
    const uint64_t candidate_idx,
    const uint32_t tile_distance,
    const uint32_t tile_match_column) {
  // Parameters
  archive_text_t* const archive_text = filtering_candidates->archive->text;
  mm_allocator_t* const mm_allocator = filtering_candidates->mm_allocator;
  // Push
  mm_allocator_push_state(mm_allocator);
  // Get Candidate & Pattern
  uint64_t candidate_text_position;
  uint32_t candidate_length;
  gpu_buffer_bpm_align_get_candidate(gpu_buffer_bpm_align,
      candidate_idx,&candidate_text_position,&candidate_length);
  // Retrieve Candidate Text
  text_trace_t text_trace;
  archive_text_retrieve(
      archive_text,candidate_text_position,candidate_length,
      false,false,&text_trace,filtering_candidates->mm_allocator);
  const uint8_t* const text = text_trace.text; // Candidate
  uint64_t i, uncalled_bases_text = 0;
  for (i=0;i<candidate_length;++i) {
    if (text[i]==ENC_DNA_CHAR_N) ++uncalled_bases_text;
  }
  // Align BPM & Set result
  uint64_t check_tile_match_end_column, check_tile_distance;
  bpm_compute_edit_distance(
      bpm_pattern_tile,text,candidate_length,&check_tile_distance,
      &check_tile_match_end_column,bpm_pattern_tile->pattern_length,false);
  if (tile_distance!=check_tile_distance || tile_match_column!=check_tile_match_end_column) {
    if (uncalled_bases_text == 0) {
      gem_fatal_error_msg("Filtering.Candidates.BPM.Distance.Buffered. Check verify candidate "
          "(Distance:%d!=%lu) (MatchPos:%d!=%lu) (Text.Uncalled.bases=%lu)",
          tile_distance,check_tile_distance,tile_match_column,
          check_tile_match_end_column,uncalled_bases_text);
    }
  }
  // Pop
  mm_allocator_pop_state(mm_allocator);
}
void filtering_candidates_buffered_bpm_align_check_global_distance(
    filtering_candidates_t* const filtering_candidates,
    filtering_region_t* const filtering_region,
    bpm_pattern_t* const bpm_pattern,
    const uint64_t global_distance) {
  // Parameters
  archive_text_t* const archive_text = filtering_candidates->archive->text;
  // Retrieve text
  const uint64_t candidate_position = filtering_region->text_begin_position;
  const uint64_t candidate_length =
      filtering_region->text_end_position-filtering_region->text_begin_position;
  archive_text_retrieve(
      archive_text,candidate_position,candidate_length,
      false,false,&filtering_region->text_trace,
      filtering_candidates->mm_allocator);
  const text_trace_t* const text_trace = &filtering_region->text_trace;
  // Check Whole-Read
  uint64_t match_end_column, match_distance;
  bpm_compute_edit_distance(bpm_pattern,text_trace->text,text_trace->text_length,
      &match_distance,&match_end_column,bpm_pattern->pattern_length,false);
  //  if (!(global_distance <= match_distance &&
  //        match_distance <= global_distance+distance_link_tiles)) {
  //  gem_slog(">FC.BPM.Distance.Candidate.Buffered.Distance\t"
  //      "Whole.Read=%lu\tTileWise={bound=%lu}\tDiff=%lu\n",
  //      match_distance,global_distance,ABS(match_distance-global_distance));
  //  }
  PROF_ADD_COUNTER(GP_FC_VERIFY_CANDIDATES_BUFFERED_DDIFF,
      ABS((int64_t)match_distance-(int64_t)global_distance));
}

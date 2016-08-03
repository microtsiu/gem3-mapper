/*
 * PROJECT: GEMMapper
 * FILE: archive_search_pe.c
 * DATE: 06/06/2012
 * AUTHOR(S): Santiago Marco-Sola <santiagomsola@gmail.com>
 */

#include "archive/archive_search_pe.h"
#include "archive/archive_search_pe_stages.h"
#include "archive/archive_select.h"
#include "archive/archive_score_pe.h"
#include "archive/archive_check.h"

/*
 * Debug
 */
#define DEBUG_ARCHIVE_SEARCH_PE GEM_DEEP_DEBUG

/*
 * Profile
 */
#define PROFILE_LEVEL PHIGH

/*
 * Constants
 */
#define ARCHIVE_SEARCH_PE_EXTENSION_MAX_READ_LENGTH 500

/*
 * Memory Injection (Support Data Structures)
 */
void archive_search_pe_inject_mm(
    archive_search_t* const archive_search_end1,
    archive_search_t* const archive_search_end2,
    mm_search_t* const mm_search) {
  // Search end/1
  archive_search_inject_mm_stack(archive_search_end1,mm_search->mm_stack);
  archive_search_inject_mapper_stats(archive_search_end1,mm_search->mapper_stats);
  archive_search_inject_interval_set(archive_search_end1,&mm_search->interval_set);
  archive_search_inject_text_collection(archive_search_end1,&mm_search->text_collection);
  archive_search_inject_filtering_candidates(archive_search_end1,
      &mm_search->filtering_candidates_forward_end1,
      &mm_search->filtering_candidates_reverse_end1,
      &mm_search->text_collection,mm_search->mm_stack);
  // Search end/2
  archive_search_inject_mm_stack(archive_search_end2,mm_search->mm_stack);
  archive_search_inject_mapper_stats(archive_search_end2,mm_search->mapper_stats);
  archive_search_inject_interval_set(archive_search_end2,&mm_search->interval_set);
  archive_search_inject_text_collection(archive_search_end2,&mm_search->text_collection);
  archive_search_inject_filtering_candidates(archive_search_end2,
      &mm_search->filtering_candidates_forward_end2,
      &mm_search->filtering_candidates_reverse_end2,
      &mm_search->text_collection,mm_search->mm_stack);
}
/*
 * Archive Search PE Continue Search
 */
void archive_search_pe_continue(
    archive_search_t* const archive_search_end1,
    archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  PROFILE_START(GP_ARCHIVE_SEARCH_PE,PROFILE_LEVEL);
  // Callback (switch to proper search stage)
  while (true) {
    switch (archive_search_end1->pe_search_state) {
      case archive_search_pe_state_begin: // Beginning of the search (Init)
        archive_search_pe_begin(archive_search_end1,archive_search_end2,paired_matches);
        break;
      case archive_search_pe_state_search_end1:
        archive_search_pe_search_end1(archive_search_end1,archive_search_end2,paired_matches);
        break;
      case archive_search_pe_state_search_end2:
        archive_search_pe_search_end2(archive_search_end1,archive_search_end2,paired_matches);
        break;
      case archive_search_pe_state_recovery:
        archive_search_pe_recovery(archive_search_end1,archive_search_end2,paired_matches);
        break;
      case archive_search_pe_state_find_pairs:
        archive_search_pe_find_pairs(archive_search_end1,archive_search_end2,paired_matches);
        break;
      case archive_search_pe_state_end:
        archive_search_pe_end(archive_search_end1,archive_search_end2,paired_matches);
        return;
      default:
        GEM_INVALID_CASE();
        break;
    }
  }
  PROFILE_STOP(GP_ARCHIVE_SEARCH_PE,PROFILE_LEVEL);
}
/*
 * Paired-End Indexed Search (PE Online Approximate String Search)
 */
void archive_search_pe(
    archive_search_t* const archive_search_end1,
    archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  gem_cond_debug_block(DEBUG_ARCHIVE_SEARCH_PE) {
    tab_fprintf(stderr,"[GEM]>ArchiveSearch.PE\n");
    tab_fprintf(gem_log_get_stream(),"  => Tag %s\n",archive_search_end1->sequence.tag.buffer);
    tab_fprintf(gem_log_get_stream(),"  => Sequence/1 %s\n",archive_search_end1->sequence.read.buffer);
    tab_fprintf(gem_log_get_stream(),"  => Sequence/2 %s\n",archive_search_end2->sequence.read.buffer);
    tab_global_inc();
  }
  // Parameters
  search_parameters_t* const search_parameters = &archive_search_end1->search_parameters;
  // Init
  archive_search_end1->pe_search_state = archive_search_pe_state_begin;
  // PE search
  archive_search_pe_continue(archive_search_end1,archive_search_end2,paired_matches);
  // PE select
  archive_select_pe_matches(archive_search_end1,archive_search_end2,
      &search_parameters->select_parameters_report,paired_matches);
  // PE Score (Select alignment-Model and process accordingly)
  archive_score_matches_pe(archive_search_end1,archive_search_end2,paired_matches);
  // PE Check matches
  if (search_parameters->check_type!=archive_check_nothing) {
    archive_check_pe_matches(
        archive_search_end1->archive,search_parameters->alignment_model,
        &search_parameters->swg_penalties,&archive_search_end1->sequence,
        &archive_search_end2->sequence,paired_matches,
        search_parameters->check_type,archive_search_end1->mm_stack);
  }
  // DEBUG
  gem_cond_debug_block(DEBUG_ARCHIVE_SEARCH_PE) {
    tab_global_inc();
    archive_search_pe_print(gem_log_get_stream(),archive_search_end1,archive_search_end2,paired_matches);
    tab_global_dec();
    tab_global_dec();
  }
}
/*
 * Display
 */
void archive_search_pe_print(
    FILE* const stream,
    archive_search_t* const archive_search_end1,
    archive_search_t* const archive_search_end2,
    paired_matches_t* const paired_matches) {
  tab_fprintf(stream,"[GEM]>ArchiveSearch.PE\n");
  tab_global_inc();
  tab_fprintf(stream,"=> PE.Input\n");
  tab_fprintf(stream,"=>   Tag %s\n",archive_search_end1->sequence.tag.buffer);
  tab_fprintf(stream,"=>   Sequence/1 %s\n",archive_search_end1->sequence.read.buffer);
  tab_fprintf(stream,"=>   Sequence/2 %s\n",archive_search_end2->sequence.read.buffer);
  tab_fprintf(stream,"=>   Search.State %s\n",archive_search_pe_state_label[archive_search_end1->pe_search_state]);
  tab_fprintf(stream,"=> ASM.Parameters\n");
  tab_global_inc();
  search_parameters_print(stream,&archive_search_end1->search_parameters);
  tab_global_dec();
  tab_fprintf(stream,"=> End/1\n");
  tab_fprintf(stream,"  => Searched %s\n",archive_search_end1->pair_searched ? "yes" : "no");
  tab_fprintf(stream,"  => Extended %s (shortcut-extension=%s)\n",
      archive_search_end1->pair_extended ? "yes" : "no",
      archive_search_end1->pair_extended_shortcut ? "yes" : "no");
  tab_fprintf(stream,"=> End/2\n");
  tab_fprintf(stream,"  => Searched %s\n",archive_search_end2->pair_searched ? "yes" : "no");
  tab_fprintf(stream,"  => Extended %s\n",archive_search_end2->pair_extended ? "yes" : "no");
  if (!mapper_stats_template_length_is_reliable(archive_search_end1->mapper_stats)) {
    tab_fprintf(stream,"=> Template-length 'n/a'\n");
  } else {
    const double template_length_mean = mapper_stats_template_length_get_mean(archive_search_end1->mapper_stats);
    const double template_length_stddev = mapper_stats_template_length_get_stddev(archive_search_end1->mapper_stats);
    const uint64_t template_length_expected_max = mapper_stats_template_length_get_expected_max(archive_search_end1->mapper_stats);
    const uint64_t template_length_expected_min = mapper_stats_template_length_get_expected_min(archive_search_end1->mapper_stats);
    tab_fprintf(stream,"=> Template-length {min=%lu,max=%lu,mean=%2.1f,stddev==%2.1f}\n",
        template_length_expected_min,template_length_expected_max,template_length_mean,template_length_stddev);
  }
  // Paired Matches
  tab_fprintf(stream,"=> Paired.Matches\n");
  tab_global_inc();
  paired_matches_print(stream,paired_matches);
  tab_global_dec();
  tab_global_dec();
}


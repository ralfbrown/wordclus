/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcglobal.cpp	      global variables				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2002,2003,2005,2009,2010,2015			*/
/*	   Ralf Brown/Carnegie Mellon University			*/
/*	This program is free software; you can redistribute it and/or	*/
/*	modify it under the terms of the GNU Lesser General Public 	*/
/*	License as published by the Free Software Foundation, 		*/
/*	version 3.							*/
/*									*/
/*	This program is distributed in the hope that it will be		*/
/*	useful, but WITHOUT ANY WARRANTY; without even the implied	*/
/*	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR		*/
/*	PURPOSE.  See the GNU Lesser General Public License for more 	*/
/*	details.							*/
/*									*/
/*	You should have received a copy of the GNU Lesser General	*/
/*	Public License (file COPYING) and General Public License (file	*/
/*	GPL.txt) along with this program.  If not, see			*/
/*	http://www.gnu.org/licenses/					*/
/*									*/
/************************************************************************/

#include "FramepaC.h"
#include "wcconfig.h"
#include "wordclus.h"
#include "wcglobal.h"

/************************************************************************/
/*	Manifest constants for this module				*/
/************************************************************************/

#define MAX_LINE 8192

/************************************************************************/
/*	Global variables shared with other modules			*/
/************************************************************************/

WcGlobalVariables wc_vars ;

void EbSetCharEncoding(const char *enc_name) ;

/************************************************************************/
/*	Global variables for this module				*/
/************************************************************************/

static WcGlobalVariables *active_vars = nullptr ;
static WcGlobalVariables *default_vars = nullptr ;

/************************************************************************/
/*	Methods for class WcGlobalVariables				*/
/************************************************************************/

WcGlobalVariables::WcGlobalVariables()
{
   applyConfiguration(0) ;
   return ;
}

//----------------------------------------------------------------------

WcGlobalVariables::WcGlobalVariables(const WcGlobalVariables &vars)
{
   memcpy(this,&vars,sizeof(WcGlobalVariables)) ;
   // fix up the items which are allocated on the heap
   if (_tightbound_left)
      _tightbound_left = (FrList*)_tightbound_left->deepcopy() ;
   if (_tightbound_right)
      _tightbound_right = (FrList*)_tightbound_right->deepcopy() ;
   return ;
}

//----------------------------------------------------------------------

WcGlobalVariables::~WcGlobalVariables()
{
   if (this == active_vars)
      default_vars->select() ;
   if (this == &wc_vars)
      {
      return ;
      }
   freeVariables() ;
   return ;
}

//----------------------------------------------------------------------

void WcGlobalVariables::freeVariables()
{
   if (this == &wc_vars)
      {
      free_object(_tightbound_left) ;	_tightbound_left = nullptr ;
      free_object(_tightbound_right) ;	_tightbound_right = nullptr ;
      }
   return ;
}

//----------------------------------------------------------------------

void WcGlobalVariables::applyConfiguration(const WcConfig *config)
{
   _only_count_pairs = false ;
   _only_incr_contexts = false ;
   if (config)
      {
      WcSetCharEncoding(config->char_enc) ;
      uint32_t options = config->options ;
      _verbose = (options & WC_VERBOSE) != 0 ;
      _showmem = (options & WC_SHOWMEM) != 0 ;
      _Unicode_bswap = (options & WC_BYTESWAP) != 0 ;
      _lowercase_source = (options & WC_IGNORE_SRC_CASE) != 0 ;
      _lowercase_target = (options & WC_IGNORE_TGT_CASE) != 0 ;
      _lowercase_output = (options & WC_DOWNCASE) != 0 ;
      _keep_numbers_distinct = (options & WC_SEP_NUMBERS) != 0 ;
      _keep_punctuation_distinct = (options & WC_SEP_PUNCT) != 0 ;
      _keep_singletons = (options & WC_KEEP_SINGLETONS) != 0 ;
      _phrasal_context = (options & WC_PHRASAL_CONTEXT) != 0 ;
      _single_phrase_context = (options & WC_SINGLE_CONTEXT) != 0 ;
      _remove_ambig_terms = (options & WC_REMOVE_AMBIG) != 0 ;
      _monolingual = (options & WC_MONOLINGUAL) != 0 ;
      _two_pass_clustering = (options & WC_TWOPASS) != 0 ;
      _iterated_grammar = (options & WC_ITERATEDGRAMMAR) != 0 ;
      _sum_cluster_sizes = (options & WC_SUM_SIZES) != 0 ;
      _fill_bitext_gaps = (options & WC_NOFILLGAPS) == 0 ;
      _exclude_punctuation = (options & WC_EXCL_PUNCT) != 0 ;
      _exclude_numbers = (options & WC_EXCL_NUM) != 0 ;
      _use_seeds_for_context_only = (options & WC_CONTEXTONLY) != 0 ;
      _ignore_auto_clusters = (options & WC_SKIPAUTOCLUST) != 0 ;
      if ((options & WC_EXPDECAY) != 0)
	 _decay_type = Decay_Exponential ;
      else if ((options & WC_LINDECAY) != 0)
	 _decay_type = Decay_Linear ;
      else if ((options & WC_DECAY) == 0)
	 _decay_type = Decay_None ;
      else
	 _decay_type = Decay_Reciprocal ;
      if (config->word_order >= 0)
	 _word_order_similarity = (int)config->word_order ;
      if (config->tight_bound_left)
	 _tightbound_left = (FrList*)config->tight_bound_left->deepcopy() ;
      if (config->tight_bound_right)
	 _tightbound_right = (FrList*)config->tight_bound_right->deepcopy() ;
      if (config->past_bound_weight >= 0.0)
	 _past_boundary_weight = config->past_bound_weight ; 
      if (config->sl_context_left > 0)
	 _neighborhood_left = (size_t)config->sl_context_left ;
      if (config->sl_context_right > 0)
	 _neighborhood_right = (size_t)config->sl_context_right ;
      if (config->tl_context_left > 0)
	 _tl_context_l = (size_t)config->tl_context_left ;
      if (config->tl_context_right > 0)
	 _tl_context_r = (size_t)config->tl_context_right ;
      if (config->min_freq > 0)
	 _min_frequency = (size_t)config->min_freq ;
      if (config->max_freq > 0)
	 _max_frequency = (size_t)config->max_freq ;
      if (config->max_ambig > 0)
	 _max_wordpair_ambig = (size_t)config->max_ambig ;
      if (config->max_diff > 0)
	 _max_wordpair_difference = (size_t)config->max_diff ;
      if (config->thresh_file)
	 _threshold_file = FrDupString(config->thresh_file) ;
      if (config->clust_metric)
	 {
	 const char *metric = config->clust_metric->symbolName() ;
	 _clustering_metric = FrParseClusterMetric(metric,&cerr) ;
	 }
      if (config->clust_rep)
	 {
	 const char *rep = config->clust_rep->symbolName() ;
	 _clustering_rep = FrParseClusterRep(rep,&cerr) ;
	 }
      if (config->clust_type)
	 {
	 const char *type = config->clust_type->symbolName() ;
	 _clustering_method = FrParseClusterMethod(type,&cerr) ;
	 }
      if (config->clust_iter > 0)
	 _clustering_iter = config->clust_iter ;
      }
   else
      {
      // use the default configuration
      // bool
      _verbose = false ;
      _showmem = false ;
      _Unicode_bswap = false ;
      _monolingual = true /*false*/ ;
      _two_pass_clustering = false ;
      _lowercase_output = false ;
      _lowercase_source = true ;
      _lowercase_target = false ;
      _reverse_languages = false ;
      _reverse_tokens = false ;
      _keep_numbers_distinct = false ;
      _keep_punctuation_distinct = false ;
      _keep_singletons = false ;
      _phrasal_context = false ;
      _single_phrase_context = false ;
      _remove_ambig_terms = false ;
      _sum_cluster_sizes = false ;
      _fill_bitext_gaps = true ;
      _exclude_punctuation = false ;
      _use_seeds_for_context_only = false ;
      _ignore_auto_clusters = false ;

      // int
      _word_order_similarity = 0 ;
      _neighborhood_left = 3 ;
      _neighborhood_right = 3 ;
      _tl_context_l = 0 ;
      _tl_context_r = 0 ;
      _min_frequency = 2 ;
      _max_frequency = ULONG_MAX ;
      _max_term_count = ULONG_MAX ;
      _stop_term_count = 0 ;
      _max_wordpair_ambig = 1 ;
      _max_wordpair_difference = 16 ;

      // double
      _past_boundary_weight = 0.5 ;
      _decay_alpha = 0.5 ;
      _decay_beta = 0.5 ;
      _decay_gamma = 0.1 ;
      _termfreq_discount = 1.0 ;

      // char*
      _threshold_file = nullptr ;

      // istream*

      // ostream*

      // FrSymbol*

      // enum
      _decay_type = Decay_Reciprocal ;

      // FrList*
      _tightbound_left = nullptr ;
      _tightbound_right = nullptr ;

      // other
      _char_encoding = FrChEnc_Latin1 ;
      _clustering_method = FrCM_GROUPAVERAGE ;
      _clustering_rep = FrCR_CENTROID ;
      _clustering_metric = FrCM_COSINE ;
      _clustering_iter = 5 ;
      }
   return ;
}

//----------------------------------------------------------------------

WcGlobalVariables *WcGlobalVariables::select()
{
   WcGlobalVariables *prev = active_vars ;
   if (!active_vars && !default_vars)
      default_vars = active_vars = new WcGlobalVariables(wc_vars) ;
   if (this == 0)
      {
      if (default_vars)
	 return default_vars->select() ;
      else
	 return prev ;
      }
   if (this != active_vars)
      {
      if (prev)
	 memcpy(prev,&wc_vars,sizeof(WcGlobalVariables)) ;
      if (this != &wc_vars)
	 memcpy(&wc_vars,this,sizeof(WcGlobalVariables)) ;
      active_vars = this ;
      return prev ? prev : active_vars ;
      }
   return prev ;
}

//----------------------------------------------------------------------

FrCharEncoding WcCurrentCharEncoding()
{
   return char_encoding ;
}

//----------------------------------------------------------------------

void WcSetCharEncoding(const char *char_enc)
{
   char_encoding = FrParseCharEncoding(char_enc) ;
   return ;
}

// end of file wcglobal.cpp //

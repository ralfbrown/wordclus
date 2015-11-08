/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcglobal.h	      global variables				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2001,2002,2003,2005,2006,2008,2009,2010,2015	*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#ifndef __WCGLOBAL_H_INCLUDED
#define __WCGLOBAL_H_INCLUDED

//----------------------------------------------------------------------

class WcConfig ;

class WcGlobalVariables
   {
   public:
      double _past_boundary_weight ;
      double _decay_alpha ;
      double _decay_beta ;
      double _decay_gamma ;
      double _termfreq_discount ;
      WcDecayType _decay_type ;
      bool _verbose ;
      bool _showmem ;
      bool _Unicode_bswap ;
      bool _monolingual ;
      bool _two_pass_clustering ;
      bool _iterated_grammar ;
      bool _lowercase_source ;
      bool _lowercase_target ;
      bool _lowercase_output ;
      bool _reverse_languages ;
      bool _reverse_tokens ;
      bool _keep_numbers_distinct ;
      bool _keep_punctuation_distinct ;
      bool _keep_singletons ;
      bool _phrasal_context ;
      bool _single_phrase_context ;
      bool _remove_ambig_terms ;
      bool _sum_cluster_sizes ;
      bool _fill_bitext_gaps ;
      bool _exclude_punctuation ;
      bool _exclude_numbers ;
      bool _use_seeds_for_context_only ;
      bool _ignore_auto_clusters ;
      bool _only_count_pairs ;
      bool _only_incr_contexts ;
      FrList *_tightbound_left ;
      FrList *_tightbound_right ;
      char   *_threshold_file ;
      int    _word_order_similarity ;
      size_t _min_frequency ;
      size_t _max_frequency ;
      size_t _max_term_count ;
      size_t _stop_term_count ;
      size_t _max_wordpair_ambig ;
      size_t _max_wordpair_difference ;
      size_t _neighborhood_left ;
      size_t _neighborhood_right ;
      size_t _tl_context_l ;
      size_t _tl_context_r ;
      size_t _clustering_iter ;
      FrCharEncoding _char_encoding ;
      FrCasemapTable _lowercase_table ;
      FrClusteringMethod _clustering_method ;
      FrClusteringRep _clustering_rep ;
      FrClusteringMeasure _clustering_metric ;
      const char *_clustering_settings ;
   public: // methods
      WcGlobalVariables() ;
      WcGlobalVariables(const WcGlobalVariables &) ;
      ~WcGlobalVariables() ;
      void freeVariables() ;

      void applyConfiguration(const WcConfig *config) ;
      WcGlobalVariables *select() ;
   } ;

//----------------------------------------------------------------------

extern WcGlobalVariables wc_vars ;

/************************************************************************/
/************************************************************************/

#define Unicode_bswap 		wc_vars._Unicode_bswap
#define conserve_memory		wc_vars._conserve_memory
#define verbose			wc_vars._verbose
#define showmem			wc_vars._showmem
#define monolingual		wc_vars._monolingual
#define two_pass_clustering	wc_vars._two_pass_clustering
#define iterated_grammar	wc_vars._iterated_grammar
#define lowercase_source	wc_vars._lowercase_source
#define lowercase_target	wc_vars._lowercase_target
#define lowercase_output	wc_vars._lowercase_output
#define reverse_languages	wc_vars._reverse_languages
#define reverse_tokens		wc_vars._reverse_tokens
#define decay_type		wc_vars._decay_type
#define decay_alpha		wc_vars._decay_alpha
#define decay_beta		wc_vars._decay_beta
#define decay_gamma		wc_vars._decay_gamma
#define termfreq_discount	wc_vars._termfreq_discount
#define tightbound_left		wc_vars._tightbound_left
#define tightbound_right	wc_vars._tightbound_right
#define word_order_similarity	wc_vars._word_order_similarity
#define keep_numbers_distinct	wc_vars._keep_numbers_distinct
#define keep_punctuation_distinct wc_vars._keep_punctuation_distinct
#define keep_singletons		wc_vars._keep_singletons
#define use_phrasal_context	wc_vars._phrasal_context
#define single_phrase_context	wc_vars._single_phrase_context
#define remove_ambig_terms	wc_vars._remove_ambig_terms
#define sum_cluster_sizes	wc_vars._sum_cluster_sizes
#define fill_bitext_gaps	wc_vars._fill_bitext_gaps
#define exclude_punctuation	wc_vars._exclude_punctuation
#define exclude_numbers		wc_vars._exclude_numbers
#define use_seeds_for_context_only wc_vars._use_seeds_for_context_only
#define ignore_auto_clusters	wc_vars._ignore_auto_clusters
#define past_boundary_weight	wc_vars._past_boundary_weight
#define neighborhood_left	wc_vars._neighborhood_left
#define neighborhood_right	wc_vars._neighborhood_right
#define tl_context_l		wc_vars._tl_context_l
#define tl_context_r		wc_vars._tl_context_r
#define min_frequency		wc_vars._min_frequency
#define max_frequency		wc_vars._max_frequency
#define max_term_count		wc_vars._max_term_count
#define stop_term_count		wc_vars._stop_term_count
#define max_wordpair_ambig	wc_vars._max_wordpair_ambig
#define max_wordpair_difference	wc_vars._max_wordpair_difference
#define threshold_file		wc_vars._threshold_file
#define char_encoding		wc_vars._char_encoding
#define lowercase_table		wc_vars._lowercase_table
#define clustering_method	wc_vars._clustering_method
#define clustering_settings	wc_vars._clustering_settings
#define clustering_rep		wc_vars._clustering_rep
#define clustering_metric	wc_vars._clustering_metric
#define clustering_iter		wc_vars._clustering_iter
#define only_count_pairs	wc_vars._only_count_pairs
#define only_incr_contexts	wc_vars._only_incr_contexts

//----------------------------------------------------------------------

inline bool is_punct(const char *word)
{
   if (!word)
      return false ;
   return Fr_ispunct(word[0]) && (Fr_isspace(word[1]) || word[1] == '\0') ;
}

inline bool is_punct(const FrSymbol *word)
{ return is_punct(word->symbolName()) ; }

#endif /* !__WCGLOBAL_H_INCLUDED */

// end of file wcglobal.h //

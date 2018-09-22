/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcparam.h	      WcParameters structure			*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2005,2006,2008,2009,2010,	*/
/*		2015,2016,2017,2018 Carnegie Mellon University		*/
/*	This program may be redistributed and/or modified under the	*/
/*	terms of the GNU General Public License, version 3, or an	*/
/*	alternative license agreement as detailed in the accompanying	*/
/*	file LICENSE.  You should also have received a copy of the	*/
/*	GPL (file COPYING) along with this program.  If not, see	*/
/*	http://www.gnu.org/licenses/					*/
/*									*/
/*	This program is distributed in the hope that it will be		*/
/*	useful, but WITHOUT ANY WARRANTY; without even the implied	*/
/*	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR		*/
/*	PURPOSE.  See the GNU General Public License for more details.	*/
/*									*/
/************************************************************************/

#ifndef __WCPARAM_H_INCLUDED
#define __WCPARAM_H_INCLUDED

#include "framepac/contextcoll.h"
#include "framepac/cstring.h"
#include "framepac/hashtable.h"

/************************************************************************/
/************************************************************************/

class WcParameters
   {
   private:
      Fr::SymHashTable* m_equiv_classes { nullptr } ;
      class WcWordIDPairTable* m_mutualinfo_id { nullptr } ;
      WcWordCorpus*      m_corpus { nullptr } ;
      Fr::ContextVectorCollection<WcWordCorpus::ID,uint32_t,float,false>* m_contextcoll { nullptr } ;
      size_t             m_min_wordfreq { 0 } ;
      size_t             m_max_wordfreq { UINT_MAX } ;
      size_t             m_rare_threshold { 0 } ;
      size_t             m_max_terms { UINT_MAX } ;
      size_t             m_stop_terms { 0 } ;
      size_t             m_desired_clusters { 100 } ;
      size_t             m_backoff_step { 1 } ;
      size_t             m_iterations { 5 } ;
      size_t             m_src_context_left { 2 } ;  // "neighborhood_left"
      size_t             m_src_context_right { 2 } ; // "neighborhood_right"
      size_t             m_dimensions { 0 } ;
      size_t             m_basis_plus { 4 } ;
      size_t             m_basis_minus { 4 } ;
      int                m_mono_skip { 0 } ;
      unsigned           m_max_equiv_length { 100 } ;
      unsigned           m_max_context_length { 1 } ;
      size_t             m_phrase_length { 1 } ;
      double             m_threshold { 0.3 } ;
      double             MI_threshold { 0.0 } ;
      WcMIScoreFuncID*   m_mi_score_func_id { nullptr } ;
      WcWordFreqProcFunc* m_wordfreq_func { nullptr } ;
      WcVectorFilterFunc* m_prefilter_func { nullptr } ;
      WcVectorFilterFunc* m_postfilter_func { nullptr } ;
      WcGlobalFilterFunc* m_gpostfilter_func { nullptr } ;
      WcClusterFilterFunc* m_clusterfilter_func { nullptr } ;
      WcClusterPostprocFunc* m_clusterpostproc_func { nullptr } ;
      void*       m_mi_score_data { nullptr } ;
      void*       m_tvsim_data { nullptr } ;
      void*       m_clustersim_data { nullptr } ;
      void*       m_prefilter_data { nullptr } ;
      void*       m_postfilter_data { nullptr } ;
      void*       m_gpostfilter_data { nullptr } ;
      void*       m_clusterfilter_data { nullptr } ;
      void*       m_clusterpostproc_data { nullptr } ;
      const char* m_cluster_method { nullptr } ;
      const char* m_cluster_measure { nullptr } ;
      const char* m_cluster_rep { nullptr } ;
      const char* m_cluster_settings { nullptr } ;
      const char* m_stopwords_file { nullptr } ;
      const char* m_equiv_class_file { nullptr } ;
      const char* m_context_equivs_file { nullptr } ;
      bool        m_verbose { false } ;
      bool        m_showmem { false } ;
      bool        m_use_chi_squared { false } ;
      bool        m_downcase_source { false } ;
      bool        m_hard_cluster_limit { true } ;
      bool        m_ignore_auto_clusters { false } ;
      bool        m_ignore_beyond_cluster_limit { false } ; 
      bool        m_ignore_unseen_seeds { false } ;
      bool        m_suppress_auto_brackets { false } ;
      bool        m_recluster_seeds { false } ;
      bool        m_postfilter_needs_keytab { false } ;
      bool        m_punct_as_stopwords { false } ;
      bool        m_exclude_numbers { false } ;
      bool        m_exclude_punct { false } ;
      bool        m_keep_singletons { false } ;
   public:
      double      m_past_boundary_weight { 0.5 } ;
      double      m_decay_alpha { 0.5 } ;
      double      m_decay_beta { 0.5 } ;
      double      m_decay_gamma { 0.1 } ;
      double      m_termfreq_discount { 1.0 } ;
      size_t      left_context { 0 } ;	// extra words used to disambiguate senses when clustering
      size_t      right_context { 0 } ;
      WcDecayType m_decay_type { Decay_Reciprocal } ;
      bool        m_skip_auto_clusters { false } ;
      bool        m_all_lengths { true } ;
      bool        m_auto_numbers { true } ;
      bool        m_have_markup { true } ;
      bool        m_no_period_MI { false } ;
      bool        m_distinct_numbers { false } ;
      bool        m_distinct_punct { false } ;

   protected:
      void init() {}

   public: // methods
      WcParameters() { }
      WcParameters(const WcParameters &params) = default ;
      WcParameters(const WcParameters *params)
	 { if (params) memcpy(this,params,sizeof(WcParameters)) ; }
      WcParameters(size_t min_f, size_t max_f, size_t max_t, size_t stop_t,
		   size_t s_left, size_t s_right, size_t phr, double MI,
		   bool skip = false, bool all_lengths = true, bool auto_numbers = true)
	 : WcParameters()
	 { m_min_wordfreq = min_f ; m_max_wordfreq = max_f ;
	   m_max_terms = max_t ; m_stop_terms = stop_t ;
	   m_src_context_left = s_left ; m_src_context_right = s_right ;
	   m_phrase_length = phr ; MI_threshold = MI ;
	   m_skip_auto_clusters = skip ; m_all_lengths = all_lengths ;
	   m_auto_numbers = auto_numbers ;
	 }

      // accessors
      size_t minWordFreq() const { return m_min_wordfreq ; }
      size_t maxWordFreq() const { return m_max_wordfreq ; }
      size_t rareThreshold() const { return m_rare_threshold ; }
      size_t maxTermCount() const { return m_max_terms ; }
      size_t stopTermCount() const { return m_stop_terms ; }
      size_t phraseLength() const { return  m_phrase_length ; }
      size_t desiredClusters() const { return m_desired_clusters ; }
      size_t backoffStep() const { return m_backoff_step ; }
      size_t iterations() const { return m_iterations ; }
      size_t neighborhoodLeft() const { return m_src_context_left ; }
      size_t neighborhoodRight() const { return m_src_context_right ; }
      size_t dimensions() const { return m_dimensions ; }
      size_t basisPlus() const { return m_basis_plus ; }
      size_t basisMinus() const { return m_basis_minus ; }
      double miThreshold() const { return MI_threshold ; }
      double clusteringThreshold() const { return m_threshold ; }
      int monoSkip() const { return m_mono_skip ; }
      unsigned maxEquivLength() const { return m_max_equiv_length ; }
      unsigned maxContextEquivLength() const { return m_max_context_length ; }
      bool runVerbosely() const { return m_verbose ; }
      bool showMemory() const { return m_showmem ; }
      bool skipAutoClusters() const { return m_skip_auto_clusters ; }
      bool allLengths() const { return m_all_lengths ; }
      bool autoNumbers() const { return m_auto_numbers ; }
      bool haveMarkup() const { return  m_have_markup ; }
      bool chiSquaredMI() const { return m_use_chi_squared ; }
      bool reclusterSeeds() const { return m_recluster_seeds ; }
      bool postFilterNeedsKeyTable() const { return m_postfilter_needs_keytab ; }
      bool downcaseSource() const { return m_downcase_source ; }
      bool hardClusterLimit() const { return m_hard_cluster_limit ; }
      bool ignoreAutoClusters() const { return m_ignore_auto_clusters ; }
      bool ignoreBeyondClusterLimit() const { return m_ignore_beyond_cluster_limit ; }
      bool ignoreUnseenSeeds() const { return m_ignore_unseen_seeds ; }
      bool suppressAutoBrackets() const { return m_suppress_auto_brackets ; }
      bool excludeNumbers() const { return m_exclude_numbers ; }
      bool excludePunctuation() const { return  m_exclude_punct ; }
      bool punctuationAsStopwords() const { return m_punct_as_stopwords ; }
      bool keepSingletons() const { return m_keep_singletons ; }
      bool noPeriodMutualInfo() const { return m_no_period_MI ; }
      bool keepNumbersDistinct() const { return m_distinct_numbers ; }
      bool keepPunctuationDistinct() const { return m_distinct_punct ; }
      WcMIScoreFuncID *miScoreFuncID() const { return m_mi_score_func_id ; }
      void *miScoreData() const { return m_mi_score_data ; }
      WcWordFreqProcFunc *wordFreqFunc() const { return m_wordfreq_func ; }
      WcVectorFilterFunc *preFilterFunc() const { return m_prefilter_func ; }
      void *preFilterData() const { return m_prefilter_data ; }
      WcVectorFilterFunc *postFilterFunc() const { return m_postfilter_func ; }
      void *postFilterData() const { return m_postfilter_data ; }
      WcGlobalFilterFunc *globalPostFilterFunc() const { return m_gpostfilter_func ; }
      void *globalPostFilterData() const { return m_gpostfilter_data ; }
      WcClusterFilterFunc *clusterFilterFunc() const { return m_clusterfilter_func ; }
      void *clusterFilterData() const { return m_clusterfilter_data ; }
      WcClusterPostprocFunc *clusterPostprocFunc() const { return m_clusterpostproc_func ; }
      void *clusterPostprocData() const { return m_clusterpostproc_data ; }
      Fr::SymHashTable *equivalenceClasses() const { return m_equiv_classes ; }
      const char* equivClassFile() const { return m_equiv_class_file ; }
      const char* contextEquivClassFile() const { return m_context_equivs_file ; }
      const char* clusteringMethod() const { return m_cluster_method ; }
      const char* clusteringMeasure() const { return  m_cluster_measure ; }
      const char* clusteringRep() const { return m_cluster_rep ; }
      const char* clusteringSettings() const { return m_cluster_settings ; }
      const char* stopwordsFile() const { return  m_stopwords_file ; }
      class WcWordIDPairTable *mutualInfoID() const { return m_mutualinfo_id ; }
      WcWordCorpus *corpus() const { return m_corpus ; }
      Fr::ContextVectorCollection<WcWordCorpus::ID,uint32_t,float,false>* contextCollection() const
	 { return m_contextcoll ; }

      // modifiers
      void desiredClusters(size_t cl) { m_desired_clusters = cl ; }
      void backoffStep(size_t bo) { m_backoff_step = bo ; }
      void iterations(size_t iter) { m_iterations = iter ; }
      void neighborhoodLeft(size_t n) { m_src_context_left = n ; }
      void neighborhoodRight(size_t n) { m_src_context_right = n ; }
      void dimensions(size_t dim) { m_dimensions = dim ; }
      void basis(size_t plus, size_t minus) { m_basis_plus = plus ; m_basis_minus = minus ; }
      void minWordFreq(size_t freq) { if (freq > m_min_wordfreq) m_min_wordfreq = freq ; }
      void maxWordFreq(size_t freq) { m_max_wordfreq = freq ; }
      void maxTermCount(size_t count) { m_max_terms = count ; }
      void stopTermCount(size_t count) { m_stop_terms = count ; }
      void phraseLength(size_t len) { m_phrase_length = len ; }
      void rareThreshold(size_t rare) { m_rare_threshold = rare ; }
      void miThreshold(double thr) { MI_threshold = thr ; }
      void clusteringThreshold(double thr) { m_threshold = thr ; }
      void runVerbosely(bool v) { m_verbose = v ; }
      void showMemory(bool sm) { m_showmem = sm ; }
      void skipAutoClusters(bool skip) { m_skip_auto_clusters = skip ; }
      void allLengths(bool all) { m_all_lengths = all ; }
      void monoSkip(int skip) { m_mono_skip = skip ; }
      void autoNumber(bool an) { m_auto_numbers = an ; }
      void haveMarkup(bool markup) { m_have_markup = markup ; }
      void chiSquaredMI(bool chi) { m_use_chi_squared = chi ; }
      void reclusterSeeds(bool re) { m_recluster_seeds = re ; }
      void postFilterNeedsKeyTable(bool kt) { m_postfilter_needs_keytab = kt ; }
      void downcaseSource(bool dc) { m_downcase_source = dc ; }
      void hardClusterLimit(bool hard) { m_hard_cluster_limit = hard ; }
      void ignoreAutoClusters(bool ignore) { m_ignore_auto_clusters = ignore ; }
      void ignoreBeyondClusterLimit(bool ignore) { m_ignore_beyond_cluster_limit = ignore ; }
      void ignoreUnseenSeeds(bool ignore) { m_ignore_unseen_seeds = ignore ; }
      void suppressAutoBrackets(bool suppress) { m_suppress_auto_brackets = suppress ; }
      void excludeNumbers(bool xn) { m_exclude_numbers = xn ; }
      void excludePunctuation(bool xp) { m_exclude_punct = xp ; }
      void punctuationAsStopwords(bool p) { m_punct_as_stopwords = p ; }
      void keepSingletons(bool keep) { m_keep_singletons = keep ; }
      void noPeriodMutualInfo(bool pmi) { m_no_period_MI = pmi ; }
      void keepNumbersDistinct(bool dist) { m_distinct_numbers = dist ; }
      void keepPunctuationDistinct(bool dist) { m_distinct_punct = dist ; }
      void maxEquivLength(unsigned len) { m_max_equiv_length = len ; }
      void maxContextEquivLength(unsigned len) { m_max_context_length = len ; }
      void equivalenceClasses(Fr::SymHashTable *eq) { m_equiv_classes = eq ; }
      void equivClassFile(const char *eq) { m_equiv_class_file = eq ; }
      void contextEquivClassFile(const char *eq) { m_context_equivs_file = eq ; }
      void clusteringMethod(const char* cm) { m_cluster_method = cm ; }
      void clusteringMeasure(const char* cm) { m_cluster_measure = cm ; }
      void clusteringRep(const char* cr) { m_cluster_rep = cr ; }
      void clusteringSettings(const char* cs) { m_cluster_settings = cs ; }
      void stopwordsFile(const char *sw) { m_stopwords_file = sw ; }
      void mutualInfoID(class WcWordIDPairTable *mi) { m_mutualinfo_id = mi ; }
      void corpus(WcWordCorpus *c) { m_corpus = c ; }
      void miScoreFuncID(WcMIScoreFuncID *fn, void *udata) { m_mi_score_func_id = fn ; m_mi_score_data = udata ; }
      void wordFreqFunc(WcWordFreqProcFunc *fn, void * /*udata*/ = nullptr)
	 { m_wordfreq_func = fn ; }
      void preFilterFunc(WcVectorFilterFunc *fn, void *udata)
         { m_prefilter_func = fn ; m_prefilter_data = udata ; }
      void postFilterFunc(WcVectorFilterFunc *fn, void *udata)
         { m_postfilter_func = fn ; m_postfilter_data = udata ; }
      void globalPostFilterFunc(WcGlobalFilterFunc *fn, void *udata)
         { m_gpostfilter_func = fn ; m_gpostfilter_data = udata ; }
      void clusterFilterFunc(WcClusterFilterFunc *fn, void *udata)
         { m_clusterfilter_func = fn ; m_clusterfilter_data = udata ; }
      void clusterPostprocFunc(WcClusterPostprocFunc *fn, void *udata)
	 { m_clusterpostproc_func = fn ; m_clusterpostproc_data = udata ; }	 
      void contextCollection(Fr::ContextVectorCollection<WcWordCorpus::ID,uint32_t,float,false>* c)
	 { m_contextcoll = c ; }
   } ;

/************************************************************************/
/************************************************************************/

Fr::CharPtr WcBuildParameterString(const WcParameters&) ;
bool WcValidateParameters(const WcParameters&) ;

#endif /* !__WCPARAM_H_INCLUDED */

// end of file wcparam.h //

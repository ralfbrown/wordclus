/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wordclus.h	      word clustering (declarations)		*/
/*  LastEdit: 08nov15							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2005,2006,2008,2009,2010,	*/
/*		2015 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __WORDCLUS_H_INCLUDED
#define __WORDCLUS_H_INCLUDED

#include "FramepaC.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

// make a tradeoff: use hash tables instead of sparse arrays to run
//   faster at the cost of more memory by uncommenting the next line
#define TRADE_MEM_FOR_SPEED

#define WORDCLUST_VERSION "1.40"

#define WC_CONCAT_WORDS_SEP '\t'

#ifndef NUMBER_TOKEN
#define NUMBER_TOKEN "<NUMBER>"
#endif

#define WcSORT_OUTPUT true

/************************************************************************/
/*	Types								*/
/************************************************************************/

// forward declarations
class EbAlignConstraints ;
class WcConfig ;

enum WcDecayType
   {
      Decay_None,
      Decay_Reciprocal,
      Decay_Linear,
      Decay_Exponential
   } ;

class WordInfo
   {
   private:
      size_t term_freq ;
      size_t doc_freq ;
      FrSymbol *disambig_symbols[1] ;
   public:
      void *operator new(size_t,void *where) { return where ; }
      WordInfo(size_t tf = 0, size_t df = 0)
	 { term_freq = tf ; doc_freq = df ; disambig_symbols[0] = nullptr ; }
      ~WordInfo() ;

      // manipulators
      void initPositions(FrSymbol *word, size_t range) ;
      void incr(size_t tf) { if (tf) { term_freq += tf ; doc_freq++ ; } }
      void update(size_t tf, size_t df) { term_freq += tf ; doc_freq += df ; }

      // accessors
      int wordPosition(const FrSymbol *marker, size_t range) const ;
      FrSymbol *disambiguate(int position,size_t range) const
	 { FrSymbol * const *mid = &disambig_symbols[range] ;
	   return (FrSymbol*)&mid[position] ; }

      size_t termFrequency() const { return term_freq ; }
      size_t docFrequency() const { return doc_freq ; }
      double invDocFrequency(size_t total_docs) const ;

      double TF_IDF(size_t total_docs) const ;
   } ;

//----------------------------------------------------------------------

class WcTermVector : public FrTermVector
   {
   private:
      // no additional data members
   public:
      WcTermVector() : FrTermVector() {}
      WcTermVector(const FrSparseArray *counts) : FrTermVector(counts) {}
      WcTermVector(const FrSymCountHashTable *counts) : FrTermVector(counts) {}
      WcTermVector(const FrList *words) : FrTermVector(words) {}  // FrSymbol or (FrSymbol FrNumber)
      WcTermVector(const FrTermVector &orig) : FrTermVector(orig) {}
      WcTermVector(const FrArray *doc, size_t anchorpoint, size_t range) ;

      // manipulators
      void weightTerms(WcDecayType decay, double null_weight, size_t range,
		       const FrSymCountHashTable *freq_ht = nullptr,
		       size_t corpus_size = 0) ;

      // comparison
      double splitCosine(const FrTermVector *othervect, int position,
			 int range) const ;
   } ;

//----------------------------------------------------------------------

class WcWordPairTable : public FrHashTable<FrCons*,FrNullObject>
   {
   private:
      // no additional data members
   public:
      WcWordPairTable(size_t cap = 100000) : FrHashTable<FrCons*,FrNullObject>(cap) {}

      // manipulators
      bool addPair(const FrSymbol *word1, const FrSymbol *word2)
         {
	 FrCons *pair = new FrCons(word1,word2) ;
	 bool already_present = add(pair) ;
	 if (already_present)
	    {
	    free_object(pair) ;
	    }
	 return already_present ;
	 }

      // accessors
      bool contains(const FrSymbol *word1, const FrSymbol *word2) const
         {
	 FrCons pair(word1,word2) ;
	 return FrHashTable<FrCons*,FrNullObject>::contains(&pair) ;
	 }
   } ;

//----------------------------------------------------------------------

typedef bool WcVectorFilterFunc(const WcTermVector *tv, const FrSymCountHashTable *word_freq, void *user_data) ;
typedef FrList *WcClusterFilterFunc(FrList *clust, const FrSymCountHashTable *word_freq, void *user_data) ;
// rules for cluster filter func:
//    if return value is not initial value of 'clust', it must free any elements that were removed
//    if return value is 0, it must free entirety of 'clust'
typedef FrList *WcClusterPostprocFunc(FrList *clusters, void *user_data) ;

typedef double WcMIScoreFunc(const FrSymbol *word1, size_t freq1,
			     const FrSymbol *word2, size_t freq2,
			     size_t cooccur, size_t corpus_size,
			     void *udata) ;

class WcParameters
   {
   private:
      FrSymHashTable  *m_desired_words ;
      FrObjHashTable  *m_equiv_classes ;
      WcWordPairTable *m_mutualinfo ;
      size_t           m_min_wordfreq ;
      size_t           m_max_wordfreq ;
      size_t           m_max_terms ;
      size_t           m_stop_terms ;
      size_t           m_desired_clusters ;
      size_t           m_backoff_step ;
      int              m_mono_skip ;
      unsigned         m_max_equiv_length ;
      size_t           m_phrase_length ;
      double           MI_threshold ;
      FrThreadPool    *m_tpool ;
      WcMIScoreFunc   *m_mi_score_func ;
      FrTermVectorSimilarityFunc *m_tvsim_func ;
      FrClusteringSimilarityFunc *m_clustersim_func ;
      WcVectorFilterFunc *m_prefilter_func ;
      WcVectorFilterFunc *m_postfilter_func ;
      WcClusterFilterFunc *m_clusterfilter_func ;
      WcClusterPostprocFunc *m_clusterpostproc_func ;
      void    *m_mi_score_data ;
      void    *m_tvsim_data ;
      void    *m_clustersim_data ;
      void    *m_prefilter_data ;
      void    *m_postfilter_data ;
      void    *m_clusterfilter_data ;
      void    *m_clusterpostproc_data ;
      const char *m_equiv_class_file ;
      size_t   m_num_threads ;
      bool     m_use_chi_squared ;
      bool     m_downcase_source ;
      bool     m_downcase_target ;
      bool     m_hard_cluster_limit ;
      bool     m_ignore_beyond_cluster_limit ;
      bool     m_ignore_unseen_seeds ;
      bool     m_suppress_auto_brackets ;
   public:
      size_t   src_context_left ;
      size_t   src_context_right ;
      size_t   trg_context_left ;
      size_t   trg_context_right ;
      EbAlignConstraints *constraints ;
      bool     m_skip_auto_clusters ;
      bool     m_monolingual ;
      bool     m_all_lengths ;
      bool     m_auto_numbers ;
      bool     m_have_markup ;

   protected:
      void init()
         { m_min_wordfreq = m_stop_terms = 0 ;
	   m_max_wordfreq = m_max_terms = UINT_MAX ;
	   src_context_left = src_context_right = 2 ;
	   trg_context_left = trg_context_right = 0 ;
	   m_phrase_length = 1 ; MI_threshold = 0.0 ;
	   constraints = nullptr ; m_skip_auto_clusters = false ;
	   m_monolingual = false ; m_mono_skip = 0 ;
	   m_all_lengths = true ; m_auto_numbers = true ;
	   m_have_markup = true ; m_max_equiv_length = 100 ;
	   m_use_chi_squared = false ; m_desired_words = 0 ;
	   m_mi_score_func = nullptr ; m_mi_score_data = nullptr ;
	   m_tvsim_func = nullptr ; m_tvsim_data = nullptr ;
	   m_clustersim_func = nullptr ; m_clustersim_data = nullptr ;
	   m_prefilter_func = nullptr ; m_prefilter_data = nullptr ;
	   m_postfilter_func = nullptr ; m_postfilter_data = nullptr ;
	   m_clusterfilter_func = nullptr ; m_clusterfilter_data = nullptr ;
	   m_clusterpostproc_func = nullptr ; m_clusterpostproc_data = nullptr ;
	   m_downcase_source = false ; m_downcase_target = false ;
	   m_hard_cluster_limit = true ; m_ignore_beyond_cluster_limit = false ;
	   m_ignore_unseen_seeds = false ; m_suppress_auto_brackets = false ;
	   m_equiv_classes = nullptr ; m_equiv_class_file = nullptr ;
	   m_mutualinfo = 0 ; m_desired_clusters = 100 ;
	   m_backoff_step = 1 ;
	   m_num_threads = 0 ;
	 }

   public: // methods
      WcParameters() { init() ; }
      WcParameters(const WcParameters &params)
	 { memcpy(this,&params,sizeof(WcParameters)) ; }
      WcParameters(const WcParameters *params)
	 { memcpy(this,params,sizeof(WcParameters)) ; }
      WcParameters(size_t min_f, size_t max_f, size_t max_t, size_t stop_t,
		   size_t s_left, size_t s_right,
		   size_t t_left, size_t t_right, size_t phr, double MI,
		   EbAlignConstraints *constr, bool skip = false, bool mono = false,
		   bool all_lengths = true, bool auto_numbers = true)
	 { init() ;
	   m_min_wordfreq = min_f ; m_max_wordfreq = max_f ;
	   m_max_terms = max_t ; m_stop_terms = stop_t ;
	   src_context_left = s_left ; src_context_right = s_right ;
	   trg_context_left = t_left ; trg_context_right = t_right ;
	   m_phrase_length = phr ; MI_threshold = MI ;
	   constraints = constr ; m_skip_auto_clusters = skip ;
	   m_monolingual = mono ; m_all_lengths = all_lengths ;
	   m_auto_numbers = auto_numbers ;
	 }

      // accessors
      size_t numThreads() const { return m_num_threads ; }
      size_t minWordFreq() const { return m_min_wordfreq ; }
      size_t maxWordFreq() const { return m_max_wordfreq ; }
      size_t maxTermCount() const { return m_max_terms ; }
      size_t stopTermCount() const { return m_stop_terms ; }
      size_t phraseLength() const { return  m_phrase_length ; }
      size_t desiredClusters() const { return m_desired_clusters ; }
      size_t backoffStep() const { return m_backoff_step ; }
      double miThreshold() const { return MI_threshold ; }
      int monoSkip() const { return m_mono_skip ; }
      unsigned maxEquivLength() const { return m_max_equiv_length ; }
      bool skipAutoClusters() const { return m_skip_auto_clusters ; }
      bool monolingualOnly() const { return m_monolingual ; }
      bool allLengths() const { return m_all_lengths ; }
      bool autoNumbers() const { return m_auto_numbers ; }
      bool haveMarkup() const { return  m_have_markup ; }
      bool chiSquaredMI() const { return m_use_chi_squared ; }
      bool downcaseSource() const { return m_downcase_source ; }
      bool downcaseTarget() const { return m_downcase_target ; }
      bool hardClusterLimit() const { return m_hard_cluster_limit ; }
      bool ignoreBeyondClusterLimit() const { return m_ignore_beyond_cluster_limit ; }
      bool ignoreUnseenSeeds() const { return m_ignore_unseen_seeds ; }
      bool suppressAutoBrackets() const { return m_suppress_auto_brackets ; }
      WcMIScoreFunc *miScoreFunc() const { return m_mi_score_func ; }
      void *miScoreData() const { return m_mi_score_data ; }
      FrTermVectorSimilarityFunc *tvSimFunc() const { return m_tvsim_func ; }
      void *tvSimData() const { return m_tvsim_data ; }
      FrClusteringSimilarityFunc *clusterSimFunc() const { return m_clustersim_func ; }
      void *clusterSimData() const { return m_clustersim_data ; }
      WcVectorFilterFunc *preFilterFunc() const { return m_prefilter_func ; }
      void *preFilterData() const { return m_prefilter_data ; }
      WcVectorFilterFunc *postFilterFunc() const { return m_postfilter_func ; }
      void *postFilterData() const { return m_postfilter_data ; }
      WcClusterFilterFunc *clusterFilterFunc() const { return m_clusterfilter_func ; }
      void *clusterFilterData() const { return m_clusterfilter_data ; }
      WcClusterPostprocFunc *clusterPostprocFunc() const { return m_clusterpostproc_func ; }
      void *clusterPostprocData() const { return m_clusterpostproc_data ; }
      const FrSymHashTable *desiredWords() const { return m_desired_words ; }
      FrObjHashTable *equivalenceClasses() const { return m_equiv_classes ; }
      const char *equivClassFile() const { return m_equiv_class_file ; }
      WcWordPairTable *mutualInfo() const { return m_mutualinfo ; }
      FrThreadPool *threadPool() const { return m_tpool ; }

      // modifiers
      void numThreads(size_t thr) { m_num_threads = thr ; }
      void desiredClusters(size_t cl) { m_desired_clusters = cl ; }
      void backoffStep(size_t bo) { m_backoff_step = bo ; }
      void minWordFreq(size_t freq)
	 { if (freq > m_min_wordfreq) m_min_wordfreq = freq ; }
      void skipAutoClusters(bool skip)
         { m_skip_auto_clusters = skip ; }
      void monolingualOnly(bool mono) { m_monolingual = mono ; }
      void monoSkip(int skip) { m_mono_skip = skip ; }
      void autoNumber(bool an) { m_auto_numbers = an ; }
      void haveMarkup(bool markup) { m_have_markup = markup ; }
      void chiSquaredMI(bool chi) { m_use_chi_squared = chi ; }
      void downcaseSource(bool dc) { m_downcase_source = dc ; }
      void downcaseTarget(bool dc) { m_downcase_target = dc ; }
      void hardClusterLimit(bool hard) { m_hard_cluster_limit = hard ; }
      void ignoreBeyondClusterLimit(bool ignore) { m_ignore_beyond_cluster_limit = ignore ; }
      void ignoreUnseenSeeds(bool ignore) { m_ignore_unseen_seeds = ignore ; }
      void suppressAutoBrackets(bool suppress) { m_suppress_auto_brackets = suppress ; }
      void maxEquivLength(unsigned len) { m_max_equiv_length = len ; }
      void desiredWords(FrSymHashTable *dw) { m_desired_words = dw ; }
      void equivalenceClasses(FrObjHashTable *eq) { m_equiv_classes = eq ; }
      void equivClassFile(const char *eq) { m_equiv_class_file = eq ; }
      void mutualInfo(WcWordPairTable *mi) { m_mutualinfo = mi ; }
      void miScoreFunc(WcMIScoreFunc *fn, void *udata) { m_mi_score_func = fn ; m_mi_score_data = udata ; }
      void tvSimFunc(FrTermVectorSimilarityFunc *fn, void *udata = nullptr) { m_tvsim_func = fn ; m_tvsim_data = udata ; }
      void clusterSimFunc(FrClusteringSimilarityFunc *fn, void *data) { m_clustersim_func = fn ; m_clustersim_data = data ; }
      void preFilterFunc(WcVectorFilterFunc *fn, void *udata)
         { m_prefilter_func = fn ; m_prefilter_data = udata ; }
      void postFilterFunc(WcVectorFilterFunc *fn, void *udata)
         { m_postfilter_func = fn ; m_postfilter_data = udata ; }
      void clusterFilterFunc(WcClusterFilterFunc *fn, void *udata)
         { m_clusterfilter_func = fn ; m_clusterfilter_data = udata ; }
      void clusterPostprocFunc(WcClusterPostprocFunc *fn, void *udata)
	 { m_clusterpostproc_func = fn ; m_clusterpostproc_data = udata ; }	 
      void threadPool(FrThreadPool *tpool) { m_tpool = tpool ; }
   } ;

//----------------------------------------------------------------------

// FrSparseArray is currently not thread-safe, so use hash table instead
#if defined(FrMULTITHREAD) || defined(TRADE_MEM_FOR_SPEED)
class WcSparseArray :  public FrSymCountHashTable
#else
class WcSparseArray :  public FrSparseArray
#endif
   {
   private:
      static FrAllocator allocator ;
      size_t	m_frequency ;
   public:
      void *operator new(size_t) { return allocator.allocate() ; }
      void operator delete(void *blk) { allocator.release(blk) ; }
#if defined(FrMULTITHREAD) || defined(TRADE_MEM_FOR_SPEED)
      WcSparseArray() : FrSymCountHashTable(0) { m_frequency = 0 ; }
      WcSparseArray(size_t max) : FrSymCountHashTable(max) { m_frequency = 0 ; }
#else
      WcSparseArray() : FrSparseArray() { m_frequency = 0 ; nonObjectArray(true) ; }
      WcSparseArray(size_t max) : FrSparseArray(max) { m_frequency = 0 ; nonObjectArray(true) ; }
#endif

      // accessors
      size_t frequency() const { return m_frequency ; }

      // modifiers
      void setFrequency(size_t f) { m_frequency = f ; }
      void incrFrequency(size_t inc) { m_frequency += inc ; }
      void incrFrequency() { m_frequency++ ; }
#if defined(FrMULTITHREAD) || defined(TRADE_MEM_FOR_SPEED)
      void incrCount(FrSymbol *word, size_t freq)
         { addCount(word,freq) ;  }
#else
      void incrCount(FrSymbol *word, size_t freq)
         { addItem((uintptr_t)word, (FrObject*)freq, FrArrAdd_INCREMENT) ; }
#endif
   } ;

/************************************************************************/
/************************************************************************/

extern bool use_nearest ;
extern bool use_RMS_cosine ;
extern bool sum_cluster_sizes ;
extern bool use_multiple_passes ;

//----------------------------------------------------------------------

// configuration
void apply_WordClus_configuration(WcConfig *config) ;
void WcLoadTermWeights(const char *weights_file) ;
void WcSetWordDelimiters(const char *delim) ;
const char *WcWordDelimiters() ;

// file access
FILE *WcOpenCorpus(const char *filename) ;
void WcCloseCorpus(FILE *fp) ;
FrList *WcLoadFileList(bool use_stdin, const char *listfile) ;

// utility
FrSymbol *WcConcatWords(FrSymbol *word1, FrSymbol *word2, char sep = WC_CONCAT_WORDS_SEP) ;
FrSymbol *WcConcatWords(const FrArray &words, size_t start, size_t len, char sep = WC_CONCAT_WORDS_SEP) ;
FrSymbol *WcConcatPhrases(const FrList *words1, const FrList *words2) ;

// preprocessing
WcWordPairTable *WcComputeMutualInfo(const FrList *filelist,
				     const WcParameters *params,
				     const FrSymCountHashTable *uniwordfreq,
				     size_t corpus_size) ;

void WcAnalyzeContextMono(const FrList *sent, FrCharEncoding encoding,
			  size_t range_l, size_t range_r, size_t phrase_size,
			  double min_phrase_MI, FrSymHashTable *ht,
			  const FrObjHashTable *equivs, const FrSymHashTable *desired_words,
			  unsigned max_equiv_length, WcWordPairTable *mutualinfo = nullptr,
			  size_t freq = 1, bool auto_numbers = true,
			  bool have_markup = true) ;
// the above function is just a combination of the following two
bool WcPrepContextMono(const FrList *sent, FrCharEncoding encoding,
		       FrArray *&source_array, FrArray *&tokenized_array,
		       unsigned *&coverage, unsigned &max_coverage,
		       size_t phrase_size, const FrObjHashTable *equivs,
		       bool auto_numbers, bool have_markup) ;
void WcAnalyzeContextMono(const FrArray &source, const FrArray &tokenized,
			  const unsigned *coverage, unsigned max_coverage,
			  size_t range_l, size_t range_r, size_t phrase_size,
			  double min_phrase_MI, FrSymHashTable *ht,
			  const FrSymHashTable *desired_words,
			  const WcWordPairTable *mutualinfo, size_t frequency) ;

void WcConvertCounts2Vectors(FrSymHashTable *key_words,
			     FrSymCountHashTable *uniwordfreq = nullptr) ;
void WcRemoveAutoClustersFromSeeds(FrObjHashTable *seeds) ;
void WcAdjustContextWeights(FrSymHashTable *ht,size_t range_l, size_t range_r,
			    const FrSymCountHashTable *uniwordfreq = nullptr,
			    size_t corpus_size = 0);

// the actual clustering
FrList *cluster_vectors(FrSymHashTable *ht, const WcParameters *params,
			FrThresholdList *thresholds,
			FrObjHashTable *seeds = nullptr,
			bool verbose = false,
			bool exclude_singletons = true) ;

// top-level processing functions
void WcProcessCorpusFiles(const char *listfile, bool use_stdin,
			  FILE *outfp,FILE *tokfp,FILE *tagfp,
			  const WcParameters *params,
			  FrThresholdList *thresholds,
			  const char *outfilename = nullptr,
			  const char *tokfilename = nullptr,
			  const char *tagfilename = nullptr) ;

// output of results
void WcOutputClusters(const FrList *cluster_list, FILE *outfp,
		      const char *seed_file, bool monoling = false,
		      bool sort_output = true,
		      const char *output_filename = nullptr,
		      bool skip_auto_clusters = false) ;
void WcOutputTokenFile(const FrList *cluster_list, FILE *tokfp,
		       bool monoling = false, bool sort_output = true,
		       const char *output_filename = nullptr,
		       bool skip_auto_clusters = false,
		       bool suppress_auto_brackets = false) ;
void WcOutputTaggedCorpus(const FrList *cluster_list, FILE *tagfp,
			  bool monoling = false, bool sort_output = true,
			  const char *output_filename = nullptr,
			  bool skip_auto_clusters = false) ;

// cleanup
void WcClearTermVectors(FrSymHashTable *) ;
void WcClearCounts(FrSymHashTable *) ;
void WcClearWordDelimiters() ;
void WcClearDisambigData() ;

//----------------------------------------------------------------------

bool is_number16(const char *word) ;


inline bool is_number(const char *word)
{
//   if (*word == Fr_highbyte(FrQU_ASCII))
//      return is_number16(word) ;
   if (*word == '+' || *word == '-')
      word++ ;				// skip leading sign if present
   if (Fr_isdigit(*word))		// must have at least one digit
      {
      word++ ;
      while (*word)
         {
         if (!Fr_isdigit(*word) && *word != '.' && *word != ',')
	    return false ;
         else
	    word++ ;
         }
      return true ;
      }
   else
      return false ;
}

inline bool is_number(const FrSymbol *word)
{ return is_number(word->symbolName()) ; }

inline bool is_number(const FrString *word)
{ return is_number((const char *)word->stringValue()) ; }

inline bool is_compound_number(const char *word)
{
//   if (*word == Fr_highbyte(FrQU_ASCII))
//      return is_number16(word) ;
   if (*word == '+' || *word == '-')
      word++ ;				// skip leading sign if present
   if (Fr_isdigit(*word))		// must have at least one digit
      {
      word++ ;
      while (*word)
         {
         if (!Fr_isdigit(*word) && *word != '.' && *word != ',' &&
	     *word != WC_CONCAT_WORDS_SEP)
	    return false ;
         else
	    word++ ;
         }
      return true ;
      }
   else
      return false ;
}

inline bool is_compound_number(const FrSymbol *word)
{ return is_compound_number(word->symbolName()) ; }

inline bool is_compound_number(const FrString *word)
{ return is_compound_number((const char *)word->stringValue()) ; }

FrCharEncoding WcCurrentCharEncoding() ;
void WcSetCharEncoding(const char *char_enc) ;

#endif /* !__WORDCLUS_H_INCLUDED */

// end of file wordclus.h //

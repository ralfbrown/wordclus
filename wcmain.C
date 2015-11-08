/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcmain.cpp	      word clustering (main program)		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2005,2006,2008,2009,2010,2015	*/
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

#include <stdio.h>
#include <stdlib.h>
#include "wordclus.h"
#include "wcbatch.h"
#include "wcglobal.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#ifndef EXIT_SUCCESS
#  define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#  define EXIT_FAILURE 1
#endif

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

static size_t num_examples = 0 ;

#ifndef NDEBUG
#  undef _FrCURRENT_FILE
   static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* !NDEBUG */

/************************************************************************/
/*	External Functions						*/
/************************************************************************/

FrList *EbExtractAlignTag(const char *tags) ;
FrList *EbStripMorph(FrList *words, FrCharEncoding enc) ;
char *WcStripCoindex(const char *word, FrCharEncoding encoding) ;

/************************************************************************/
/************************************************************************/

// avoid pulling in unneeded parts of the EBMT machinery by defining some
//   dummy functions
int perform_EBMT_operation(FrObject *, ostream &, istream &)
{
   return false ;
}

/************************************************************************/
/************************************************************************/


static void WcShowProgress(size_t &count, size_t incr, size_t interval, const char *line_prefix,
			   bool run_verbosely = true)
{
   if (!run_verbosely)
      return ;
   size_t prev_intervals = count / interval ;
   count += incr ;
   size_t curr_intervals = count / interval ;
   if (curr_intervals > prev_intervals)
      {
      if (curr_intervals % 50 == 0)
	 {
	 if (showmem)
	    {
	    FrMemoryStats() ;
	    }
	 cout << endl << line_prefix << (char)('0' + ((curr_intervals / 50) % 10)) << flush  ;
	 }
      cout << "." << flush ;
      }
   return ;
}

//----------------------------------------------------------------------

static bool clear_disambiguation(const FrSymbol *sym, FrNullObject, va_list)
{
   FrSymbol *symbol = const_cast<FrSymbol*>(sym) ;
   FrFree((void*)symbol->symbolFrame()) ;
   symbol->setFrame(0) ;
   return true ;
}

//----------------------------------------------------------------------

void WcClearDisambigData()
{
   FrSymbolTable *symtab = FrSymbolTable::current() ;
   if (symtab)
      symtab->iterate(clear_disambiguation) ;
   return ;
}

//----------------------------------------------------------------------

static bool is_punctuation(const char *word)
{
   if (Fr_ispunct(word[0]) && word[1] == '\0')
      return true ;
   return false ;
}

//----------------------------------------------------------------------

static FrList *remove_punctuation(FrList *wordlist)
{
   FrList *result ;
   FrList **end = &result ;
   while (wordlist)
      {
      FrObject *word = poplist(wordlist) ;
      if (word && is_punctuation(word->printableName()))
	 free_object(word) ;
      else
	 result->pushlistend(word,end) ;
      }
   *end = nullptr ;			// terminate the list
   return result ;
}

//----------------------------------------------------------------------

static bool analyze_context_mono_batch(const WcLineBatch &lines, const WcParameters *params,
				       va_list args)

{
   FrVarArg(FrSymHashTable *,ht) ;
   FrVarArg2(bool,int,run_verbosely) ;
   const FrSymHashTable *desired_words = params->desiredWords() ;
   FrCharEncoding enc = params->downcaseSource() ? char_encoding : FrChEnc_RawOctets ;
   const FrObjHashTable *equivs = params->equivalenceClasses() ;
   WcWordPairTable *mutualinfo = params->mutualInfo() ;
   for (size_t i = 0 ; i < lines.numLines() ; ++i)
      {
      const char *line = lines.line(i) ;
      FrList *swords = FrCvtSentence2Wordlist(line) ;
      if (exclude_punctuation)
	 swords = remove_punctuation(swords) ;
      size_t minphrase = params->allLengths() ? 1 : params->phraseLength() ;
      FrArray *source_array ;
      FrArray *tokenized_array ;
      unsigned *coverage ;
      unsigned max_coverage = params->maxEquivLength() ;
      if (WcPrepContextMono(swords,enc,source_array,tokenized_array,coverage,max_coverage,
			    minphrase,equivs,params->autoNumbers(),params->haveMarkup()))
	 {
	 for (size_t phrlen = minphrase ; phrlen <= params->phraseLength() ; phrlen++)
	    {
	    WcAnalyzeContextMono(*source_array,*tokenized_array,coverage,max_coverage,
				 params->src_context_left, params->src_context_right,
				 phrlen,params->miThreshold(),ht,desired_words,mutualinfo,1) ;
	    }
	 delete source_array ;
	 delete tokenized_array ;
	 FrFree(coverage) ;
	 }
      }
   WcShowProgress(num_examples,lines.numLines(),2000,";   ",run_verbosely) ;
   return true ;
}

//----------------------------------------------------------------------

static void analyze_context(const char *filename, const WcParameters *params,
			    const char *delim, FrSymHashTable *ht)
{
   assertq(ht != nullptr) ;
   FrElapsedTimer etimer ;
   FrTimer timer ;
   FILE *fp = WcOpenCorpus(filename) ;
   if (!fp)
      return ;
   if (verbose)
      cout << ";   " << flush ;
   if (!delim)
      delim = FrStdWordDelimiters(char_encoding) ;
   WcProcessFile(fp,params,&analyze_context_mono_batch,ht,verbose) ;
   WcCloseCorpus(fp) ;
   if (verbose)
      {
      cout << "\n;  [ Processed " << filename << " in " << etimer.read100ths() << "s, "
	   << timer.readsec() << " CPU seconds ]" << endl ;
      }
   return ;
}

/************************************************************************/
/************************************************************************/

void apply_WordClus_configuration(WcConfig *config)
{
   if (config)
      wc_vars.applyConfiguration(config) ;
   return ;
}

//----------------------------------------------------------------------

static void analyze_files(const FrList *filelist, const WcParameters *params,
			  FrSymHashTable *key_words, const char *delim)
{
   FrElapsedTimer etimer ;
   FrTimer timer ;
   for (const FrList *files = filelist ; files ; files = files->rest())
      {
      const char *filename = 
	 (char*)((FrString*)files->first())->stringValue() ;
      if (verbose)
	 cout << ";   processing " << filename << endl ;
      analyze_context(filename,params,delim,key_words) ;
      }
   cout << ";  Analyzing files took " << etimer.read100ths() << "s ("
	<< timer.readsec() << " CPU seconds)" << endl ;
   return ;
}

//----------------------------------------------------------------------

static bool create_empty_context_list(const FrSymbol *compound, FrObject *entry, va_list args)
{
   size_t count = (size_t)entry ;
   if (count >= min_frequency && count <= max_frequency &&
       (!exclude_numbers || !is_compound_number(compound)))
      {
      WcSparseArray *counts = new WcSparseArray ;
      FrVarArg(FrSymHashTable*,ht) ;
      ht->add(compound,counts) ;
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

FrList *WcLoadFileList(bool use_stdin, const char *listfile)
{
   FrList *filelist = nullptr ;
   if (use_stdin)
      pushlist(new FrString("-"),filelist) ;
   else
      {
      FILE *listfp = stdin ;
      if (listfile && *listfile)
	 {
	 listfp = fopen(listfile,"r") ;
	 if (!listfp)
	    {
	    cerr << "Unable to open " << listfile
		 << " to get list of corpus files!" << endl;
	    exit(EXIT_FAILURE) ;
	    }
	 }
      while (!feof(listfp))
	 {
	 char line[FrMAX_LINE] ;
	 if (fgets(line,sizeof(line),listfp))
	    {
	    char *filename = line ;
	    FrSkipWhitespace(filename) ;
	    char *end = strchr(filename,'\0') ;
	    while (end > filename && Fr_isspace(end[-1]))
	       end-- ;
	    *end = '\0' ;
	    if (*filename && *filename != '#' && *filename != ';')
	       pushlist(new FrString(filename),filelist) ;
	    }
	 }
      filelist = listreverse(filelist) ;
      fclose(listfp) ;
      }
   return filelist ;
}

//----------------------------------------------------------------------

static FrList *remove_unprintable(FrList *words)
{
   FrSymbol *null = FrSymbolTable::add("") ;
   FrList *result = nullptr ;
   FrList **end = &result ;
   while (words)
      {
      FrSymbol *word = (FrSymbol*)poplist(words) ;
      if (word != null)
	 result->pushlistend(word,end) ;
      }
   *end = nullptr ;			// terminate result list
   return result ;
}

//----------------------------------------------------------------------

static void update_cooccur(FrSymbol *word1, FrSymbol *word2, FrSymHashTable *cooccur_table)
{
   FrSymCountHashTable *cooccur = static_cast<FrSymCountHashTable*>(cooccur_table->lookup(word1)) ;
   if (!cooccur)
      {
      cooccur = new FrSymCountHashTable ;
      if (cooccur_table->add(word1,cooccur))
	 {
	 // someone else created the sub-table in parallel, so discard
	 //   the one we tried to add and use the other one
	 delete cooccur ;
	 cooccur = static_cast<FrSymCountHashTable*>(cooccur_table->lookup(word1)) ;
	 }
      }
   if (cooccur)
      {
      cooccur->addCount(word2,1) ;
      }
   return ;
}

//----------------------------------------------------------------------

static void collect_cooccur(const char *sent, FrSymHashTable *cooccur_table,
			    bool have_markup, const FrSymHashTable *desired_words)
{
   if (sent && *sent)
      {
      FrList *swords = FrCvtSentence2Wordlist(sent) ;
      if (exclude_punctuation)
	 swords = remove_punctuation(swords) ;
      FrCharEncoding enc = lowercase_source ? char_encoding : FrChEnc_RawOctets ;
      swords = remove_unprintable(swords) ;
      FrSymbol *prevword = nullptr ;
      while (swords)
	 {
	 FrObject *srcword = poplist(swords) ;
	 FrSymbol *word ;
	 if (have_markup)
	    {
	    char *sym = WcStripCoindex(FrPrintableName(srcword),enc) ;
	    word = FrSymbolTable::add(sym) ;
	    FrFree(sym) ;
	    }
	 else
	    {
	    word = FrSymbolTable::add(FrPrintableName(srcword)) ;
	    }
	 free_object(srcword) ;
	 if (desired_words && !desired_words->contains(word))
	    {
	    // if the current word is not frequent enough, don't
	    //   generate co-occurrence counts with it as either the
	    //   second word (this iteration) or first word (next
	    //   iteration)
	    word = nullptr ;
	    }
	 if (prevword && word)
	    {
	    update_cooccur(prevword,word,cooccur_table) ;
	    }
	 prevword = word ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

static bool collect_cooccur_counts_batch(const WcLineBatch &lines, const WcParameters *params,
					 va_list args)
{
   FrVarArg(FrSymHashTable*,cooccur_table) ;
   FrVarArg(const FrSymHashTable*,desired_words) ;
   FrVarArg2(bool,int,run_verbosely) ;
   bool have_markup = params->haveMarkup() ;
   for (size_t i = 0 ; i < lines.numLines() ; ++i)
      {
      collect_cooccur(lines.line(i),cooccur_table,have_markup,desired_words) ;
      }
   WcShowProgress(num_examples,lines.numLines(),2000,";   ",run_verbosely) ;
   return true ;
}

//----------------------------------------------------------------------

static bool by_chance(size_t freq1, size_t freq2, size_t cooccur,
		      size_t corpus_size)
{
   if (corpus_size >= 100 && freq1 > cooccur && freq2 > cooccur)
      {
      double expected = ((double)freq1 / corpus_size) * (double)freq2 ;
      size_t maxfreq = max(freq1,freq2) ;
      // for low-frequency terms, we can use the expected value as-is, but
      //   if the two terms are very frequent, we need to relax the
      //   by-chance check a little or we risk throwing out good
      //   translations
      double discount = 1.05 - (maxfreq/(double)corpus_size)/5.0 ;
      return (cooccur < discount * expected) ;
      }
   return false ;
}

//----------------------------------------------------------------------

static double score_mi_chisquared(const FrSymbol * /*word1*/, size_t freq1,
				  const FrSymbol * /*word2*/, size_t freq2,
				  size_t cooccur, size_t total, void *)
{
   if (cooccur < min_frequency)
      return -1.0 ;			// don't use this pair
   //  N=total, |X|=freq1, |Y|=freq2, C[X,Y]=cooccur
   //
   //         N( (N-C[X,Y])C[X,Y] - |X||Y| )^2
   //      -----------------------------------
   //       |Y| * (N - |Y|) * |X| * (N - |X|)
   //
   // we normalize the above into the range 0..1 by omitting the N in the
   //   numerator
   double chi = ((double)(total-cooccur)*(double)cooccur -
		 (double)freq1*(double)freq2) ;
   double denom = (double)freq1 * (double)(total-freq1) *
                  (double)freq2 * (double)(total-freq2) ;
   // return a value between 0 and 1, rather than between 0 and N as for the
   // true chi^2 definition
   return chi * chi / denom ;
}

//----------------------------------------------------------------------

static double score_mi_correlation(const FrSymbol * /*word1*/, size_t freq1,
				   const FrSymbol * /*word2*/, size_t freq2,
				   size_t cooccur, size_t /*total*/, void *)
{
   if (cooccur < min_frequency)
      return -1.0 ;			// don't use this pair
   size_t freq = (freq1 < freq2) ? freq1 : freq2 ;
   return cooccur / (double)freq ;
}

//----------------------------------------------------------------------

static bool filter_mi_pair(const FrSymbol *word2, size_t cooccur_count, va_list args)
{
   FrVarArg(const FrSymbol*,word1) ;
   FrVarArg(size_t,freq1) ;
   FrVarArg(size_t,corpus_size) ;
   FrVarArg(const FrSymCountHashTable*,uniwordfreq) ;
   FrVarArg(const WcParameters*,params) ;
   WcWordPairTable *mutualinfo = params->mutualInfo() ;
   double thresh = params->miThreshold() ;
   WcMIScoreFunc *fn = params->miScoreFunc() ;
   void *udata = params->miScoreData() ;
   size_t freq2 = uniwordfreq->lookup(word2) ;
   if (fn && fn(word1,freq1,word2,freq2,cooccur_count,corpus_size,udata) >= thresh &&
       !by_chance(freq1,freq2,cooccur_count,corpus_size))
      {
      mutualinfo->addPair(word1,word2) ;
      }
   return true ;
}

//----------------------------------------------------------------------

static bool filter_mi_pairs(const FrSymbol *word1, FrObject *cooccurrences,
			    va_list args)
{
   FrVarArg(const WcParameters*,params) ;
   FrVarArg(const FrSymCountHashTable*,uniwordfreq) ;
   FrVarArg(size_t,corpus_size) ;
   FrSymCountHashTable *subtable = static_cast<FrSymCountHashTable*>(cooccurrences) ;
   if (subtable)
      {
      size_t freq1 = uniwordfreq->lookup(word1) ;
      subtable->iterate(filter_mi_pair,word1,freq1,corpus_size,uniwordfreq,params) ;
      }
   return true ;
}

//----------------------------------------------------------------------

static bool free_subtable(const FrSymbol */*key*/, FrObject *val)
{
   free_object(val) ;
   return true ;
}

//----------------------------------------------------------------------

WcWordPairTable *WcComputeMutualInfo(const FrList *filelist,
				     const WcParameters *params,
				     const FrSymCountHashTable *uniwordfreq,
				     size_t corpus_size)
{
   FrElapsedTimer etimer ;
   FrTimer timer ;
   const FrSymHashTable *desired_words = params->desiredWords() ;
   FrSymHashTable *cooccur_table = new FrSymHashTable ;
   for (const FrList *files = filelist ; files ; files = files->rest())
      {
      const char *filename =
	 (char*)((FrString*)files->first())->stringValue() ;
      FILE *fp = WcOpenCorpus(filename) ;
      if (fp)
	 {
	 if (verbose)
	    {
	    cout << ";   processing " << filename << endl
		 << ";   " << flush ;
	    }
	 WcProcessFile(fp,params,&collect_cooccur_counts_batch,cooccur_table,desired_words,verbose) ;
	 WcCloseCorpus(fp) ;
	 if (verbose) cout << endl ; // terminate line of progress dots
	 }
      else
	 {
	 cout << ";!!   Error opening file '" << filename << "'" << endl  ;
	 }
      }
   WcWordPairTable *mutualinfo = new WcWordPairTable ;
   WcParameters local_params(params) ;
   local_params.mutualInfo(mutualinfo) ;
   if (!local_params.miScoreFunc())
      {
      local_params.miScoreFunc(params->chiSquaredMI()?score_mi_chisquared:score_mi_correlation,0) ;
      }
   cooccur_table->iterateAndClear(filter_mi_pairs,&local_params,uniwordfreq,corpus_size) ;
   local_params.mutualInfo(0) ;
   cooccur_table->onRemove(free_subtable) ;
   delete cooccur_table ;
   if (mutualinfo && mutualinfo->currentSize() > 0)
      {
      cout << ";  found " << mutualinfo->currentSize()
	   << " words with MI pairings in " << etimer.read100ths() << "s ("
	   << timer.readsec() << " CPU seconds)" << endl ;
      }
   else
      cout << ";  no mutual-information pairings found!" << endl ;
   return mutualinfo ;
}

//----------------------------------------------------------------------

inline bool is_stopword(const FrSymbol *word)
{
   return (word && word->inverseRelation() != nullptr) ;
}

//----------------------------------------------------------------------

static bool filter_by_freq(const FrSymbol *name, size_t freq, va_list args)
{
   FrVarArg(FrSymHashTable *,desired_words) ;
   FrVarArg(const FrObjHashTable *,seeds) ;
   if (freq >= min_frequency && !is_stopword(name))
      {
      desired_words->add(name) ;
      }
   else if (seeds && freq >= min_frequency) //FIXME?
      {
      FrString namestr(name->symbolName()) ;
      if (seeds->contains(&namestr))
	 {
	 desired_words->add(name) ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

static void count_frequencies(const char *sent, FrSymCountHashTable *freq, bool have_markup,
			      size_t *corpus_size)
{
   if (sent && *sent)
      {
      FrList *swords = FrCvtSentence2Wordlist(sent) ;
      if (exclude_punctuation)
	 swords = remove_punctuation(swords) ;
      FrCharEncoding enc = lowercase_source ? char_encoding : FrChEnc_RawOctets ;
      while (swords)
	 {
	 FrObject *srcword = poplist(swords) ;
	 FrSymbol *word ;
	 if (have_markup)
	    {
	    char *sym = WcStripCoindex(FrPrintableName(srcword),enc) ;
	    word = FrSymbolTable::add(sym) ;
	    FrFree(sym) ;
	    }
	 else
	    {
	    word = FrSymbolTable::add(FrPrintableName(srcword)) ;
	    }
	 freq->addCount(word,1) ;
	 (*corpus_size)++ ;
	 free_object(srcword) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

static bool count_frequencies_batch(const WcLineBatch &lines, const WcParameters *params,
				    va_list args)
{
   FrVarArg(FrSymCountHashTable *,freq) ;
   FrVarArg(size_t*,corpus_size) ;
   FrVarArg2(bool,int,run_verbosely) ;
   bool have_markup = params->haveMarkup() ;
   for (size_t i = 0 ; i < lines.numLines() ; ++i)
      {
      count_frequencies(lines.line(i),freq,have_markup,corpus_size) ;
      }
   WcShowProgress(num_examples,lines.numLines(),2000,";   ",run_verbosely) ;
   return true ;
}

//----------------------------------------------------------------------

static FrSymCountHashTable *count_frequencies(const FrList *filelist,
					      const WcParameters *params,
					      size_t &corpus_size,
					      FrSymHashTable *&desired_words)
{
   FrSymCountHashTable *uniwordfreq = new FrSymCountHashTable ;
   FrElapsedTimer etimer ;
   FrTimer timer ;
   desired_words = nullptr ;
   const FrObjHashTable *seeds = params->equivalenceClasses() ;
   for ( ; filelist ; filelist = filelist->rest())
      {
      const char *filename = (char*)((FrString*)filelist->first())->stringValue() ;
      FILE *fp = WcOpenCorpus(filename) ;
      if (fp)
	 {
	 if (verbose)
	    {
	    cout << ";   processing " << filename << endl
		 << ";   " << flush ;
	    }
	 WcProcessFile(fp,params,&count_frequencies_batch,uniwordfreq,&corpus_size,verbose) ;
	 WcCloseCorpus(fp) ;
	 if (verbose) cout << endl ; // terminate line of progress dots
	 }
      else
	 {
	 cout << ";!!   Error opening file '" << filename << "'" << endl ;
	 }
      }
   FrSymHashTable *desired = new FrSymHashTable ;
   if (desired)
      {
      desired_words = desired ;
      uniwordfreq->iterate(filter_by_freq,desired,seeds) ;
      cout << ";   found " << desired->currentSize() << " words above threshold" << endl ;
      }
   cout << ";   counting words took " << etimer.read100ths() << "s ("
	<< timer.readsec() << " CPU seconds)." << endl ;
   return uniwordfreq ;
}

//----------------------------------------------------------------------

static bool remove_filtered(const FrSymbol *term, FrObject *entry, va_list args)
{
   FrVarArg(const FrSymCountHashTable*,word_counts) ;
   FrVarArg(const WcParameters*,params) ;
   WcVectorFilterFunc *fn = params->preFilterFunc() ;
   WcTermVector *tv = (WcTermVector*)entry ;
   void *data = params->preFilterData() ;
   if (!fn(tv,word_counts,data))
      {
      delete tv ;
      FrVarArg(FrSymHashTable*,ht) ;
      ht->remove(term) ;
      }
   return true ;
}

//----------------------------------------------------------------------

static void WcPreFilterVectors(FrSymHashTable *key_words, const FrSymCountHashTable *word_counts,
			       const WcParameters &params)
{
   if (!key_words)
      return ;
   size_t before_count = key_words->currentSize() ;
   key_words->iterate(remove_filtered,word_counts,&params,key_words) ;
   size_t after_count = key_words->currentSize() ;
   if (after_count < before_count)
      {
      size_t disc = before_count - after_count ;
      cout << ";   discarded " << disc << " vectors." << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static void WcPostFilterClusters(FrList *clusters, const FrSymCountHashTable *word_counts,
				 const WcParameters &params)
{
   if (!clusters)
      return ;
   size_t discarded(0) ;
   WcVectorFilterFunc *fn = params.postFilterFunc() ;
   void *data = params.postFilterData() ;
   for ( ; clusters ; clusters = clusters->rest())
      {
      FrList *clust = (FrList*)clusters->first() ;
      // cluster format: (NAME tv1 tv2 ....)
      FrList *prev = clust ;
      FrList *next ;
      for (FrList *cl = clust->rest() ; cl ; prev = cl, cl = next)
	 {
	 next = cl->rest() ;
	 WcTermVector *tv = (WcTermVector*)cl->first() ;
	 if (!fn(tv,word_counts,data))
	    {
	    // chop the term vector out of the cluster by pointing its
	    //   predecessor at its successor
	    prev->replacd(next) ;
	    // and then delete it
	    cl->replacd(0) ;
	    cl->replaca(0) ;		// but not the term vector!
	    free_object(cl) ;
	    ++discarded ;
	    }
	 }
      }
   if (discarded > 0)
      {
      cout << ";   discarded " << discarded << " vectors." << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static FrList *WcFilterClusterMembers(FrList *clusters, const FrSymCountHashTable *word_counts,
				      const WcParameters &params)
{
   if (!clusters)
      return clusters ;
   WcClusterFilterFunc *fn = params.clusterFilterFunc() ;
   FrList *result = clusters ;
   if (fn)
      {
      size_t discarded(0) ;
      void *data = params.clusterFilterData() ;
      FrList *prev = nullptr ;
      for ( ; clusters ; clusters = clusters->rest())
	 {
	 FrList *clust = (FrList*)clusters->first() ;
	 size_t startsize(clust ? listlength(clust->rest()) : 0) ;
	 FrList *updated = fn(clust,word_counts,data) ;
	 if (updated != clust)
	    {
	    size_t endsize(updated ? listlength(updated->rest()) : 0) ;
	    if (endsize < startsize)
	       {
	       discarded += (startsize - endsize) ;
	       }
	    if (updated)
	       {
	       clusters->replaca(updated) ;
	       }
	    else if (prev)
	       {
	       prev->replacd(clusters->rest()) ;
	       }
	    else
	       {
	       result = clusters->rest() ;
	       }
	    }
	 }
      if (discarded > 0)
	 {
	 cout << ";   discarded " << discarded << " vectors." << endl ;
	 }
      }
   if (params.clusterPostprocFunc())
      {
      size_t clusters_before = listlength(result) ;
      result = params.clusterPostprocFunc()(result,params.clusterPostprocData()) ;
      size_t clusters_after = listlength(result) ;
      if (clusters_before > clusters_after)
	 {
	 cout << ";   eliminated/merged " << (clusters_before-clusters_after) << " clusters." << endl ;
	 }
      }
   return result ;
}

//----------------------------------------------------------------------

void WcProcessCorpusFiles(const char *listfile, bool use_stdin,
			  FILE *outfp, FILE *tokfp, FILE *tagfp,
			  const WcParameters *global_params,
			  FrThresholdList *thresholds,
			  const char *outfilename, const char *tokfilename,
			  const char *tagfilename)
{
   assertq(global_params != nullptr) ;
   FrList *filelist = WcLoadFileList(use_stdin,listfile) ;
   int passnum = 1 ;
   size_t corpus_size(0) ;
   WcParameters params(global_params) ;
   params.downcaseSource(lowercase_source) ;
   params.downcaseTarget(lowercase_target) ;
   params.threadPool(new FrThreadPool(params.numThreads())) ;
   bool monoling = params.monolingualOnly() ;
   cout << "; Pass " << passnum++ <<": count word frequencies" << endl ;
   FrSymHashTable *desired_words ;
   FrSymCountHashTable *uniwordfreq
      = count_frequencies(filelist,&params,corpus_size,desired_words) ;
   params.desiredWords(desired_words) ;
   WcWordPairTable *mutualinfo = nullptr ;
   if (params.phraseLength() > 1 && params.miThreshold() > 0.0)
      {
      cout << "; Pass " << passnum++ << ": compute pair-wise mutual information" << endl ;
      if (params.miThreshold() > 0.0)
	 {
	 num_examples = 0 ;
	 mutualinfo = WcComputeMutualInfo(filelist,&params,uniwordfreq,corpus_size) ;
	 }
      FramepaC_gc();
      }
   params.mutualInfo(mutualinfo) ;
   cout << "; Pass " << passnum++ << ": analyze local contexts" << endl ;
   FrSymHashTable *key_words = new FrSymHashTable ;
   if (two_pass_clustering)
      {
      cout << ";   Step A: find word-pair frequencies" << endl ;
      only_count_pairs = true ;
      num_examples = 0 ;
      analyze_files(filelist,&params,key_words,WcWordDelimiters()) ;
      FramepaC_gc();
      only_count_pairs = false ;
      cout << ";     (" << key_words->currentSize() << " word pairs found)"
	   << endl ;
      delete desired_words ;
      desired_words = new FrSymHashTable ;
      params.desiredWords(desired_words) ;
      key_words->iterate(create_empty_context_list,desired_words) ;
      cout << ";   Step B: analyze contexts for pairs occurring more than "
	   << min_frequency << " times (" << desired_words->currentSize() << ')'
	   << endl ;
      only_incr_contexts = true ;
      }
   FramepaC_gc();
   num_examples = 0 ;
   analyze_files(filelist,&params,key_words,WcWordDelimiters()) ;
   only_incr_contexts = false ;
   params.desiredWords(0) ;
   delete desired_words ;
   params.mutualInfo(0) ;
   delete mutualinfo ;
   free_object(filelist) ;
   cout << ";   " << key_words->currentSize() << " words found" << endl ;
   FramepaC_gc() ;
   WcConvertCounts2Vectors(key_words,uniwordfreq) ;
   if (ignore_auto_clusters)
      WcRemoveAutoClustersFromSeeds(params.equivalenceClasses()) ;
   if (use_seeds_for_context_only)
      params.equivalenceClasses(0) ;
   FramepaC_gc() ;
   if (verbose && showmem)
      FrMemoryStats() ;
   if (params.preFilterFunc())
      {
      FrTimer timer ;
      cout << "; Pass " << passnum++ << ": pre-filter term vectors" << endl ;
      WcPreFilterVectors(key_words,uniwordfreq,params) ;
      if (verbose)
	 {
	 cout << ";   filtering vectors took " << timer.readsec() << " CPU seconds." << endl ;
	 }
      }
   cout << "; Pass " << passnum++ << ": cluster local contexts" << endl ;
   WcAdjustContextWeights(key_words,params.src_context_left,
			  params.src_context_right,uniwordfreq,corpus_size) ;
   FrList *clusters = cluster_vectors(key_words,&params,thresholds,
				      params.equivalenceClasses(),
				      verbose,!keep_singletons) ;
   cout << ";   " << clusters->listlength() << " clusters found" << endl ;
   if (params.postFilterFunc())
      {
      FrTimer timer ;
      cout << "; Pass " << passnum++ << ": post-filter term vectors" << endl ;
      WcPostFilterClusters(clusters,uniwordfreq,params) ;
      if (verbose)
	 {
	 cout << ";   filtering vectors took " << timer.readsec() << " CPU seconds." << endl ;
	 }
      }
   if (params.clusterFilterFunc() || params.clusterPostprocFunc())
      {
      FrTimer timer ;
      cout << "; Pass " << passnum++ << ": post-filter clusters" << endl ;
      clusters = WcFilterClusterMembers(clusters,uniwordfreq,params) ;
      if (verbose)
	 {
	 cout << ";   filtering clusters took " << timer.readsec() << " CPU seconds." << endl ;
	 }
      }
   cout << "; Pass " << passnum++ << ": output equivalence classes" << endl ;
   if (clusters)
      {
      const char *seedfile = params.equivClassFile() ;
      WcOutputClusters(clusters,outfp,seedfile,monoling,WcSORT_OUTPUT,outfilename,params.skipAutoClusters()) ;
      WcOutputTokenFile(clusters,tokfp,monoling,WcSORT_OUTPUT,tokfilename,params.skipAutoClusters(),
			params.suppressAutoBrackets()) ;
      WcOutputTaggedCorpus(clusters,tagfp,monoling,WcSORT_OUTPUT,tagfilename,params.skipAutoClusters()) ;
      }
   // finally, clean up
   delete params.threadPool() ;
   params.threadPool(0) ;
   WcClearDisambigData() ;
   FrEraseClusterList(clusters) ;
   delete uniwordfreq ;
   WcClearCounts(key_words) ;
   delete key_words ;
   if (verbose && showmem)
      {
      FramepaC_gc() ;
      FrMemoryStats() ;
      }
   return ;
}

//----------------------------------------------------------------------

// end of file wcmain.cpp //

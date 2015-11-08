/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcanalyz.cpp	      preprocessing from sentences to contexts	*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2005,2006,2008,2009,2015	*/
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
#include "wctoken.h"
#include "wordclus.h"
#include "wcglobal.h"

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

FrAllocator WcSparseArray::allocator("WcSparseArray",sizeof(WcSparseArray)) ;

unsigned char *number_charmap = nullptr ;

static FrSymbol *symNUMBER = nullptr ;

/************************************************************************/
/*	External Functions						*/
/************************************************************************/

const FrSymbol *map_number(const FrSymbol *number,
			   const unsigned char *number_charmap) ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

FrSymbol *disambiguate(FrSymbol *word, int position, size_t range)
{
//   if (!word)
//      word = FrSymbolTable::add("") ;
   WordInfo *disamb = (WordInfo*)word->symbolFrame() ;
   if (!disamb)
      {
      disamb = (WordInfo*)FrMalloc(sizeof(WordInfo)+(2*range+1)*sizeof(FrSymbol*));
      if (disamb)
	 {
	 (void)new (disamb)WordInfo ;
	 disamb->initPositions(word,range) ;
	 word->setFrame((FrFrame*)disamb) ;
	 }
      else
	 {
	 FrNoMemory("disambiguating word by position") ;
	 return word ;
	 }
      }
   return disamb->disambiguate(position,range) ;
}

//----------------------------------------------------------------------

inline bool is_stopword(const FrSymbol *word)
{
   return (word && word->inverseRelation() != 0) ;
}

//----------------------------------------------------------------------

static void add_context(WcSparseArray *counts,FrSymbol *lcontext,
			FrSymbol *rcontext, size_t frequency = 1)
{
   if (single_phrase_context)
      {
      FrSymbol *context = WcConcatWords(lcontext,rcontext) ;
      FrSymbol *disambig = disambiguate(context,-1,1) ;
      counts->incrCount(disambig,frequency) ;
      }
   else
      {
      if (lcontext)
	 {
	 FrSymbol *disamb = disambiguate(lcontext,-1,1) ;
	 counts->incrCount(disamb,frequency) ;
	 }
      if (rcontext)
	 {
	 FrSymbol *disamb = disambiguate(rcontext,+1,1) ;
	 counts->incrCount(disamb,frequency) ;
	 }
      }
   return ;
}

/************************************************************************/
/*	context-analysis functions					*/
/************************************************************************/

static bool mutual_info_sufficient(const FrArray &words,size_t start, size_t len,
				   const WcWordPairTable *mutualinfo)
{
   if (mutualinfo)
      {
#define STRICT
#ifdef STRICT
      for (size_t i = 1 ; i < len ; i++)
	 {
	 if (!mutualinfo->contains((FrSymbol*)words[start+i-1],
				   (FrSymbol*)words[start+i]))
	    {
	    return false ;
	    }
	 }
#else // LENIENT
      for (size_t i = 1 ; i < len ; i++)
	 {
	 if (mutualinfo->contains((FrSymbol*)words[start+i-1],
				  (FrSymbol*)words[start+i]))
	    return true ;
	 }
      return false ;
#endif /* STRICT/LENIENT */
      }
   return true ;
}

//----------------------------------------------------------------------

static bool frequency_sufficient(const FrArray &words,size_t start, size_t len,
				 const FrSymHashTable *desired_words)
{
   if (desired_words)
      {
      for (size_t i = start ; i < start+len ; i++)
	 {
	 FrSymbol *word = (FrSymbol*)words[i] ;
	 if (!desired_words->contains(word))
	    {
	    return false ;
	    }
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

#if 0
static void count_word(FrSymbol *word, FrSymHashTable *ht,
		       size_t frequency = 1)
{
   if (word && ht)
      {
      ht->addCount(word,frequency) ;
      }
   return ;
}
#endif

//----------------------------------------------------------------------

static bool tokenized_word(FrString *context, FrSymbol *&token,
			   const FrObjHashTable *equivs, bool auto_numbers)
{
   if (equivs)
      {
      FrObject *found_obj ;
      if (equivs->lookup(context,&found_obj))
	 {
	 token = (FrSymbol*)found_obj ;
	 return true ;
	 }
      }
   if (auto_numbers && is_number(context))
      {
      token = symNUMBER ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

static void add_phrasal_context(int srclen, int i, const FrArray &source, const FrArray &tokenized,
				const unsigned *coverage, unsigned max_coverage,
				FrSymbol *null, WcSparseArray *counts,
				size_t range_l, size_t range_r, size_t phrase_size,
				size_t frequency)
{
   FrSymbol *lcontext = nullptr ;
   if (range_l > 0)
      {
      // accumulate the left context: for each word of context, find the
      //   token with the greatest coverage that exactly reaches up to the
      //   following one
      int offset(0) ;
      FrArray cphrase(range_l) ;
      for (size_t j = 1 ; j <= range_l ; j++)
	 {
	 FrSymbol *context ;
	 int match(0) ;
	 if (i <= -offset)
	    {
	    // we've run off the start of the input, so the context is null
	    context = null ;
	    }
	 else
	    {
	    unsigned max = ((unsigned)i+offset < max_coverage) ? i+offset : max_coverage ;
	    for (unsigned o = 1 ; o <= max ; o++)
	       {
	       if (coverage[i+offset-o] == o)
		  {
		  match = o ;
		  }
	       }
	    if (match > 0)
	       context = (FrSymbol*)tokenized[i+offset-match] ;
	    else
	       {
	       match = 1 ;
	       context = (FrSymbol*)source[i+offset-1] ;
	       }
	    }
	 cphrase[range_l-j] = context ;
	 if (i <= -offset)
	    break ;		// don't put in multiple nulls
	 offset -= match ;
	 }
      lcontext = WcConcatWords(cphrase,0,range_l,' ') ;
      }
   FrSymbol *rcontext = nullptr ;
   if (range_r > 0)
      {
      // accumulate the right context: for each word of tokenized context,
      //   skip ahead by the number of source words it covers
      int offset = i + phrase_size ;
      FrArray cphrase(range_r) ;
      for (size_t j = 0 ; j < range_r ; j++)
	 {
	 FrSymbol *context = (offset < srclen) ? (FrSymbol*)tokenized[offset] : null ;
	 cphrase[j] = context ;
	 if (offset > (int)srclen)
	    break ;		// don't put in multiple nulls
	 if (offset < srclen)
	    offset += coverage[offset] ;
	 }
      rcontext = WcConcatWords(cphrase,0,range_r,' ') ;
      }
   add_context(counts,lcontext,rcontext,frequency) ;
   return  ;
}

//----------------------------------------------------------------------

static void add_simple_context(int srclen, int i,
			       const FrArray &source, const FrArray &tokenized,
			       const unsigned *coverage, unsigned max_coverage,
			       FrSymbol *null, WcSparseArray *counts,
			       size_t range_l, size_t range_r, size_t phrase_size)
{
   // accumulate the left context: for each word of context, find the
   //   token with the greatest coverage that exactly reaches up to the
   //   following one
   int offset(0) ;
   for (int j = 1 ; j <= (int)range_l ; j++)
      {
      if (i <= -offset)
	 {
	 // we've run off the start of the input, so the context is null
	 FrSymbol *disambig = disambiguate(null,-j,range_l) ;
	 counts->incrCount(disambig,1) ;
	 continue ;
	 }
      int match(0) ;
      unsigned max = ((unsigned)i+offset < max_coverage) ? (i+offset) : max_coverage ;
      for (unsigned o = 1 ; o <= max ; o++)
	 {
	 if (coverage[i+offset-o] == o)
	    {
	    match = o ;
	    }
	 }
      FrSymbol *word ;
      if (match > 0)
	 {
	 offset -= match ;
	 word = (FrSymbol*)tokenized[i+offset] ;
	 }
      else
	 {
	 offset-- ;
	 word = (FrSymbol*)source[i+offset] ;
	 if (!word)
	    {
	    word = null ;
	    cerr << "replacing 0 pointer with null symbol" << endl ;
	    }
	 }
      FrSymbol *disambig = disambiguate(word,-j,range_l) ;
      counts->incrCount(disambig,1) ;
      }
   // accumulate the right context: for each word of tokenized context,
   //   skip ahead by the number of source words it covers
   offset = i + phrase_size ;
   for (int j = 1 ; j <= (int)range_r ; j++)
      {
      FrSymbol *context = (offset < srclen) ? (FrSymbol*)tokenized[offset] : null ;
      if (offset < srclen)
	 offset += coverage[offset] ;
      FrSymbol *disambig = disambiguate(context,j,range_l) ;
      counts->incrCount(disambig,1) ;
      }
   return ;
}

//----------------------------------------------------------------------

void WcAnalyzeContextMono(const FrArray &source, const FrArray &tokenized,
			  const unsigned *coverage, unsigned max_coverage,
			  size_t range_l, size_t range_r, size_t phrase_size,
			  double min_phrase_MI, FrSymHashTable *ht,
			  const FrSymHashTable *desired_words,
			  const WcWordPairTable *mutualinfo, size_t frequency)
{
   (void)min_phrase_MI ; // future enhancement
   // generate a list of phrases of the specified length starting at each
   //   position in the source sentence
   size_t srclen = source.arrayLength() ;
   if (srclen < phrase_size)
      return ;
   if (frequency == 0) frequency = 1 ;
   FrArray swords(srclen) ;
   for (size_t i = 0 ; i <= srclen - phrase_size ; i++)
      {
      swords[i] = (FrSymbol*)source[i] ;
      if (phrase_size > 1)
	 {
	 if (!frequency_sufficient(source,i,phrase_size,desired_words) ||
	     !mutual_info_sufficient(source,i,phrase_size,mutualinfo))
	    {
	    swords[i] = nullptr ; // no good phrase at this position
	    }
	 else if (!is_stopword((FrSymbol*)swords[i]) &&
		  !is_stopword((FrSymbol*)source[i+phrase_size-1]))
	    // generate the phrase if it neither starts nor ends with a stopword
	    {
	    swords[i] = WcConcatWords(source,i,phrase_size,' ') ;
	    }
	 }
      else if (is_stopword((FrSymbol*)source[i])
	       || (exclude_numbers && is_number((FrSymbol*)source[i]))
	       || !frequency_sufficient(source,i,1,desired_words))
	 {
	 swords[i] = nullptr ;		// don't bother with this one, it will get filtered anyway
	 }
      }
   // scan the list of source phrases and accumulate their contexts
   FrSymbol *null = FrSymbolTable::add("") ;
   for (int i = 0 ; i <= (int)(srclen-phrase_size) ; i++)
      {
      FrSymbol *sword = (FrSymbol*)swords[i] ;
      if (!sword || sword == symNUMBER)
	 continue ;
      FrObject **entry = ht->lookupValuePtr(sword) ;
      WcSparseArray *counts ;
      if (entry)
	 {
	 counts = (WcSparseArray*)(*entry) ;
	 if (!counts)
	    {
	    counts = new WcSparseArray ;
	    (*entry) = counts ;
	    }
	 }
      else
	 {
	 counts = new WcSparseArray ;
	 ht->add(sword,counts) ;
	 }
      counts->incrFrequency(frequency) ;
      if (use_phrasal_context)
	 {
	 add_phrasal_context(srclen,i,source,tokenized,coverage,max_coverage,null,counts,
			     range_l,range_r,phrase_size,frequency) ;
	 }
      else
	 {
	 add_simple_context(srclen,i,source,tokenized,coverage,max_coverage,null,counts,
			    range_l,range_r,phrase_size) ;
	 }
      }
   return ;
}

//----------------------------------------------------------------------

bool WcPrepContextMono(const FrList *sent, FrCharEncoding encoding,
		       FrArray *&source_array, FrArray *&tokenized_array,
		       unsigned *&coverage, unsigned &max_coverage,
		       size_t phrase_size, const FrObjHashTable *equivs,
		       bool auto_numbers, bool have_markup)
{
   size_t srclen = listlength(sent) ;
   if (srclen < phrase_size)
      {
      source_array = tokenized_array = nullptr ;
      coverage = nullptr ;
      return false ;
      }
   FrArray *source = new FrArray(srclen) ;
   FrArray *tokenized = new FrArray(srclen) ;
   unsigned *cover = FrNewC(unsigned,srclen) ;
   if (!source || !tokenized || !cover)
      {
      delete source ;
      delete tokenized ;
      FrFree(cover) ;
      source_array = tokenized_array = nullptr ;
      coverage = nullptr ;
      return false ;
      }
   symNUMBER = FrSymbolTable::add(NUMBER_TOKEN) ;
   // strip off any co-indices
   for (size_t i = 0 ; i < srclen ; i++, sent = sent->rest())
      {
      FrObject *srcword = sent->first() ;
      FrSymbol *word ;
      if (have_markup)
	 {
	 char *sym = WcStripCoindex(FrPrintableName(srcword),encoding) ;
	 word = FrSymbolTable::add(sym) ;
	 FrFree(sym) ;
	 }
      else
	 {
	 word = FrSymbolTable::add(FrPrintableName(srcword)) ;
	 }
      (*source)[i] =  word ;
      }
   // generate tokenized form of each source word or matching phrase
   unsigned longest_match = 1 ;
   if (max_coverage == 0)
      max_coverage = 1 ;
   for (size_t i = 0 ; i < srclen ; i++)
      {
      FrString phrase("") ;
      unsigned max = max_coverage ;
      if (i + max > srclen)
	 {
	 max = srclen - i ;
	 }
      unsigned match =  0 ;
      for (unsigned len = 0 ; len < max ; len++)
	 {
	 if (len > 0)
	    {
	    phrase += " " ;
	    }
	 phrase += ((*source)[i+len])->printableName() ;
	 FrSymbol *tok ;
	 if (tokenized_word(&phrase,tok,equivs,auto_numbers))
	    {
	    (*tokenized)[i] = tok ;
	    cover[i] = match = (len + 1) ;
	    }
	 }
      if (match == 0)
	 {
	 (*tokenized)[i] = (FrSymbol*)((*source)[i]) ;
	 cover[i] = 1 ;
	 }
      else if (match > longest_match)
	 longest_match = match ;
      }
   source_array = source ;
   tokenized_array = tokenized ;
   coverage = cover ;
   max_coverage = longest_match ;
   return true ;
}

//----------------------------------------------------------------------

void WcAnalyzeContextMono(const FrList *sent, FrCharEncoding encoding,
			  size_t range_l, size_t range_r, size_t phrase_size,
			  double min_phrase_MI, FrSymHashTable *ht,
			  const FrObjHashTable *equivs, const FrSymHashTable *desired_words,
			  unsigned max_equiv_length, WcWordPairTable *mutualinfo, size_t frequency,
			  bool auto_numbers, bool have_markup)
{
   FrArray *source ;
   FrArray *tokenized ;
   unsigned *coverage ;
   unsigned max_coverage = max_equiv_length ;
   if (WcPrepContextMono(sent,encoding,source,tokenized,coverage,max_coverage,phrase_size,equivs,
			 auto_numbers,have_markup))
      {
      WcAnalyzeContextMono(*source,*tokenized,coverage,max_coverage,range_l,range_r,
			   phrase_size,min_phrase_MI,ht,desired_words,mutualinfo,frequency) ;
      delete source ;
      delete tokenized ;
      FrFree(coverage) ;
      }
   return ;
}

//----------------------------------------------------------------------

static bool convert_counts(const FrSymbol *key, FrObject *entry, va_list args)
{
   WcSparseArray *counts = (WcSparseArray*)entry ;
   if (counts)
      {
      size_t freq = counts->frequency() ;
      if (freq == 0) freq = 1 ;
      FrVarArg(FrSymCountHashTable*,uniwordfreq) ;
      FrVarArg(FrSymHashTable*,ht) ;
      WcTermVector *tv = new WcTermVector(counts) ;
      counts->freeObject() ;
      tv->setKey(key) ;
      tv->setFreq(freq) ;
      ht->add(key,tv,true) ;		// replace sparse array by term vector
      if (uniwordfreq && !uniwordfreq->contains(key))
	 {
	 uniwordfreq->add(key,freq) ;
	 }
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcConvertCounts2Vectors(FrSymHashTable *key_words,
			     FrSymCountHashTable *uniwordfreq)
{
   if (key_words)
      key_words->iterate(convert_counts,uniwordfreq,key_words) ;
   return ;
}

//----------------------------------------------------------------------

static bool clear_term_vector(const FrSymbol *, FrObject *vec, va_list)
{
   WcTermVector *tv = (WcTermVector*)vec ;
   delete tv ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcClearTermVectors(FrSymHashTable *ht)
{
   if (ht)
      ht->iterateAndClear(clear_term_vector) ;
   return ;
}

//----------------------------------------------------------------------

static bool clear_count(const FrSymbol *, FrObject *c, va_list)
{
   WcSparseArray *cnt = (WcSparseArray*)c ;
   delete cnt ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcClearCounts(FrSymHashTable *ht)
{
   if (ht)
      ht->iterateAndClear(clear_count) ;
   return ;
}

//----------------------------------------------------------------------

static bool remove_auto_cluster(FrObject *seed, FrObject *cl, va_list args)
{
   FrSymbol *cluster = static_cast<FrSymbol*>(cl) ;
   FrVarArg(FrObjHashTable *,ht) ;
   if (cluster && FrIsGeneratedClusterName(cluster))
      {
      ht->remove(seed) ;
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcRemoveAutoClustersFromSeeds(FrObjHashTable *seeds)
{
   if (seeds)
      seeds->iterate(remove_auto_cluster,seeds) ;
   return ;
}

//----------------------------------------------------------------------

static bool adjust_context_weight(const FrSymbol *, FrObject *vec, va_list args)
{
   WcTermVector *tv = (WcTermVector*)vec ;
   FrVarArg(size_t,range) ;
   FrVarArg(const FrSymCountHashTable *,uniwordfreq) ;
   FrVarArg(size_t,corpus_size) ;
   if (tv)
      tv->weightTerms(decay_type,past_boundary_weight,range,uniwordfreq,
		      corpus_size) ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcAdjustContextWeights(FrSymHashTable *ht,size_t range_l, size_t range_r,
			    const FrSymCountHashTable *uniwordfreq,
			    size_t corpus_size)
{
   assertq(ht != 0) ;
   if (decay_type != Decay_None)
      {
      size_t range = range_l > range_r ? range_l : range_r ;
      ht->iterate(adjust_context_weight,range,uniwordfreq,corpus_size) ;
      }
   return ;
}

//----------------------------------------------------------------------

// end of file wcanalyz.cpp //

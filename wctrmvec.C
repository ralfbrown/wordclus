/************************************************************************/
/*									*/
/*  WordCluster								*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wctrmvec.cpp	      term vectors				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2002,2005,2009,2015 			*/
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

#include <math.h>
#include <stdlib.h>
#include "wordclus.h"
#include "wcglobal.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

#ifndef NDEBUG
#  undef _FrCURRENT_FILE
   static const char _FrCURRENT_FILE[] = __FILE__ ;
#endif /* !NDEBUG */

/************************************************************************/
/* 	Helper Functions						*/
/************************************************************************/

inline double maximum(double x,double y) { return x > y ? y : x ; }

/************************************************************************/
/*	Methods for WcTermVector					*/
/************************************************************************/

static FrCons *disambiguate(FrSymbol *word, int position, size_t range)
{
   WordInfo *disamb = (WordInfo*)word->symbolFrame() ;
   if (!disamb)
      {
      disamb = (WordInfo*)FrMalloc(sizeof(WordInfo)+2*range*sizeof(FrSymbol*));
      if (disamb)
	 {
	 (void)new (disamb)WordInfo ;
	 disamb->initPositions(word,range) ;
	 word->setFrame((FrFrame*)disamb) ;
	 }
      else
	 {
	 FrNoMemory("disambiguating word by position") ;
	 return new FrList(word,new FrInteger(1)) ;
	 }
      }
   return new FrCons(disamb->disambiguate(position,range),new FrInteger(1)) ;
}

//----------------------------------------------------------------------

WcTermVector::WcTermVector(const FrArray *doc, size_t anchorpoint,
			   size_t range)
{
   if (!doc)
      {
      init() ;
      return ;
      }
   FrList *wordlist = nullptr ;
   FrSymbol *null = FrSymbolTable::add("") ;
   size_t docsize = doc->arrayLength() ;
   for (int i = anchorpoint + range ; i >= (int)(anchorpoint - range) ; i--)
      {
      if (i == (int)anchorpoint)
	 continue ;
      if (i >= 0 && i < (int)docsize)
	 pushlist(disambiguate((FrSymbol*)doc->getNth(i),i-anchorpoint,range),
		  wordlist) ;
      else
	 pushlist(disambiguate(null,i-anchorpoint,range),wordlist) ;
      }
   bool sorted = false ;
   init(wordlist,2*range+1,sorted) ;
   return ;
}

//----------------------------------------------------------------------

static double word_probability(FrSymbol *word, const FrSymCountHashTable *ht,
			       size_t corpus_size)
{
   if (ht && corpus_size > 0)
      {
      size_t count ;
      if (ht->lookup(word,&count))
	 return (count / (double)corpus_size) ;
      }
   return 0.0 ;
}

//----------------------------------------------------------------------

void WcTermVector::weightTerms(WcDecayType decay, double null_weight,
			       size_t range, const FrSymCountHashTable *freq,
			       size_t corpus_size)
{
   vector_length = 0.0 ;
   for (size_t term = 0 ; term < num_terms ; term++)
      {
      FrSymbol *word = *(FrSymbol**)terms[term] ;
      if (word)
	 {
	 WordInfo *info = (WordInfo*)word->symbolFrame() ;
	 if (info)
	    {
	    int pos = info->wordPosition(terms[term],range) ;
	    if (pos != 0)
	       {
	       if (termfreq_discount != 1.0)
		  weights[term] = pow(weights[term],termfreq_discount) ;
	       double freqwt = 1.0 ;
	       if (decay_beta != 0.0)
		  {
		  double wordfreq = word_probability(word,freq,corpus_size) ;
		  if (decay_beta > 0.0)
		     freqwt = maximum(exp(-decay_beta * wordfreq),decay_gamma);
		  else
		     {
		     double prob = -decay_beta * wordfreq ;
		     if (prob > 1.0) prob = 1.0 ;
		     freqwt = -log2(prob) ;
		     }
		  }
	       double weight ;
	       if (decay == Decay_Exponential)
		  {
		  weight = exp(-decay_alpha * fabs((double)pos)) ;
		  }
	       else if (decay == Decay_Linear)
		  weight = ((range + 1) -
			    fabs((double)pos)) / (double)(range+1) ;
	       else if (decay == Decay_Reciprocal)
		  weight = 1.0 / fabs((double)pos) ;
	       else
		  weight = 1.0 ;
	       if (*word->symbolName() == '\0')
		  weight *= null_weight ;
	       weights[term] *= (weight * freqwt) ;
	       }
	    }
	 }
      vector_length += (weights[term] * weights[term]) ;
      }
   vector_length = sqrt(vector_length) ;
   return ;
}

//----------------------------------------------------------------------

double WcTermVector::splitCosine(const FrTermVector *othervect,
				 int position, int range) const
{
   if (vectorLength() == 0.0 || othervect->vectorLength() == 0.0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   FrSymbol *word1(getTerm(0)) ;
   FrSymbol *word2(othervect->getTerm(0)) ;
   size_t other_terms(othervect->numTerms()) ;
   double cosin(0.0) ;
   double vlen1(0.0) ;
   double vlen2(0.0) ;
   for ( ; ; )
      {
      if (word1 <= word2)
	 {
	 if (word1 == word2)
	    {
	    FrSymbol *sym1 = *((FrSymbol**)word1) ;
	    FrSymbol *sym2 = *((FrSymbol**)word2) ;
	    WordInfo *info1 = (WordInfo*)sym1->symbolFrame() ;
	    WordInfo *info2 = (WordInfo*)sym2->symbolFrame() ;
	    if (info1 && info2 &&
		(info1->wordPosition(word1,range) * position) >= 0 &&
		(info2->wordPosition(word2,range) * position) >= 0)
	       {
	       double wt1 = termWeight(term1) ;
	       double wt2 = othervect->termWeight(term2++) ;
	       cosin += (wt1 * wt2) ;
	       vlen1 += (wt1 * wt1) ;
	       vlen2 += (wt2 * wt2) ;
	       }
	    if (term2 >= other_terms)
	       break ;
	    word2 = othervect->getTerm(term2) ;
	    }
	 if (++term1 >= num_terms)	// advance first term list
	    break ;
	 word1 = getTerm(term1) ;
	 }
      else
	 {
	 if (++term2 >= other_terms)	// advance second term list
	    break ;			//   and quit if at and
	 word2 = othervect->getTerm(term2) ;
	 }
      }
   if (vlen1 > 0.0 && vlen2 > 0.0)
      return cosin / (sqrt(vlen1) * sqrt(vlen2)) ;
   else
      return 0.0 ;
}

// end of file wctrmvec.cpp //

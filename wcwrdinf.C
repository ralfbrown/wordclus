/************************************************************************/
/*									*/
/*  WordCluster								*/
/*  Version 0.60							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcwrdinf.cpp	      WordInfo					*/
/*  LastEdit: 04jun00							*/
/*									*/
/*  (c) Copyright 1999,2000 Ralf Brown/Carnegie Mellon University	*/
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
#include "wordclus.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define DEFAULT_IDF 10000.0

/************************************************************************/
/************************************************************************/

/************************************************************************/
/*	Methods for class WordInfo					*/
/************************************************************************/

void WordInfo::initPositions(FrSymbol *word, size_t range)
{
   for (size_t i = 0 ; i <= 2*range ; i++)
      disambig_symbols[i] = word ;
   return ;
}

//----------------------------------------------------------------------

int WordInfo::wordPosition(const FrSymbol *marker, size_t range) const
{
   const FrSymbol * const *end = &disambig_symbols[2*range] ;
   const FrSymbol * const *word = (FrSymbol**)marker ;
   if (word >= disambig_symbols && word <= end)
      return (word - (const FrSymbol *const*)disambig_symbols) - (int)range ;
   else
      return 0 ;
}

//----------------------------------------------------------------------

double WordInfo::invDocFrequency(size_t total_docs) const
{
   if (doc_freq)
      return log(total_docs / (double)doc_freq) ;
   else
      return DEFAULT_IDF ;
}

//----------------------------------------------------------------------

double WordInfo::TF_IDF(size_t total_docs) const
{
   if (doc_freq)
      return term_freq * log(total_docs / (double)doc_freq) / doc_freq ;
   else
      return DEFAULT_IDF ;
}

// end of file wcwrdinf.cpp //

/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wctoken.C	      word-token processing			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2015 Ralf Brown/Carnegie Mellon University		*/
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

#ifndef __WCTOKEN_H_INCLUDED
#define __WCTOKEN_H_INCLUDED

#include "FramepaC.h"

/************************************************************************/
/************************************************************************/

/************************************************************************/
/************************************************************************/

void EBMT_set_word_delimiters(FrCharEncoding enc) ;
void EBMT_set_word_delimiters(const char *delim) ;

bool WcIsToken(const char *word) ;
bool WcIsToken(FrSymbol *word) ;
bool WcIsToken(const FrObject *obj) ;

bool EBMT_equal(const FrObject *s1, const FrObject *s2) ;

bool WcTokenEqual(FrSymbol *s1, FrSymbol *s2) ;
bool WcTokenEqual(FrSymbol *o1, const FrObject *o2) ;
inline bool WcTokenEqual(const FrObject *o1, FrSymbol *o2)
   { return WcTokenEqual(o2,o1) ; }
bool WcTokenEqual(const FrObject *o1, const FrObject *o2) ;

char *WcStripCoindex(const char *word, FrCharEncoding encoding) ;


#endif /* !__WCTOKEN_H_INCLUDED */

// end of file wctoken.h //

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

#include "wctoken.h"
#include "wordclus.h"
#include "wcglobal.h"

#if defined(unix) || defined(__linux__) || defined(__GNUC__)
#include <unistd.h> // for rmdir()
#endif /* unix */

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define EbTOKEN_START '<'
#define EbTOKEN_END   '>'

/************************************************************************/
/*	Global Data for this module					*/
/************************************************************************/

static const char text_read_mode[] = "r" ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

bool WcIsToken(const char *word)
{
   return (word && word[0] == EbTOKEN_START && word[1] != '\0' &&
           strchr(word+1,EbTOKEN_END) != 0) ;
}

//----------------------------------------------------------------------

bool WcIsToken(FrSymbol *word)
{
   return word && WcIsToken(word->symbolName()) ;
}

//----------------------------------------------------------------------

bool WcIsToken(const FrObject *obj)
{
   return obj && WcIsToken(obj->printableName()) ;
}

//----------------------------------------------------------------------

char *WcStripCoindex(const char *word, FrCharEncoding encoding)
{
   if (!word)
      return 0 ;
   char name[FrMAX_SYMBOLNAME_LEN+2] ;
   strncpy(name,word,sizeof(name)) ;
   name[sizeof(name)-1] = '\0' ;
   if (word[0] == EbTOKEN_START)
      {
      char *end = strrchr(name,EbTOKEN_END) ;
      if (end)
	 {
	 // OK, we have a token which has been decorated with a co-index
	 // so strip off the co-index
	 end[1] = '\0' ;
	 }
      }
   Fr_strlwr(name,encoding) ;
   return FrDupString(name) ;
}

//----------------------------------------------------------------------

bool WcTokenEqual(FrSymbol *s1, FrSymbol *s2)
{
   if (s1 == s2)
      return true ;
   else if (s1 && s2)
      {
      const char *str1 = s1->symbolName() ;
      const char *str2 = s2->symbolName() ;
      const char *end = FrTruncationPoint(str1,EbTOKEN_END) ;
      return memcmp(str1,str2,(end-str1+1)) == 0 ;
      }
   else
      return false ;
}

//----------------------------------------------------------------------

bool WcTokenEqual(FrSymbol *o1, const FrObject *o2)
{
   if (o1 == o2)
      return true ;
   else if (o1 && o2)
      {
      const char *s2 = o2->printableName() ;
      if (s2)
	 {
	 // we can avoid the computational expense of generating new
	 //   symbols sans any coindices, and just directly check the
	 //   names against each other
	 const char *s1 = o1->symbolName() ;
	 const char *end = FrTruncationPoint(s1,EbTOKEN_END) ;
	 return Fr_strnicmp(s1,s2,(end-s1+1),lowercase_table) == 0 ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

bool WcTokenEqual(const FrObject *o1, const FrObject *o2)
{
   if (o1 == o2)
      return true ;
   else if (o1 && o2)
      {
      const char *s1 = o1->printableName() ;
      const char *s2 = o2->printableName() ;
      if (s1 && s2)
	 {
	 // if both are symbols or strings, we can avoid the computational
	 //   expense of generating new symbols sans any coindices, and
	 //   just directly check the names against each other
	 const char *end = FrTruncationPoint(s1,EbTOKEN_END) ;
	 return Fr_strnicmp(s1,s2,(end-s1+1),lowercase_table) == 0 ;
	 }
      if (o1->consp() && o2->consp())
	 {
	 FrList *l1 = (FrList*)o1 ;
	 FrList *l2 = (FrList*)o2 ;
	 while (l1 && l2)
	    {
	    if (!WcTokenEqual(l1->first(),l2->first()))
	       return false ;
	    l1 = l1->rest() ;
	    l2 = l2->rest() ;
	    }
	 return true ;
	 }
      }
   return false ;
}

// end of file wctoken.C //

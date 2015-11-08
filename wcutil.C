/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcutil.cpp	      utility functions				*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2015 				*/
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

#include "wordclus.h"

/************************************************************************/
/************************************************************************/

FILE *WcOpenCorpus(const char *filename)
{
   FILE *fp ;
   if (strcmp(filename,"-") == 0)
      fp = stdin ;
   else
      {
      fp = fopen(filename,"r") ;
      if (!fp)
	 FrWarningVA("unable to open corpus file\n\t%s",filename) ;
      }
   return fp ;
}

//----------------------------------------------------------------------

void WcCloseCorpus(FILE *fp)
{
   if (fp != stdin && fp != 0)
      fclose(fp) ;
   return ;
}

//----------------------------------------------------------------------

FrSymbol *WcConcatWords(FrSymbol *word1, FrSymbol *word2, char sep)
{
   char name[2*FrMAX_SYMBOLNAME_LEN+3] ;
   strcpy(name,word1 ? word1->symbolName() : "") ;
   size_t len = strlen(name) ;
   if (len > FrMAX_SYMBOLNAME_LEN)
      len = sizeof(name) - 2 ;
   name[len] = sep ;
   strncpy(name+len+1,word2 ? word2->symbolName() : "",FrMAX_SYMBOLNAME_LEN) ;
   //name[2*FrMAX_SYMBOLNAME_LEN+1] = '\0' ;
   name[FrMAX_SYMBOLNAME_LEN] = '\0' ;
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

FrSymbol *WcConcatWords(const FrArray &words, size_t start, size_t len, char sep)
{
   char name[2*FrMAX_SYMBOLNAME_LEN+3] ;
   size_t namelen = 0 ;
   for (size_t i = start ; i < start + len ; i++)
      {
      FrSymbol *word = (FrSymbol*)words[i] ;
      if (!word)
	 continue ;
      const char *wordstr = word->symbolName() ;
      size_t wordlen = strlen(wordstr) ;
      if (namelen > 0)
	 name[namelen++] = sep ;
      memcpy(name+namelen,wordstr,wordlen) ;
      namelen += wordlen ;
      if (namelen > FrMAX_SYMBOLNAME_LEN)
	 {
	 namelen = FrMAX_SYMBOLNAME_LEN ;
	 break ;
	 }
      }
   name[namelen] = '\0' ;
   return FrSymbolTable::add(name) ;
}

//----------------------------------------------------------------------

FrSymbol *WcConcatPhrases(const FrList *words1, const FrList *words2)
{
   char namebuf[2*FrMAX_SYMBOLNAME_LEN+3] ;
   char *name = namebuf ;
   while (words1 && name - namebuf <= FrMAX_SYMBOLNAME_LEN)
      {
      FrObject *wd = words1->first() ;
      words1 = words1->rest() ;
      if (wd)
	 {
	 const char *word = wd->printableName() ;
	 strcpy(name,word) ;
	 name = strchr(name,'\0') ;
	 if (words1)
	    *name++ = ' ' ;
	 }
      }
   *name++ = WC_CONCAT_WORDS_SEP ;
   while (words2 && name - namebuf <= 2*FrMAX_SYMBOLNAME_LEN)
      {
      FrObject *wd = words2->first() ;
      words2 = words2->rest() ;
      if (wd)
	 {
	 const char *word = wd->printableName() ;
	 strcpy(name,word) ;
	 name = strchr(name,'\0') ;
	 if (words2)
	    *name++ = ' ' ;
	 }
      }
   return FrSymbolTable::add(namebuf) ;
}

// end of file wcutil.cpp //

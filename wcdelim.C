/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcdelim.C	      word delimiters				*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 2015,2016,2017,2018 Carnegie Mellon University	*/
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

#include "wordclus.h"
#include "framepac/list.h"

using namespace Fr ;

/************************************************************************/
/*	Globals							        */
/************************************************************************/

static CharPtr word_delimiters { nullptr } ;

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

const char *WcWordDelimiters()
{
   return word_delimiters ;
}

//----------------------------------------------------------------------

void WcSetWordDelimiters(const char *delim)
{
   if (delim)
      {
      word_delimiters = CharPtr(256,delim,256) ;
      }
   else
      {
      word_delimiters = nullptr ;
      }
   return ;
}

//----------------------------------------------------------------------

void WcClearWordDelimiters()
{
   WcSetWordDelimiters(nullptr) ;
   return ;
}

//----------------------------------------------------------------------

static bool is_punctuation(const Object *word)
{
   if (!word)
      return false ;
   const char* w = word->stringValue() ;
   return (w && ispunct(w[0]) && w[1] == '\0') ;
}

//----------------------------------------------------------------------

List* remove_punctuation(List* wordlist)
{
   return wordlist->removeIf(is_punctuation) ;
}

//----------------------------------------------------------------------

// end of file wcdelim.C //

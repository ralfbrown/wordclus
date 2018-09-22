/************************************************************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcglobal.cpp	      global variables				*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 2000,2002,2003,2005,2009,2010,2015,2016,2017,2018	*/
/*	   Ralf Brown/Carnegie Mellon University			*/
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

/************************************************************************/
/*	Manifest constants for this module				*/
/************************************************************************/

/************************************************************************/
/*	Global variables for this module				*/
/************************************************************************/

static std::locale* char_encoding ;
bool lowercase_output { false } ;

/************************************************************************/
/************************************************************************/

bool WcLowercaseOutput()
{
   return lowercase_output ;
}

//----------------------------------------------------------------------

void WcLowercaseOutput(bool lc)
{
   lowercase_output = lc ;
   return ;
}

//----------------------------------------------------------------------

std::locale* WcCurrentCharEncoding()
{
   return char_encoding ;
}

//----------------------------------------------------------------------

void WcSetCharEncoding(const char *char_enc)
{
   delete char_encoding ;
   try
      {
      char_encoding = new std::locale(char_enc) ;
      }
   catch (std::exception&)
      {
      try
	 {
	 char_encoding = new std::locale() ;
	 }
      catch (std::exception&)
	 {
	 char_encoding = new std::locale("C") ;
	 }
      }
   return ;
}

// end of file wcglobal.cpp //

/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcbatch.h	      batch-of-lines processing			*/
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

#ifndef __WCBATCH_H_INCLUDED
#define __WCBATCH_H_INCLUDED

#include <cstdio>	// for FILE*

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define WcLINES_PER_BATCH 1000

/************************************************************************/
/*	Types								*/
/************************************************************************/

class WcLineBatch
   {
   private:
      WcLineBatch *m_next ;
      char        *m_lines[WcLINES_PER_BATCH] ;
      char	  *m_altlines[WcLINES_PER_BATCH] ;
      char        *m_tags[WcLINES_PER_BATCH] ;
      size_t       m_numlines ;
   public:
      WcLineBatch() ;
      ~WcLineBatch() { clear() ; }

      // accessors
      WcLineBatch *next() const { return m_next ; }
      size_t numLines() const { return m_numlines ; }
      size_t maxLines() const { return lengthof(m_lines) ; }
      const char *line(size_t N) const { return (N < numLines()) ? m_lines[N] : 0 ; }
      const char *altLine(size_t N) const { return (N < numLines()) ? m_altlines[N] : 0 ; }
      const char *tags(size_t N) const {  return (N < numLines()) ? m_tags[N] : 0 ; }

      // modifiers
      void next(WcLineBatch *n) { m_next = n ; }
      void clear() ;
      bool fill(FILE *fp, bool monoling = false, bool reverse_langs = false,
		int mono_skip = 0, bool downcase_source = false, unsigned char *charmap = 0,
		const char *delim = 0) ;
      bool process(...) const ;
   } ;

typedef bool WcProcessFileFunc(const WcLineBatch &lines, const WcParameters *params,
			       va_list args) ;

/************************************************************************/
/************************************************************************/

bool WcProcessFile(FILE *fp, const WcParameters *params, WcProcessFileFunc *fn, ...) ;

#endif /* !__WCBATCH_H_INCLUDED */

// end of file wcbatch.h //

/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcbatch.h	      batch-of-lines processing			*/
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

#ifndef __WCBATCH_H_INCLUDED
#define __WCBATCH_H_INCLUDED

#include "framepac/file.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define WcLINES_PER_BATCH 10000

/************************************************************************/
/*	Types								*/
/************************************************************************/

typedef bool WcProcessFileFunc(const Fr::LineBatch &lines, const WcParameters *params,
			       va_list args) ;

/************************************************************************/
/************************************************************************/

bool WcProcessFile(Fr::CFile& fp, const WcParameters *params, WcProcessFileFunc *fn, ...) ;

#endif /* !__WCBATCH_H_INCLUDED */

// end of file wcbatch.h //

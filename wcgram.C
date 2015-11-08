/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcgram.cpp	      grammar induction (main program)		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2001,2002,2003,2005,2006,2008,2009,2010,2015		*/
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
#include "FramepaC.h"
#include "wordclus.h"
#include "wcglobal.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define ALLOC_GRANULARITY 1000

/************************************************************************/
/*	Types for this modules						*/
/************************************************************************/

/************************************************************************/
/*	Globals for this module						*/
/************************************************************************/

size_t required_common_words = 2 ;

/************************************************************************/
/*	External Functions						*/
/************************************************************************/

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

/************************************************************************/
/*	Methods for class WcParallelCorpus				*/
/************************************************************************/

/************************************************************************/
/************************************************************************/

// end of file wcgram.cpp //

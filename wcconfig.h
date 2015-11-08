/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcconfig.h	      configuration file			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2002,2003,2005,2006,2008,2010,2015		*/
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

#ifndef __WCCONFIG_H_INCLUDED
#define __WCCONFIG_H_INCLUDED

#include <stdio.h>
#include "FramepaC.h"

//-----------------------------------------------------------------------

#ifdef __GNUC__
#  define NEED_STRICMP
#endif

//-----------------------------------------------------------------------

#define DEFAULT_WORDCLUS_CFGFILE "wordclus.cfg"

#define WC_VERBOSE		0x0001
#define WC_UNICODE		0x0002
#define WC_BYTESWAP		0x0004

#define WC_SHOWMEM		0x0010
#define WC_MONOLINGUAL		0x0020
#define WC_REMOVE_AMBIG		0x0040
#define WC_CONTEXTONLY		0x0080

#define WC_DOWNCASE		0x0100
#define WC_DECAY		0x0200
#define WC_LINDECAY		0x0400
#define WC_EXPDECAY		0x0800
#define WC_SEP_PUNCT		0x1000
#define WC_SEP_NUMBERS		0x2000
#define WC_IGNORE_SRC_CASE	0x4000
#define WC_IGNORE_TGT_CASE	0x8000

#define WC_SUM_SIZES           0x10000
#define WC_NOFILLGAPS	       0x20000

#define WC_SKIPAUTOCLUST       0x40000
#define WC_TWOPASS	       0x80000
#define WC_ITERATEDGRAMMAR    0x100000

#define WC_EXCL_PUNCT	     0x1000000
#define WC_EXCL_NUM	     0x2000000
#define WC_KEEP_SINGLETONS   0x4000000

#define WC_PHRASAL_CONTEXT 0x100000000
#define WC_SINGLE_CONTEXT  0x200000000

//-----------------------------------------------------------------------

class WcConfig : public FrConfiguration
   {
   public:
      uint32_t  options ;
      FrList   *tight_bound_left ;
      FrList   *tight_bound_right ;
      FrSymbol *clust_metric ;
      FrSymbol *clust_rep ;
      FrSymbol *clust_type ;
      char     *thresh_file ;
      char     *char_enc ;
      long	min_freq ;
      long	max_freq ;
      long	max_terms ;
      long	max_ambig ;
      long	max_diff ;
      long	sl_context_left ;
      long	sl_context_right ;
      long	tl_context_left ;
      long	tl_context_right ;
      long 	word_order ;
      long      clust_iter ;
      double	past_bound_weight ;
      int	last_local_var ;	// must be last in list
   private:
      static FrConfigurationTable WC_def[] ;
   public: // methods
      WcConfig(const char *base_dir) : FrConfiguration(base_dir) {}
      virtual void init() ;
      virtual void resetState() ;
      virtual ~WcConfig() ;
      virtual size_t lastLocalVar() const ;
      virtual ostream &dump(ostream &output) const ;
   } ;

//----------------------------------------------------------------------

WcConfig *get_WordClus_setup_file(const char *filespec, const char *argv0,
				  ostream &err) ;
const WcConfig *get_WordClus_config() ;
void unload_WordClus_config() ;

#endif /* !__WCCONFIG_H_INCLUDED */

// end of file wcconfig.h //


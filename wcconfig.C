/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcconfig.cpp	      configuration file			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2000,2002,2003,2005,2006,2008,2009,2010,2015		*/
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

#include "wcconfig.h"
#include "wordclus.h"
#include "wcglobal.h"
#ifdef FrSTRICT_CPLUSPLUS
#  include <cstdlib>
#  include <fstream>
#else
#  include <fstream.h>
#  include <stdlib.h>
#endif

/************************************************************************/
/*  the parsing information for the WordClus configuration		*/
/************************************************************************/

static FrCommandBit wc_options[] =
{
   { "VERBOSE",     	WC_VERBOSE,		false,	"" },
   { "SHOWMEM",		WC_SHOWMEM,		false,	"" },
   { "UNICODE",		WC_UNICODE,		false,	"" },
   { "BYTESWAP",	WC_BYTESWAP,		false,	"" },
   { "REMOVE_AMBIG",	WC_REMOVE_AMBIG,	false,	"" },
   { "DOWNCASE",	WC_DOWNCASE,		false,	"" },
   { "DOWNCASE_SRC",	WC_IGNORE_SRC_CASE,	true,	"" },
   { "DOWNCASE_TRG",	WC_IGNORE_TGT_CASE,	false,	"" },
   { "DECAY",		WC_DECAY,		false,	"" },
   { "LINDECAY",	WC_LINDECAY,		false,	"" },
   { "EXPDECAY",	WC_EXPDECAY,		false,	"" },
   { "SEP_PUNCT",	WC_SEP_PUNCT,		false,	"" },
   { "SEP_NUMBERS",	WC_SEP_NUMBERS,		false,	"" },
   { "MONOLINGUAL",	WC_MONOLINGUAL,		false,	"" },
   { "SUM_SIZES",	WC_SUM_SIZES,		false,	"" },
   { "EXCL_PUNCT",	WC_EXCL_PUNCT,		false,	"" },
   { "NO_FILLGAPS",	WC_NOFILLGAPS,		false,	"" },
   { "CONTEXT_ONLY",	WC_CONTEXTONLY,		false,	"" },
   { "SKIP_AUTO_CLUST", WC_SKIPAUTOCLUST,	false,	"" },
   { "KEEP_SINGLETONS",	WC_KEEP_SINGLETONS,	false,	"" },
   { "PHRASAL_CONTEXT",	WC_PHRASAL_CONTEXT,	false,	"" },
   { "SINGLE_CONTEXT",	WC_SINGLE_CONTEXT,	false,	"" },
   { "TWOPASS",		WC_TWOPASS,		false,	"" },
   { "ITERATED_GRAMMAR",WC_ITERATEDGRAMMAR,	false,  "" },
   { 0,			0,			false,	"" }
} ;

static WcConfig *dummy_WcConfig = nullptr ;
#undef addr
#define addr(x) (void*)((char*)(&dummy_WcConfig->x) - (char*)dummy_WcConfig)

FrConfigurationTable WcConfig::WC_def[] =
{
 	// keyword	parsing func   location		 settability
	//   	next-state  extra-args default	min	max
	//	description
      // keyword	parsing func	storage loc	next state  extra-arg
   { "Base-Directory",	basedir,	0,			FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Char-Encoding",	cstring,	addr(char_enc),		FrSET_STARTUP,
		0,    	0,			"UTF8",	0,	0,
		"" },
   { "Cluster-Metric",	symbol,		addr(clust_metric),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Cluster-Iter",	integer,	addr(clust_iter),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Cluster-Rep",	symbol,		addr(clust_rep),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Cluster-Type",	symbol,		addr(clust_type),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Decay-Alpha",	real,		&decay_alpha,		FrSET_ANYTIME,
		0,    	0,			"0.5",	0,	0,
		"" },
   { "Decay-Beta",	real,		&decay_alpha,		FrSET_ANYTIME,
		0,    	0,			"0.5",	0,	0,
		"" },
   { "Decay-Gamma",	real,		&decay_alpha,		FrSET_ANYTIME,
		0,    	0,			"0.1",	0,	0,
		"" },
   { "TermFreq-Discount", real,		&termfreq_discount,	FrSET_ANYTIME,
		0,    	0,			"1.0",	"0.0",	"2.0",
		"" },
   { "Min-Freq",	integer,	addr(min_freq),		FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Max-Terms", 	integer,	addr(max_terms),	FrSET_STARTUP,
		0,    	0,			"-1",	"100",	"99999999",
		"" },
   { "Max-Freq",	integer,	addr(max_freq),		FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Max-Ambiguity",	integer,	addr(max_ambig),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Max-Difference",	integer,	addr(max_diff),		FrSET_ANYTIME,
		0,    	0,			"12",	"1",	"9999",
		"" },
   { "Neighborhood",	integer,	addr(sl_context_left),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Neighborhood-Left",integer,	addr(sl_context_left),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Neighborhood-Right",integer,	addr(sl_context_right), FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Null-Weight",	real,		addr(past_bound_weight),FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Options",		bitflags,	addr(options),	   	FrSET_STARTUP,
		0, 	wc_options,		0,	0,	0,
      		"" },
   { "Threshold-File",	filename,	addr(thresh_file),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Tight-Bound-Left",list,     	addr(tight_bound_left),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Tight-Bound-Right", list,     	addr(tight_bound_right),FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "TL-Context-Left",	integer,	addr(tl_context_left),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "TL-Context-Right",integer,	addr(tl_context_right),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "Word-Order-Sim",	integer,	addr(word_order),	FrSET_STARTUP,
		0,    	0,			0,	0,	0,
		"" },
   { "",		invalid,	0,			FrSET_READONLY,
		0,    	0,			0,	0,	0,
		"" },
      // what to do if we reach EOF
   { 0,			invalid,	0,			FrSET_READONLY,
		0,    	0,			0,	0,	0,
		"" },
} ;

/************************************************************************/
/*	Methods for class WcConfig					*/
/************************************************************************/

void WcConfig::init()
{
   FrConfiguration::init() ;
   tight_bound_left = 0 ;
   tight_bound_right = 0 ;
   past_bound_weight = -1.0 ;
   min_freq = -1 ;
   max_freq = -1 ;
   max_terms = -1 ;
   max_ambig = -1 ;
   max_diff = UINT_MAX ;
   tl_context_left = -1 ;
   tl_context_right = -1 ;
   word_order = -1 ;
   return ;
}

//-----------------------------------------------------------------------

void WcConfig::resetState()
{
   curr_state = WC_def ;
   return ;
}

//----------------------------------------------------------------------

WcConfig::~WcConfig()
{
   freeValues() ;
   return ;
}

//-----------------------------------------------------------------------

size_t WcConfig::lastLocalVar() const
{
   return (size_t)((char*)(&this->last_local_var) - (char*)this) ;
}

//-----------------------------------------------------------------------

ostream &WcConfig::dump(ostream &output) const
{
   return dumpValues("WcConfig",output) ;
}

/************************************************************************/
/************************************************************************/

static WcConfig *make_WcConfig(istream &input,const char *base_directory)
{
   WcConfig *cfg = new WcConfig(base_directory) ;
   if (cfg)
      cfg->load(input,true) ;
   return cfg ;
}

/************************************************************************/
/************************************************************************/

static WcConfig *wc_config = nullptr ;

//----------------------------------------------------------------------

static WcConfig *load_setup_file(const char *filename)
{
   ifstream in(filename) ;
   if (in.good())
      {
      delete wc_config ;		// unload any prior setup file
      char *base_directory = FrFileDirectory(filename) ;
      wc_config = make_WcConfig(in,base_directory) ;
      FrFree(base_directory) ;
//      wc_config->dump(cout) ;
      return wc_config ;
      }
   else
      return 0 ;
}

//----------------------------------------------------------------------

static WcConfig *try_loading_setup(const char *dir, const char *basename)
{
   char *filename = FrAddDefaultPath(basename,dir) ;
   if (!filename)
      {
      FrNoMemory("while trying to load configuration file") ;
      return 0 ;
      }
   WcConfig *config = load_setup_file(filename) ;
   FrFree(filename) ;
   return config ;
}

//----------------------------------------------------------------------

WcConfig *get_WordClus_setup_file(const char *filespec, const char *argv0,
				  ostream &err)
{
   WcConfig *config ;
   if (strcmp(filespec,".") != 0)
      {
      config = load_setup_file(filespec) ;
      if (!config)
	 {
	 err << "unable to open specified setup file, "
		 "will use default file" << endl ;
	 }
      return config ;
      }
   // if we get here, either no setup file was specified, or unable to open it
   // so, try to load wordclus.cfg from the current directory, then
   // .wordclus.cfg from user's home directory, and finally wordclus.cfg from
   // the directory containing the executable
   const char *cfg = DEFAULT_WORDCLUS_CFGFILE ;
   config = try_loading_setup(".",cfg) ;
   if (config)
      return config ;
   char *homedir = getenv("HOME") ;
   if (homedir && *homedir)
      {
      config = try_loading_setup(homedir,"." DEFAULT_WORDCLUS_CFGFILE) ;
      if (config)
	 return config ;
      }
   char *dir = FrFileDirectory(argv0) ;
   if (dir)
      {
      config = try_loading_setup(dir,cfg) ;
      FrFree(dir) ;
      }
   return config ;
}

//----------------------------------------------------------------------

const WcConfig *get_WordClus_config()
{
   return wc_config ;
}

//----------------------------------------------------------------------

void unload_WordClus_config()
{
   delete wc_config ;
   wc_config = nullptr ;
   return ;
}

// end of file wcconfig.C //

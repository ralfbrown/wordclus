/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wordclus.cpp	      word clustering (main program)		*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2005,2006,2009,2010,2015	*/
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
#  include <cmath>
#  include <cstdlib>
#  include <fstream>
#else
#  include <fstream.h>
#  include <math.h>
#  include <stdlib.h>
#endif

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define DEFAULT_THRESHOLD 0.25

#ifndef EXIT_SUCCESS
#  define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#  define EXIT_FAILURE 1
#endif

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

static int phrase_size = 1 ;
static double min_phrase_MI = 0.05 ;
static int mono_skip = 0 ;

/************************************************************************/
/************************************************************************/

static void banner(ostream &out)
{
   out << "Bilingual Word Clustering v" WORDCLUST_VERSION 
          "    Copyright 1999-2002,...,2010,2015 Ralf Brown/Carnegie Mellon Univ."
       << endl ;
   return ;
}

//----------------------------------------------------------------------

static void usage(const char *argv0)
{
   cerr << "Usage: " << argv0 << " [options] outfile dictionary [infile-list]\n"
	   "Options:\n"
           "\t=FILE\tload configuration info from FILE\n"
           "\t-#N\tgenerate no more than N clusters (default 2000)\n"
           "\t-1\tinput files are monolingual\n"
           "\t-a\tremove ambiguous terms from clusters\n"
	   "\t-A#\tallow at most # possibilities for building a word pair\n"
           "\t-bX\tweight sentence boundary by factor of X\n"
           "\t-cmNAME\tuse clustering measure NAME (COS,EUCL,JACC,DICE,...)\n"
           "\t-crNAME\tuse cluster representative NAME (CENT,NEAR,AVG,RMS,...)\n"
           "\t-ctNAME\tuse clustering type NAME (INCR,INCR2,AGG,TIGHT,...)\n"
           "\t-C+\tadd cluster sizes when merging, instead of using larger\n"
           "\t-d\tdecay weights inversely based on distance from key word\n"
	   "\t-ddX\tdiscount context frequencies by raising to power X (0..1)\n"
           "\t-dfX,Y\tdecay weights based on inverse term freq of context words\n"
           "\t-deX\tdecay weights exponentially based on dist from key word\n"
           "\t-dl\tdecay weights linearly based on distance from key word\n"
           "\t-dwX,Y\tdecay weights based on context word frequencies\n"
           "\t-eFILE\tload initial equivalence classes from FILE\n"
           "\t-e:FILE\tload initial equiv classes from FILE, ignoring auto clusters\n"
           "\t-e=FILE\tuse equivalence classes from FILE for context only\n"
           "\t-EFILE\twrite resulting equivalence classes to FILE\n"
           "\t-fN\tdon't try to cluster words occurring less than N times\n"
           "\t-f@N\tcluster only the N most frequent words\n"
           "\t-f-N\tdon't try to cluster words occurring more than N times\n"
           "\t-f-@N\ttreat the N most frequent words as stopwords\n"
           "\t-i\tdon't preserve case of input text\n"
           "\t-l\tforce target-language output to lower case\n"
           "\t-ls\ttoggle downcasing source-language input before processing\n"
	   "\t\t\t(default is enabled)\n"
           "\t-lt\ttoggle downcasing target-language input before processing\n"
           "\t-m\tshow memory usage\n"
           "\t-nN\tuse 'neighborhood' of +/- N (0-9) words as context\n"
           "\t-ntN\tuse target-lang 'neighborhood' of +/- N words as context\n"
           "\t-N\tkeep numbers in a distinct cluster\n"
           "\t-oN\tword-order similarity: allow N-word offset in positions\n"
           "\t-OFILE\toutput clusters to FILE as tagged EBMT corpus\n"
           "\t-pN,M\tcluster phrases up to length N (1-9) with MutInfo >= M\n"
           "\t-P\tkeep punctuation distinct from non-punctuation\n"
           "\t-r\treverse direction by swapping source/target languages\n"
           "\t-s\tuse standard input as corpus\n"
           "\t-SFILE\tread stopwords (for clustering) from FILE\n"
           "\t-tX\tset clustering threshold to X (0.0-1.0)\n"
           "\t-t=FILE\tset variable clustering thresholds from FILE\n"
           "\t-TFILE\tcopy equiv classes from FILE to -E output file\n"
           "\t-Ux\tuse character set 'x' (Latin-1, Latin-2, GB-2312, EUC, etc.)\n"
           "\t-Ub\tbyte-swap characters when using Unicode character set\n"
	   "\t-v\trun verbosely\n"
//           "\t-wFILE\tload TF*IDF weights from FILE\n"
           "\t-W\ttreat contexts as whole phrases rather than bags of words\n"
           "\t-WW\ttreat left and right contexts as a single phrase with a gap\n"
	   "\t-xn\texclude numbers (digit strings) from clustering\n"
	   "\t-xp\texclude punctuation from clustering\n"
	<< endl ;
   exit(EXIT_FAILURE) ;
   return ;
}

//----------------------------------------------------------------------

static void set_flag_var(bool &flag, char option)
{
   if (option == '+')
      flag = true ;
   else if (option == '-')
      flag = false ;
   else
      flag = !flag ;
   return ;
}

//----------------------------------------------------------------------

static void make_seeds(const char *seed_class_file, FrObjHashTable *seeds)
{
   if (seed_class_file && seeds)
      {
      size_t count(0) ;
      FILE *fp = fopen(seed_class_file,"r") ;
      FrSymbol *symEOF = makeSymbol("*EOF*") ;
      if (fp)
	 {
	 while (!feof(fp))
	    {
	    char line[FrMAX_LINE] ;
	    if (!fgets(line,sizeof(line),fp))
	       break ;
	    char *lineptr = line ;
	    FrObject *o1 = string_to_FrObject(lineptr) ;
	    FrObject *o2 = string_to_FrObject(lineptr) ;
	    FrObject *o3 = string_to_FrObject(lineptr) ;
	    if (o1 && o1->printableName() &&
		o2 && o2->printableName() && o3 != symEOF)
	       {
	       FrSymbol *word1 = FrCvt2Symbol(o1) ;
	       FrSymbol *classname = FrCvt2Symbol(o2) ;
	       FrSymbol *word2 = nullptr ;
	       if (o3 && o3->printableName() && !monolingual)
		  word2 = FrCvt2Symbol(o3) ;
	       FrString *keystr = new FrString(word1->printableName()) ;
	       if (word2)
		  {
		  (*keystr) += "\t" ;
		  (*keystr) += word2->printableName() ;
		  }
	       if (seeds->add(keystr,classname))
		  {
		  delete keystr ;	// already in hash table
		  }
	       count++ ;
	       }
	    free_object(o1) ;
	    free_object(o2) ;
	    free_object(o3) ;
	    }
	 fclose(fp) ;
	 }
      if (verbose)
	 cout << ";[ "<< count <<" word pairs have initial classes assigned ]"
	      << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static void extract_desired_clusters(const char *options,
				     size_t &desired_clusters,
				     size_t &backoff_step)
{
   char *end = nullptr ;
   if (*options)
      {
      size_t clusters = (size_t)strtol(options,&end,10) ;
      if (end && end != options)
	 {
	 desired_clusters = clusters ;
	 if (*end == ',')
	    {
	    options = end+1 ;
	    size_t backoff = (size_t)strtol(options,&end,10) ;
	    if (backoff)
	       backoff_step = backoff ;
	    }
	 }
      }
   else
      cout << "Usage for -#:   -#<num_clusters>[,<backoff_step>]"
	   << endl ;
   return ;
}

//----------------------------------------------------------------------

static void extract_neighborhood_size(const char *options)
{
   if (*options == 't')
      {
      options++ ;
      if (Fr_isdigit(*options) && *options != '0')
	 {
	 tl_context_l = *options - '0' ;
	 tl_context_r = tl_context_l ;
	 ++options ;
	 if (*options == ',')
	    {
	    options++ ;
	    if (Fr_isdigit(*options) && *options != '0')
	       tl_context_r = *options - '0' ;
	    }
	 }
      }
   else
      {
      if (Fr_isdigit(*options) && *options != '0')
	 {
	 neighborhood_left = *options - '0' ;
	 neighborhood_right = neighborhood_left ;
	 ++options ;
	 if (*options == ',')
	    {
	    options++ ;
	    if (Fr_isdigit(*options) && *options != '0')
	       neighborhood_right = *options - '0' ;
	    }
	 }
      }
   if (tl_context_l > neighborhood_left)
      {
      FrWarning("target-language left context may not be larger than the\n"
		"\tsource-language left context; adjusted") ;
      tl_context_l = neighborhood_left ;
      }
   return ;
}

//----------------------------------------------------------------------

static void extract_null_weight(const char *option)
{
   char *end = nullptr ;
   double value = strtod(option,&end) ;
   if (end && end != option && value >= 0.0 && value <= 2.0)
      {
      past_boundary_weight = value ;
      return ;
      }
   cerr << "; You must specify a weight between 0.0 and 2.0 for the -b option"
	<< endl ;
   return ;
}

//----------------------------------------------------------------------

static void extract_phrase_limits(const char *option)
{
   if (Fr_isdigit(*option) && *option != '0')
      {
      phrase_size = *option++ - '0' ;
      if (*option == ',')
	 {
	 option++ ;
	 char *end = nullptr ;
	 double min = strtod(option,&end) ;
	 if (end && end != option && min >= 0.0 && min <= 1.0)
	    min_phrase_MI = min ;
	 else
	    {
	    cerr << "; You must specify a threshold between 0.0 and 1.0 for the -p option."
		 << endl
		 << "; Using the default of " << min_phrase_MI << endl ;
	    }
	 }
      }
   return ;
}

//----------------------------------------------------------------------

static void load_stopwords(const char *stopwords_file)
{
   if (stopwords_file && *stopwords_file)
      {
#if __GNUC__ == 3 && __GNUC_MINOR_ < 10
      // !*(@$_%( broken libstdc++   (ifstream::eof() is always false!)
      FILE *sw = fopen(stopwords_file,"r") ;
      if (sw)
	 {
	 size_t count(0) ;
	 while (!feof(sw))
	    {
	    char buffer[FrMAX_LINE] ;
	    buffer[0] = '\0' ;
	    if (!fgets(buffer,sizeof(buffer),sw))
	       break ;
	    if (buffer[0])
	       {
	       char *bufptr = buffer ;
	       FrObject *obj = string_to_FrObject(bufptr) ;
	       FrSymbol *word = FrCvt2Symbol(obj) ;
	       if (word && word != makeSymbol("*EOF*"))
		  {
		  word->defineRelation(word) ;
		  count++ ;
		  }
	       free_object(obj) ;
	       }
	    }
	 cout << ";[ loaded " << count << " stopwords from "
	      << stopwords_file << " ]" << endl ;
	 }
#else
      ifstream sw(stopwords_file) ;
      if (sw.good())
	 {
	 sw.clear() ;
	 size_t count(0) ;
	 while (!sw.eof())
	    {
	    FrObject *obj ;
	    sw >> obj ;
	    if (obj)
	       {
	       FrSymbol *word = FrCvt2Symbol(obj) ;
	       if (word)
		  {
		  word->defineRelation(word) ;
		  count++ ;
		  }
	       if (obj == makeSymbol("*EOF*"))
		  break ;
	       free_object(obj) ;
	       }
	    }
	 cout << "; loaded " << count << " stopwords from "
	      << stopwords_file << endl ;
	 }
#endif /* __GNUC__ */
      }
   return ;
}

//----------------------------------------------------------------------

int main(int argc, char **argv)
{
   banner(cout) ;
   // process the commandline arguments
   const char *argv0 = argv[0] ;	// remember program name
   bool use_stdin = false ;
   char *weights_file = nullptr ;
   char *seed_class_file = nullptr ;
   const char *stopwords_file = nullptr ;
   const char *token_file = nullptr ;
   const char *input_token_file = nullptr;
   const char *output_corpus_file = nullptr ;
   double threshold = DEFAULT_THRESHOLD ;
   size_t desired_clusters = 2000 ;
   size_t backoff_step = 5 ;
   initialize_FramepaC() ;
   FrSymbolTable *sym_t = FrSymbolTable::selectDefault() ;
   sym_t->expand(300000L) ;		// reduce memory fragmentation
   sym_t->select() ;
   WcSetCharEncoding("Latin1") ;
   // load in the configuration file, if one is specified
   for (int arg = 1 ; arg < argc ; arg++)
      {
      if (argv[arg][0] == '=')
	 {
	 WcConfig *config = get_WordClus_setup_file(argv[arg]+1,argv[0],cerr) ;
	 if (!config)
	    exit(1) ;
	 apply_WordClus_configuration(config) ;
	 }
      else if (argv[arg][0] != '-')
	 break ;
      }
   // next, handle all the other commandline flags (this allows the
   //   commandline to override things in the configuration file)
   for ( ;
	argc > 1 && (argv[1][0] == '-' || argv[1][0] == '=') ;
	argc--, argv++)
      {
      if (argv[1][0] == '=')
	 continue ;			// already handled
      char option = argv[1][2] ;
      switch (argv[1][1])
	 {
	 case '#':
	    extract_desired_clusters(argv[1]+2,desired_clusters,backoff_step) ;
	    break ;
	 case '1':
	    monolingual = true ;
	    if (option == 's')
	       mono_skip = +1 ;
	    else if (option == 't')
	       mono_skip = -1 ;
	    break ;
	 case '2':
	    two_pass_clustering = true ;
	    break ;
	 case 'a':
	    remove_ambig_terms = true ;
	    break ;
	 case 'A':
	    max_wordpair_ambig = atoi(argv[1]+2) ;
	    if (max_wordpair_ambig < 1)
	       max_wordpair_ambig = 1 ;
	    break ;
	 case 'b':
	    extract_null_weight(argv[1]+2) ;
	    break ;
	 case 'c':
	    if (option == 'm')
	       clustering_metric = FrParseClusterMetric(argv[1]+3,&cerr) ;
	    else if (option == 'r')
	       clustering_rep = FrParseClusterRep(argv[1]+3,&cerr) ;
	    else if (option == 't')
	       clustering_method = FrParseClusterMethod(argv[1]+3,&cerr) ;
	    else if (option == 'i')
	       clustering_iter = atoi(argv[1]+3) ;
	    else if (option == 'p')
	       clustering_settings = argv[1]+3 ;
	    break ;
	 case 'C':
	    sum_cluster_sizes = (option == '+') ;
	    break ;
	 case 'd':
	    if (option == 'd')
	       {
	       char *end = nullptr ;
	       double disc = strtod(argv[1]+3,&end) ;
	       if (end != argv[1]+3 && disc > 0.0 && disc <= 2.0)
		  termfreq_discount = disc ;
	       else
		  cerr << "Warning: -ddX requires 0.0 < X <= 2.0; using default of " << termfreq_discount << endl ;
	       }
	    else if (option == 'w')
	       {
	       // set up the word-frequency-based weighting
	       //  beta is how fast the weight decays with increasing freq
	       //  gamma is the saturation point (minimum weight)
	       char *end = nullptr ;
	       decay_beta = strtod(argv[1]+3,&end) ;
	       if (end != argv[1]+3 && *end == ',')
		  {
		  decay_gamma = strtod(end+1,0) ;
		  if (decay_gamma < 0.0)
		     decay_gamma = 0.0 ;
		  }
	       }
	    else if (option == 'f')
	       {
	       // set up the inverse-term-frequency weighting
	       char *end = nullptr ;
	       double decay = strtod(argv[1]+3,&end) ;
	       if (end != argv[1]+3)
		  {
		  if (decay < 0.01) decay = 0.01 ;
		  decay_beta = -decay ;
		  }
	       else
		  decay_beta = -1.0 ;
	       }
	    else
	       {
	       switch (option)
		  {
		  case 'l':	decay_type = Decay_Linear ;	 break ;
		  case 'e':	decay_type = Decay_Exponential ; break ;
		  case 'n':	decay_type = Decay_None ;	 break ;
		  default:	decay_type = Decay_Reciprocal ;	 break ;
		  }
	       if (decay_type == Decay_Exponential)
		  {
		  // alpha controls how fast the weight decays with distance
		  decay_alpha = strtod(argv[1]+3,0) ;
		  if (decay_alpha <= 0.0)
		     decay_alpha = 0.5 ;
		  }
	       }
	    break ;
	 case 'e':
	    if (option == '=')
	       {
	       use_seeds_for_context_only = true ;
	       seed_class_file = argv[1]+3 ; // use FILE for context equivs
	       }
	    else if (option == ':')
	       {
	       ignore_auto_clusters = true ;
	       seed_class_file = argv[1]+3 ; // use FILE for context equivs
	       }
	    else
	       seed_class_file = argv[1]+2 ; // load initial classes from FILE
	    break ;
	 case 'E':
	    token_file = argv[1]+2 ;
	    break ;
	 case 'f':
	    if (option == '@')
	       max_term_count = atoi(argv[1]+3) ;
	    if (option == '-')
	       {
	       if (argv[1][3] == '@')
		  stop_term_count = atoi(argv[1]+4) ;
	       else
		  max_frequency = atoi(argv[1]+3) ;
	       }
	    else
	       min_frequency = atoi(argv[1]+2) ;
	    break ;
	 case 'i':
	    lowercase_source = true ;
	    break ;
         case 'l':
	    if (option == 's')
	       set_flag_var(lowercase_source,argv[1][3]) ;
	    else if (option == 't')
	       set_flag_var(lowercase_target,argv[1][3]) ;
	    else if (!option)
	       lowercase_output = true ;
	    break ;
	 case 'm':
	    showmem = true ;
	    break ;
	 case 'n':
	    extract_neighborhood_size(argv[1]+2) ;
	    break ;
	 case 'N':
	    keep_numbers_distinct = true ;
	    break ;
	 case 'o':
	    if (Fr_isdigit(option))
	       word_order_similarity = option - '0' ;
	    break ;
	 case 'O':
	    output_corpus_file = argv[1]+2 ;
	    break ;
	 case 'p':
	    extract_phrase_limits(argv[1]+2) ;
	    break ;
	 case 'P':
	    keep_punctuation_distinct = true ;
	    break ;
	 case 'r':
	    set_flag_var(reverse_languages,option) ;
	    break ;
         case 's':
	    use_stdin = true ;
	    break ;
	 case 'S':
	    stopwords_file = argv[1]+2 ;
	    break ;
	 case 't':
	    if (option == '=')
	       threshold_file = argv[1]+3 ;
	    else
	       threshold = atof(argv[1]+2) ;
	    break ;
	 case 'T':
	    if (option == '-')
	       {
	       reverse_tokens = true ;
	       input_token_file = argv[1]+3 ;
	       }
	    else
	       input_token_file = argv[1]+2 ;
	    break ;
	 case 'v':
	    verbose = true ;
	    break ;
	 case 'w':
	    weights_file = argv[1]+2 ;
	    break ;
	 case 'W':
	    use_phrasal_context = true ;
	    if (option == 'W')
	       single_phrase_context = true ;
	    break ;
	 case 'x':
	    if (option == 'p')
	       exclude_punctuation = true ;
	    else if (option == 'n')
	       exclude_numbers = true ;
	    break ;
	 case 'U':
	    if (option == 'b' && argv[1][3] == '\0')
	       set_flag_var(Unicode_bswap,argv[1][3]) ;
	    else
	       WcSetCharEncoding(argv[1]+2) ;
	    char_encoding = WcCurrentCharEncoding() ;
	    break ;
	 default:
	    cerr << "Unrecognized option " << argv[1] << endl ;
	    usage(argv0) ;
	    break ;
	 }
      }
   if (argc < 3)
      usage(argv0) ;
   const char *output_file = argv[1] ;
   FILE *out_file = nullptr ;
   if (output_file && *output_file)
      out_file = fopen(output_file,"w") ;
   else
      {
      FrError("you must specify an output file!") ;
      return EXIT_FAILURE ;
      }
   FILE *tok_file = nullptr ;
   if (token_file && *token_file)
      tok_file = fopen(token_file,"w") ;
   load_stopwords(stopwords_file) ;
   FILE *tagged_file = nullptr ;
   if (output_corpus_file && *output_corpus_file)
      tagged_file = fopen(output_corpus_file,"w") ;
   // configure library
   FrObjHashTable *seeds = nullptr ;
   if (seed_class_file && *seed_class_file)
      {
      cout << ";[ loading initial equivalences from " << seed_class_file
	   << " ]" << endl ;
      seeds = new FrObjHashTable(8000) ;
#if 1
      make_seeds(seed_class_file,seeds) ;
#else
      Tokenizer *equivalences = new Tokenizer(seed_class_file,
					      Tokenizer::keepall) ;
      make_seeds(equivalences,seeds) ;
      delete equivalences ;
#endif
      }
   FrThresholdList threshold_list(threshold_file,threshold) ;
   (void)weights_file;//keep compiler happy
//!!! WcLoadTermWeights(weights_file) ;
   // and finally, run the clustering or grammar generation
   FrTimer timer ;
   WcParameters params(min_frequency,max_frequency,max_term_count,
		       stop_term_count,neighborhood_left,neighborhood_right,
		       tl_context_l,tl_context_r,phrase_size,min_phrase_MI,0) ;
   params.downcaseSource(lowercase_source) ;
   params.downcaseTarget(lowercase_target) ;
   params.equivalenceClasses(seeds) ;
   params.equivClassFile(input_token_file) ;
   params.desiredClusters(desired_clusters) ;
   params.backoffStep(backoff_step) ;
   WcProcessCorpusFiles(argv[3],use_stdin,out_file,tok_file,tagged_file,&params,
			&threshold_list) ;
   // clean up
   if (out_file)
      fclose(out_file) ;
   delete seeds ;
   cout << ";[ Total run time was " << timer.readsec() << " seconds ]" << endl;
   unload_WordClus_config() ;
   if (showmem)
      {
      FrMemoryStats(cerr) ;
      if (verbose)
	 {
	 wc_vars.freeVariables() ;
	 FramepaC_gc() ;
	 FrMemoryStats(cerr) ;
	 FrString::dumpUnfreed(cerr) ;
	 }
      }
   return EXIT_SUCCESS ;
}

// end of file wordclus.cpp //

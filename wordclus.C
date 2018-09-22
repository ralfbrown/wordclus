/************************************************************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wordclus.cpp	      word clustering (main program)		*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2005,2006,2009,2010,2015,	*/
/*		2016,2017,2018 Carnegie Mellon University		*/
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

#include <cmath>
#include <cstdlib>
#include <fstream>

#include "framepac/argparser.h"
#include "framepac/memory.h"
#include "framepac/message.h"
#include "framepac/stringbuilder.h"
#include "framepac/symboltable.h"
#include "framepac/timer.h"

#include "wordclus.h"
#include "wcparam.h"

using namespace Fr ;

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

static const char* input_token_file = nullptr;
static const char* output_corpus_file = nullptr ;
static const char* seed_class_file = nullptr ;
static const char* context_equiv_file = nullptr ;
static size_t desired_clusters = 2000 ;
static size_t backoff_step = 5 ;

static WcParameters params ;

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

#if 0
   cerr << "Usage: " << argv0 << " [options] outfile dictionary [infile-list]\n"
	   "Options:\n"
           "\t-d\tdecay weights inversely based on distance from key word\n"
           "\t-dfX,Y\tdecay weights based on inverse term freq of context words\n"
           "\t-deX\tdecay weights exponentially based on dist from key word\n"
           "\t-dl\tdecay weights linearly based on distance from key word\n"
           "\t-dwX,Y\tdecay weights based on context word frequencies\n"
           "\t-ntN\tuse target-lang 'neighborhood' of +/- N words as context\n"
	<< endl ;
}
#endif /* 0 */

//----------------------------------------------------------------------

static void make_seeds(const char *seed_class_file, SymHashTable *seeds, bool run_verbosely)
{
   if (seed_class_file && seeds)
      {
      size_t count(0) ;
      CInputFile fp(seed_class_file) ;
      if (fp)
	 {
	 SymbolTable* symtab = SymbolTable::current() ;
	 while (CharPtr line { fp.getCLine() })
	    {
	    char* lineptr = *line ;
	    ScopedObject<> o1(lineptr) ;
	    ScopedObject<> o2(lineptr) ;
	    if (o1 && o1->printableName() &&
		o2 && o2->printableName())
	       {
	       Symbol* key = symtab->add(o1->printableName()) ;
	       Symbol* classname = symtab->add(o2->printableName()) ;
	       (void)seeds->add(key,classname) ;
	       count++ ;
	       }
	    }
	 }
      if (run_verbosely)
	 cout << ";[ "<< count <<" word pairs have initial classes assigned ]"
	      << endl ;
      }
   return ;
}

//----------------------------------------------------------------------

static bool extract_desired_clusters(const char *options)
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
   return true ;
}

//----------------------------------------------------------------------

static bool extract_neighborhood_size(const char *options)
{
   if (isdigit(*options) && *options != '0')
      {
      params.neighborhoodLeft(*options - '0') ;
      params.neighborhoodRight(params.neighborhoodLeft()) ;
      ++options ;
      if (*options == ',')
	 {
	 options++ ;
	 if (isdigit(*options) && *options != '0')
	    params.neighborhoodRight(*options - '0') ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

static bool set_cluster_metric(const char* opt)
{
   params.clusteringMeasure(opt) ;
   return true ;
}

//----------------------------------------------------------------------

static bool set_cluster_rep(const char* opt)
{
   params.clusteringRep(opt) ;
   return true ;
}

//----------------------------------------------------------------------

static bool set_cluster_method(const char* opt)
{
   //TODO: validate that the named method exists
   params.clusteringMethod(opt) ;
   return true ;
}

//----------------------------------------------------------------------

static bool extract_distance_decay(const char *option)
{
   if (*option == 'w')
      {
      // set up the word-frequency-based weighting
      //  beta is how fast the weight decays with increasing freq
      //  gamma is the saturation point (minimum weight)
      char *end = nullptr ;
      params.m_decay_beta = strtod(option+1,&end) ;
      if (end != option+1 && *end == ',')
	 {
	 params.m_decay_gamma = strtod(end+1,0) ;
	 if (params.m_decay_gamma < 0.0)
	    params.m_decay_gamma = 0.0 ;
	 }
      }
   else if (*option == 'f')
      {
      // set up the inverse-term-frequency weighting
      char *end = nullptr ;
      double decay = strtod(option+1,&end) ;
      if (end != option+1)
	 {
	 if (decay < 0.01) decay = 0.01 ;
	 params.m_decay_beta = -decay ;
	 }
      else
	 params.m_decay_beta = -1.0 ;
      }
   else if (*option != 'd')
      {
      switch (*option)
	 {
	 case 'l':	params.m_decay_type = Decay_Linear ;	 break ;
	 case 'e':	params.m_decay_type = Decay_Exponential ; break ;
	 case 'n':	params.m_decay_type = Decay_None ;	 break ;
	 default:	params.m_decay_type = Decay_Reciprocal ;	 break ;
	 }
      if (params.m_decay_type == Decay_Exponential)
	 {
	 // alpha controls how fast the weight decays with distance
	 params.m_decay_alpha = strtod(option+1,0) ;
	 if (params.m_decay_alpha <= 0.0)
	    params.m_decay_alpha = 0.5 ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

static bool extract_seed_file(const char* opt)
{
   if (opt[0] == ':')
      {
      params.ignoreAutoClusters(true) ;
      opt++ ;
      }
   seed_class_file = opt ; // use FILE for context equivs
   return true ;
}

//----------------------------------------------------------------------

static bool extract_phrase_limits(const char *option)
{
   if (isdigit(*option) && *option != '0')
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
   return true ;
}

//----------------------------------------------------------------------

static bool extract_unicode_options(const char* opt)
{
   WcSetCharEncoding(opt) ;
   return true ;
}

//----------------------------------------------------------------------

int main(int argc, char **argv)
{
   const char* clustering_settings { nullptr } ;
   size_t clustering_iter { 5 } ;
   size_t min_frequency   { 2 } ;
   size_t max_frequency   { ULONG_MAX } ;
   size_t max_term_count  { ULONG_MAX } ;
   size_t stop_term_count { 0 } ;
   bool lowercase_output  { false } ;
   bool lowercase_source  { false } ;
   bool exclude_numbers   { false } ;
   bool exclude_punct     { false } ;
   bool verbose           { false } ;
   bool showmem           { false } ;

   banner(cout) ;
   // process the commandline arguments
   const char* weights_file = nullptr ;
   const char* stopwords_file = nullptr ;
   const char* token_file = nullptr ;
   double threshold = DEFAULT_THRESHOLD ;
   Fr::Initialize() ;
   WcSetCharEncoding("en_US.iso8859-1") ;

   ArgParser cmdline_flags ;
   cmdline_flags
      .addFunc(extract_desired_clusters,"#","numclusters","N[,B]\vgenerate no more than N clusters (default 200)\nback off threshold by B if less than N")
      .add(mono_skip,"1s","use-source","extract source-language lines from bilingual input",+1)
      .add(mono_skip,"1t","use-target","extract target-language lines from bilingual input",-1)
      .add(params.m_past_boundary_weight,"b","nullweight","X\vweight sentence boundary by factor of X",0.0,2.0)
      .addFunc(set_cluster_metric,"cm","","METRIC\vselect clustering measure METRIC (COS,EUCL,JACC,DICE,...)")
      .addFunc(set_cluster_rep,"cr","","REP\vselect clustering representative REP (CENT,NEAR,AVG,RMS,...)")
      .addFunc(set_cluster_method,"ct","","METH\vselect clustering method METH (INCR,AGG,TIGHT,...)")
      .add(clustering_iter,"ci","cluster-iter","N\vset maximum number of clustering iterations to N")
      .add(clustering_settings,"cp","cluster-params","X\vset optional clustering parameter(s) to X")
      .add(params.m_termfreq_discount,"dd","","X\vdiscount context frequencies by raising to power X",0.0,2.0)
      .addFunc(extract_distance_decay,"d","","-deX decay weights exponentially, -dfX,Y, -dl, -dwX -d")
      .add(context_equiv_file,"e=","","FILE\vuse equivalence classes from FILE for context only")
      .addFunc(extract_seed_file,"e","","FILE\vload initial equiv classes from FILE (ignore auto clusters if :FILE)")
      .add(token_file,"E","","FILE\vwrite resulting equivalence classes to FILE")
      .add(max_term_count,"f@","maxterms","N\vcluster only the N most frequent terms")
      .add(max_frequency,"f-","maxfreq","N\vdon't try to cluster terms occurrent more than N times")
      .add(stop_term_count,"f-@","stopcount","N\vtreat the N most frequent terms as stopwords")
      .add(min_frequency,"f","minfreq","N\vdon't try to cluster terms occurring less than N times")
      .add(lowercase_source,"i","","ignore input case (lowercase input)")
      .add(lowercase_output,"l","","force output to lowercase")
      .add(showmem,"m","showmem","show memory usage")
      .addFunc(extract_neighborhood_size,"n","","N\vuse 'neighborhood' of +/- N (0-9) words as context")
      .add(params.m_distinct_numbers,"N","sep-numbers","put numbers in separate clusters")
      .add(output_corpus_file,"O","output","FILE\voutput clusters to FILE as tagged EBMT corpus")
      .addFunc(extract_phrase_limits,"p","","N,M\vcluster phrsaes up to length N (1-9) with mutualinfo >= M")
      .add(params.m_distinct_punct,"P","sep-punct","put punctuation in separate clusters")
      .add(stopwords_file,"S","stopwords","FILE\vread stopwords (for clustering) from FILE")
      .add(threshold,"t","","X\vset clustering threshold to X (0.0-1.0)",0.0,1.0)
      .add(input_token_file,"T","","FILE\vcopy equiv classes from FILE to -E output file")
      .addFunc(extract_unicode_options,"U","","x\vuse character set 'x' (Latin-1, Latin-2, GB-2312, EUC, etc.)")
      .add(verbose,"v","verbose","run verbosely")
      .add(weights_file,"w","","FILE\vload TF*IDF weights from FILE")
      .add(exclude_numbers,"xn","nonumbers","exclude numbers from clustering")
      .add(exclude_punct,"xp","nopunct","exclude punctuation from corpus")
      .addHelp("h","","show this usage summary") ;
   if (!cmdline_flags.parseArgs(argc,argv) || argc < 3)
      {
      cmdline_flags.showHelp() ;
      return 1 ;
      }
   params.runVerbosely(verbose) ;
   params.showMemory(showmem) ;
   WcLowercaseOutput(lowercase_output) ;

   const char *output_file = argv[1] ;
   if (!output_file || *output_file)
      {
      SystemMessage::error("you must specify an output file!") ;
      return EXIT_FAILURE ;
      }
   COutputFile out_file(output_file) ;
   COutputFile tok_file(token_file) ;
   COutputFile tagged_file(output_corpus_file) ;
   // configure library
   ScopedObject<SymHashTable> seeds(4000) ;
   if (seed_class_file && *seed_class_file)
      {
      cout << ";[ loading initial equivalences from " << seed_class_file
	   << " ]" << endl ;
      make_seeds(seed_class_file,seeds,params.runVerbosely()) ;
      }
   (void)weights_file;//keep compiler happy
//!!! WcLoadTermWeights(weights_file) ;
   // and finally, run the clustering or grammar generation
   Timer timer ;
   params.iterations(clustering_iter) ;
   params.clusteringSettings(clustering_settings) ;
   params.minWordFreq(min_frequency) ;
   params.maxWordFreq(max_frequency) ;
   params.maxTermCount(max_term_count) ;
   params.stopTermCount(stop_term_count) ;
   params.phraseLength(phrase_size) ;
   params.miThreshold(min_phrase_MI) ;
   params.downcaseSource(lowercase_source) ;
   params.equivalenceClasses(seeds) ;
   params.stopwordsFile(stopwords_file) ;
   params.contextEquivClassFile(context_equiv_file) ;
   params.equivClassFile(input_token_file) ;
   params.desiredClusters(desired_clusters) ;
   params.backoffStep(backoff_step) ;
   params.excludeNumbers(exclude_numbers) ;
   params.excludePunctuation(exclude_punct) ;
   WordCorpus *corpus = load_or_generate_corpus(argv[3],&params) ;
   if (corpus)
      {
      VectorMeasure<WcWordCorpus::ID,float>* measure = nullptr ; //TODO
      WcProcessCorpus(corpus,measure,out_file,tok_file,tagged_file,&params,
		      output_file,token_file,output_corpus_file) ;
      }
   // clean up
   seeds = nullptr ;
   cout << ";[ Total run time was " << timer << " ]" << endl;
   if (params.showMemory())
      {
      Fr::memory_stats(cerr) ;
      if (params.runVerbosely())
	 {
	 Fr::gc() ;
	 Fr::memory_stats(cerr) ;
	 }
      }
   return EXIT_SUCCESS ;
}

// end of file wordclus.cpp //

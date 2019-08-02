/****************************** -*- C++ -*- *****************************/
/*									*/
/*  wordclus -- gencorpus: generate an indexed array of word IDs	*/
/*  Version 2.00							*/
/*	by Ralf Brown							*/
/*									*/
/*  File: gencorpus.C							*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 2016,2017,2018 Carnegie Mellon University		*/
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

#include "framepac/progress.h"
#include "framepac/string.h"
#include "framepac/symboltable.h"
#include "framepac/texttransforms.h"
#include "framepac/threadpool.h"
#include "framepac/timer.h"
#include "framepac/words.h"

#include "wordclus.h"
#include "wcbatch.h"
#include "wcparam.h"

using namespace Fr ;

List *remove_punctuation(List *wordlist) ;

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define DOT_INTERVAL 10000

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

static Ptr<ProgressIndicator> progress ;
static bool use_bytes ;

/************************************************************************/
/*	Global Data							*/
/************************************************************************/

/************************************************************************/
/************************************************************************/

void init_corpus_parsing(const char delim[256])
{
   WcSetCharEncoding("en_US.iso8859-1") ;
   WcSetWordDelimiters(delim) ;
   return ;
}

//----------------------------------------------------------------------

List *load_file_list(const char *listfile)
{
   ListBuilder files ;
   CInputFile fp(listfile) ;
   if (fp)
      {
      while (auto line = fp.getTrimmedLine())
	 {
	 if (**line && **line != '#' && **line != ';')
	    files += String::create(line) ;
	 }
      }
   else if (listfile)
      {
      cerr << "Unable to open '" << listfile << "' for reading!\n" ;
      }
   return files.move() ;
}

//----------------------------------------------------------------------

WcWordCorpus* new_corpus(const WcParameters* params, const char *filename)
{
   Owned<WcWordCorpus> corpus ;
   if (!corpus || (filename && !corpus->load(filename)))
      {
      return nullptr ;
      }
   if (params)
      {
      if (params->autoNumbers())
	 corpus->setNumberToken() ;
      if (params->rareThreshold())
	 corpus->rareWordThreshold(params->rareThreshold(),"<rare>") ;
      corpus->setContextSizes(params->neighborhoodLeft(),params->neighborhoodRight()) ;
      }
   return corpus.move() ;
}

//----------------------------------------------------------------------

static bool load_corpus_line(char *line, WcWordCorpus *corpus, bool downcase, bool no_punct)
{
   if (line && *line)
      {
      if (downcase)
	 {
	 std::locale* encoding = WcCurrentCharEncoding() ;
	 lowercase_string(line,encoding) ;
	 }
      WordSplitterEnglish splitter(line,WcWordDelimiters()) ;
      Ptr<List> words(splitter.allWords()) ;
      if (no_punct)
	 {
	 words = remove_punctuation(words.move()) ;
	 }
      size_t numwords = words->size() ;
      if (numwords == 0)
	 return true ;
      // convert the words into wordIDs
      WcWordCorpus::ID newline ;
      auto wordnum = corpus->reserveIDs(numwords+1,&newline) ;
      bool auto_numbers = corpus->numberToken() != WcWordCorpus::ErrorID ;
      auto symtab = SymbolTable::current() ;
      for (auto w : *words)
	 {
	 auto str = w->stringValue() ;
	 WcWordCorpus::ID id = (auto_numbers && is_number(str))
	    ? corpus->numberToken()
	    : corpus->findOrAddID(symtab->add(str)->c_str()) ;
	 corpus->setID(wordnum,id) ;
	 ++wordnum ;
	 }
      corpus->setID(wordnum,newline) ;
      }
   return true ;
}

//----------------------------------------------------------------------

static bool load_corpus_segment(const LineBatch &lines, const WcParameters *params, va_list /*args*/)
{
   WcWordCorpus *corpus = params->corpus() ;
   for (auto line : lines)
      {
      load_corpus_line((char*)line,corpus,params->downcaseSource(),params->excludePunctuation()) ;
      }
   if (use_bytes)
      progress->incr(lines.inputBytes()) ;
   else
      progress->incr(lines.size()) ;
   return true ;
}

//----------------------------------------------------------------------

static size_t total_file_size(const List* filelist)
{
   size_t total = 0 ;
   if (filelist)
      {
      for (auto f : *filelist)
	 {
	 if (!f)
	    continue ;
	 const char* filename = f->printableName() ;
	 if (!filename || !*filename)
	    continue ;
	 if (strcmp(filename,"-") == 0)
	    return 0 ;   // stdin is typically not seekable, so we can't compute a total size
	 FILE* fp = fopen(filename,"rb") ;
	 if (!fp)
	    return 0 ; 	 // unable to open file, so we can't compute a total size
	 if (fseek(fp,0,SEEK_END) == 0)
	    total += ftell(fp) ;
	 fclose(fp) ;
	 }
      }
   return total ;
}

//----------------------------------------------------------------------

static WcWordCorpus *load_corpus(WcWordCorpus* corpus, const List *filelist,
   const WcParameters* global_params)
{
   if (!corpus)
      return nullptr ;
   Timer timer ;
   size_t bytes = total_file_size(filelist) ;
   use_bytes = bytes > 0 ;
   progress = bytes ? new ConsoleProgressIndicator(1,bytes,50,"; read ",";   ")
      : new ConsoleProgressIndicator(DOT_INTERVAL,0,50,"; read ",";      ") ;
   progress->showElapsedTime(true) ;
   bool run_verbosely = global_params ? global_params->runVerbosely() : false ;
   // ensure that the code to collect left contexts sees a newline
   //   before falling off the start of the corpus
   corpus->addWord(corpus->newlineID()) ;
   // we get excessive contention with a large number of threads, so limit how many
   //   threads will actually be used while loading the corpus
   ThreadPool::defaultPool()->limitThreads(4) ;
   for (const Object* fname : *filelist)
      {
      if (!fname->isString()) continue ;
      const char *filename = static_cast<const String*>(fname)->c_str() ;
      CInputFile fp(filename) ;
      if (fp)
	 {
	 if (run_verbosely)
	    {
	    cout << "\n;  processing " << filename << endl ;
	    if (bytes == 0)
	       cout << ";      " << flush ;
	    }
	 WcParameters params(global_params) ;
	 params.corpus(corpus) ;
	 WcProcessFile(fp,&params,&load_corpus_segment) ;
	 if (run_verbosely) cout << endl ; // terminate line of progress dots
	 }
      else
	 {
	 cout << ";!!   Error opening file '" << filename << "'\n" ;
	 }
      }
   ThreadPool::defaultPool()->unlimitThreads() ;
   progress = nullptr ;
   cout << "; loading corpus data took " << timer << ".\n" ;
   return corpus ;
}

//----------------------------------------------------------------------

WcWordCorpus *load_corpus(const List *filelist, const WcParameters *params)
{
   return load_corpus(new_corpus(params),filelist,params) ;
}

//----------------------------------------------------------------------

bool generate_indices(WcWordCorpus *corpus, bool reverse)
{
   if (!corpus)
      return false ;
   Timer timer ;
   bool success = corpus->createIndex(reverse) ;
   cout << "; creating suffix array for " << corpus->corpusSize() << " tokens took " << timer << endl ;
   return success ;
}

//----------------------------------------------------------------------

WcWordCorpus* load_or_generate_corpus(const char *filename, const WcParameters* params)
{
   WcWordCorpus* corpus ;
   if (WcWordCorpus::isCorpusFile(filename))
      {
      Timer timer ;
      corpus = new_corpus(params,filename) ;
      cout << ";[ loaded corpus of " << corpus->corpusSize() << " tokens in " << timer << " ]\n" ;
      }
   else
      {
      corpus = new_corpus(params) ;
      Ptr<List> file_list(load_file_list(filename)) ;
      corpus = load_corpus(corpus,file_list,params) ;
      if (corpus)
	 {
	 generate_indices(corpus,false/*reverse_index*/) ;
	 }
      }
   if (corpus)
      {
      const char *stopwords_file = params->stopwordsFile() ;
      if (stopwords_file)
	 {
	 size_t count = corpus->loadAttribute(stopwords_file,WcATTR_STOPWORD,true) ;
	 cout << ";[ loaded " << count << " stopwords from " << stopwords_file << " ]\n" ;
	 corpus->setAttribute(corpus->newlineID(),WcATTR_STOPWORD) ;
	 }
      const char *equivs_file = params->contextEquivClassFile() ;
      if (equivs_file)
	 {
	 Timer timer ;
	 bool success = corpus->loadContextEquivs(equivs_file,params->downcaseSource()) ;
	 if (success)
	    {
	    cout << ";[ loaded " << corpus->numContextEquivs() << " context equivalences in " ;
	    cout << timer << " ]\n" ;
	    }
	 else
	    cout << ";[ unable to load context equivalences from " << equivs_file << " ]\n" ;
	 }
      if (params->punctuationAsStopwords())
	 {
	 corpus->setAttributeIf(WcATTR_STOPWORD,is_punct) ;
	 }
      corpus->consolidateContextEquivs() ;
      }
   return corpus ;
}

//----------------------------------------------------------------------

// end of file gencorpus.C //

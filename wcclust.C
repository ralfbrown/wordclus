/************************************************************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcclust.cpp	      term-vector clustering			*/
/*  LastEdit: 19sep2018							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2005,2006,2009,2015,2016,2017,	*/
/*	   2018 Carnegie Mellon University				*/
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

#include <cfloat>
#include "wordclus.h"
#include "wctrmvec.h"
#include "wcparam.h"

#include "framepac/cluster.h"
#include "framepac/hashtable.h"
#include "framepac/message.h"
#include "framepac/symboltable.h"

using namespace Fr ;

/************************************************************************/
/*	Compile-Time Configuration					*/
/************************************************************************/

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

/************************************************************************/
/************************************************************************/

static const char *source_word(Object *key)
{
   const char *text = key ? key->printableName() : nullptr ;
   if (!text) text = "" ;
   return text ;
}

//----------------------------------------------------------------------

static bool contains_digit(const char *word)
{
   if (word)
      {
      for ( ; *word ; word++)
	 {
	 if (isdigit(*word))
	    return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static bool cluster_conflict(const VectorBase *tv1, const VectorBase *tv2,
   bool numbers_distinct, bool punctuation_distinct)
{
   if (numbers_distinct)
      {
      auto cluster1 = tv1->label() ;
      auto cluster2 = tv2->label() ;
      const char* key1 = tv1->key() ? tv1->key()->c_str() : nullptr ;
      const char* key2 = tv2->key() ? tv2->key()->c_str() : nullptr ;
      if (ClusterInfo::isNumberLabel(cluster1) && !cluster2)
	 return (!contains_digit(key2) && !ClusterInfo::isGeneratedLabel(key2)) ;
      else if (ClusterInfo::isNumberLabel(cluster2) && !cluster1)
	 return (!contains_digit(key1) && !ClusterInfo::isGeneratedLabel(key1)) ;
      }
   if (punctuation_distinct && tv1->key() && tv2->key() &&
       is_punct(tv1->key()->c_str()) != is_punct(tv2->key()->c_str()))
      return true ;
   return false ;
}

//----------------------------------------------------------------------

template <typename IdxT, typename ValT>
class VectorMeasurePunctNum : public WrappedVectorMeasure<IdxT,ValT>
   {
   public:
      typedef WrappedVectorMeasure<IdxT,ValT> super ;
   public:
      VectorMeasurePunctNum(VectorMeasure<IdxT,ValT>* base, bool num, bool punct)
	 : super(base), m_numbers(num), m_punct(punct)
	 {
	 }
      ~VectorMeasurePunctNum() {}

      virtual double similarity(const Vector<IdxT,ValT>* v1, const Vector<IdxT,ValT>* v2) const
	 {
	    return cluster_conflict(v1,v2,m_numbers,m_punct) ? -DBL_MAX : this->baseSimilarity(v1,v2) ;
	 }
      virtual double distance(const Vector<IdxT,ValT>* v1, const Vector<IdxT,ValT>* v2) const
	 {
	    return cluster_conflict(v1,v2,m_numbers,m_punct) ? DBL_MAX : this->baseDistance(v1,v2) ;
	 }

   protected:
      bool m_numbers ;
      bool m_punct ;
   } ;

//----------------------------------------------------------------------

static List *strip_counts(List *counts)
{
   ListBuilder result ;
   for (auto obj : *counts)
      {
      if (obj && obj->isList())
	 {
	 result += static_cast<List*>(obj)->front() ;
	 }
      else
	 result += obj ;
      }
   counts->shallowFree() ;
   return result.move() ;
}

//----------------------------------------------------------------------

static int compare_key_counts(const Object *o1, const Object *o2)
{
   if (o1 && o2)
      {
      if (o1->isList() && o2->isList())
	 {
	 const List* l1 = static_cast<const List*>(o1) ;
	 const List* l2 = static_cast<const List*>(o2) ;
	 size_t count1 = l1->nth(1)->intValue() ;
	 size_t count2 = l2->nth(1)->intValue() ;
	 if (count1 > count2)
	    return -1 ;
	 else if (count1 < count2)
	    return +1 ;
	 else
	    {
	    Symbol *word1 = (Symbol*)(l1->front()) ;
	    Symbol *word2 = (Symbol*)(l2->front()) ;
	    return word1->compare(word2) ;
	    }
	 }
      else
	 return o1->compare(o2) ;
      }
   else if (o1)
      return -1 ;
   else if (o2)
      return +1 ;
   return 0 ;
}

//----------------------------------------------------------------------

static List *move_to_front(List *keywords, SymHashTable *priority, bool run_verbosely)
{
   if (!priority || priority->currentSize() == 0)
      return keywords ;
   if (run_verbosely)
      cout << ";   moving seeds to front of word list\n" ;
   ListBuilder hi_pri ;
   ListBuilder lo_pri ;
   SymbolTable* symtab = SymbolTable::current() ;
   while (keywords != List::emptyList())
      {
      Object* w ;
      keywords = keywords->pop(w) ;
      if (!w)
	 continue ;
      Symbol* word ;
      if (w->isSymbol())
	 word = static_cast<Symbol*>(w) ;
      else
	 {
	 word = symtab->add(w->printableName()) ;
	 w->free() ;
	 }
      if (priority->contains(word))
	 hi_pri += word ;
      else
	 lo_pri += word ;
      }
   return hi_pri.move()->nconc(lo_pri.move()) ;
}

//----------------------------------------------------------------------

static List* highest_frequency_terms(SymHashTable* key_words, SymHashTable *seeds,
   size_t min_freq, size_t max_freq, size_t stop_terms, bool excl_numbers,
   bool run_verbosely)
{
   size_t highest_freq = 0 ;
   ListBuilder kw ;
   for (const auto entry : *key_words)
      {
      auto word = const_cast<Symbol*>(entry.first) ;
      auto tv = (WcTermVector*)entry.second ;
      if (!tv) continue ;
      size_t count = (size_t)tv->weight() ;
      if (count > highest_freq) highest_freq = count ;
      if ((seeds && word && seeds->contains(word) && count > 0)
	 ||
	  (count >= min_freq && count <= max_freq &&
	   (!excl_numbers || !is_number(word->c_str()))))
	 {
	 kw += List::create(word,Integer::create(count)) ;
	 }
      }
   List* kw_list = strip_counts(kw.move()->sort(compare_key_counts)) ;
   // remove the 'stop_terms' highest-frequency terms from clustering
   kw_list = kw_list->elide(0,stop_terms) ;
   if (kw_list)
      kw_list = move_to_front(kw_list,seeds,run_verbosely) ;
   else
      {
      cout << ";   frequency limits have removed all candidates (highest freq is " << highest_freq << ")\n" ;
      }
   return kw_list ;
}

//----------------------------------------------------------------------
// key_words is a mapping from compound-word to WcTermVector, while
// seeds is a mapping from compound-word to equivalence-class-name

ClusterInfo* cluster_vectors(SymHashTable *key_words, const WcParameters *params,
			const WcWordCorpus* corpus, SymHashTable *seeds,
   			VectorMeasure<WcWordCorpus::ID,float>* measure,
		        bool run_verbosely)
{
   if (!key_words)
      return nullptr ;
   cout << ";   sorting by term frequency\n" ;
   size_t min_freq = params->minWordFreq() ;
   if (params->reclusterSeeds()) min_freq = ~0L ;
   List* kw_list = highest_frequency_terms(key_words,seeds,min_freq,params->maxWordFreq(),
      params->stopTermCount(),params->excludeNumbers(),run_verbosely) ;
   if (!kw_list)
      return nullptr ;
   cout << ";   " << kw_list->size() << " terms to be clustered (target " << params->desiredClusters()
	<< " clusters)\n" ;
   size_t max_terms = params->maxTermCount() ;
   if (max_terms > 0 && max_terms < kw_list->size())
      {
      cout << ";   (limiting clustering to " << max_terms << " term vectors)\n" ;
      (void)kw_list->elide(max_terms,(size_t)~0) ;
      }
   bool ignore_unseen_seeds = params->ignoreUnseenSeeds() ;
   if (params->reclusterSeeds())
      seeds = nullptr ;
   List *allseeds = (seeds && !ignore_unseen_seeds) ? seeds->allKeys() : List::emptyList() ;
   SymbolTable* symtab = SymbolTable::current() ;
   ScopedObject<RefArray> vectors(kw_list->size()) ;
   for (auto keyword_obj : *kw_list)
      {
      auto keyword = static_cast<Symbol*>(keyword_obj) ;
      Object *tv_obj ;
      if (key_words->lookup(keyword,&tv_obj))
	 {
	 auto tv = (WcTermVector*)tv_obj ;
	 // skip missing and empty term vectors
	 if (!tv || tv->length() == 0)
	    continue ;
	 Object *name_obj ;
	 if (seeds && seeds->lookup(keyword,&name_obj))
	    {
	    tv->setLabel(static_cast<Symbol*>(name_obj)) ;
	    allseeds = allseeds->removeIf(Fr::equal,keyword) ;
	    }
	 else if (is_number(source_word(keyword)))
	    {
	    if (!params->excludeNumbers())
	       {
	       tv->setLabel(ClusterInfo::numberLabel()) ;
	       }
	    }
	 else
	    tv->setLabel(nullptr) ;
	 vectors->append(tv) ;
	 }
      }
   kw_list->free() ;
   for (const auto s : *allseeds)
      {
      // insert dummy term vectors for any seeds which didn't actually occur
      // in the training data
      Symbol* seedsym = symtab->add(s->stringValue()) ;
      Object* kw_entry ;
      if (key_words->lookup(seedsym,&kw_entry) && kw_entry)
	 continue ;			// already have a term vector for this seed word
      WcTermVector* tv = WcTermVector::create(1) ;
      if (!tv)
	 {
	 SystemMessage::no_memory("allocating term vector for seed word") ;
	 break ;
	 }
      Object* clusname ;
      (void)seeds->lookup(seedsym,&clusname) ;
      tv->setKey(seedsym) ;
      tv->setLabel(static_cast<Symbol*>(clusname)) ;
      vectors->append(tv) ;
      // add the new term vector to the keywords hash table, so that it gets
      //   freed when we're done clustering
      key_words->add(seedsym,(Object*)tv) ;
      }
   allseeds->free() ;
   vectors->reverse() ;
   if (params->clusteringMeasure() && strcasecmp(params->clusteringMeasure(),"user") == 0)
      {
      if (!measure)
	 measure = new VectorMeasureSplitCosine<WcWordCorpus::ID,float>(corpus) ;
      }
   if (measure && (params->keepNumbersDistinct() || params->keepPunctuationDistinct()))
      {
      measure = new VectorMeasurePunctNum<WcWordCorpus::ID,float>(measure,params->keepNumbersDistinct(),
	 params->keepPunctuationDistinct()) ;
      }
   auto paramstr = WcBuildParameterString(params) ;
   auto algo = ClusteringAlgo<WcWordCorpus::ID,float>::instantiate(params->clusteringMethod(),paramstr,measure) ;
   ClusterInfo* clusters = nullptr ;
   if (algo)
      {
      cout << ";  clustering " << vectors->size() << " vectors\n" ;
      algo->setLoggingPrefix("; ") ;
      clusters = algo->cluster(vectors) ;
      delete algo ;
      }
   else
      {
      cout << ";  NO CLUSTERING ALGORITHM!\n" ;
      if (measure) measure->free() ;
      }
   return clusters ;
}

// end of file wcclust.cpp //

/************************************************************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcmain.cpp	      word clustering (main program)		*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2005,2006,2008,2009,2010,2015,	*/
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

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include "wordclus.h"
#include "wcbatch.h"
#include "wcpair.h"
#include "wctrmvec.h"
#include "wcparam.h"

#include "framepac/cluster.h"
#include "framepac/fasthash64.h"
#include "framepac/file.h"
#include "framepac/memory.h"
#include "framepac/progress.h"
#include "framepac/stringbuilder.h"
#include "framepac/symboltable.h"
#include "framepac/texttransforms.h"
#include "framepac/timer.h"
#include "framepac/wordcorpus.h"

using namespace Fr ;

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#ifndef EXIT_SUCCESS
#  define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#  define EXIT_FAILURE 1
#endif

#define DOT_INTERVAL 200000

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

class WcContext
   {
   public:
      WcContext(const WcWordCorpus*, int dir, unsigned leftcontext) ;
      ~WcContext() {}

      const WcWordCorpus* corpus() const { return m_corpus ; }
      size_t occurrences() const { return m_occurrences ; }
      size_t wordCount() const { return m_numIDs ; }
      size_t contextSize() const { return m_contextlength ; }
      WcWordCorpus::ID contextID(size_t N) { return N < m_numIDs ? m_ids[N] : -1 ; }
      WcWordCorpus::ID canonContextID(size_t N) { return N < m_contextlength ? m_canon_ids[N] : -1 ; }
      bool sameContext(size_t loc) const ;
      void addOccurrence() { m_occurrences++ ; }
      void addLeftContext(WcWordCorpus::Index loc, const WcParameters* params,
			  WcIDCountHashTable& counts) ;
      void addRightContext(WcWordCorpus::Index loc, const WcParameters* params,
			   WcIDCountHashTable& counts) ;
      void updateCounts(WcIDCountHashTable& counts) ;

   protected:
      // it's OK to make the arrays ridiculously large to avoid dynamic allocation,
      //   since we'll only ever have two instances per thread unless splitting
      //   term vectors by context
      WcWordCorpus::ID  m_ids[100] ;
      WcWordCorpus::ID	m_canon_ids[10] ;
      const WcWordCorpus* m_corpus ;
      size_t            m_numIDs ;
      size_t            m_contextlength ;
      size_t            m_occurrences ;
      unsigned	        m_leftcontext ;
      int	        m_direction ;
   protected: //methods
      bool makeLeftContext(size_t loc, const WcParameters*) ;
      bool makeRightContext(size_t loc, const WcParameters*) ;
   } ;

//----------------------------------------------------------------------

struct CtxtVecInfo
   {
   const WcParameters* params ;
   const WcWordCorpus* corpus ;
   SymHashTable* ht ;
   } ;

//----------------------------------------------------------------------

class tvContextInfo : public Object
   {
   public:
      WcContext left ;
      WcContext right ;
      ScopedObject<WcIDCountHashTable> counts;
      size_t freq ;
   public:
      tvContextInfo(const WcWordCorpus* corpus, size_t context, size_t left_context, size_t right_context)
	 : left(corpus,-1,context-left_context),
	   right(corpus,+1,context-right_context),
	   counts(), freq(1)
	 {
	 }
      virtual ~tvContextInfo() {}

      void incrFreq() { freq++ ; }
   } ;

//----------------------------------------------------------------------

class CtxtKey : public Object
   {
   public:
      CtxtKey() { rewind() ; }
      CtxtKey(const CtxtKey* orig)
	 {
	    rewind() ;
	    if (orig)
	       {
	       m_active = orig->m_active ;
	       std::copy(orig->m_ids,orig->m_ids+m_active,m_ids) ;
	       }
	 }
      ~CtxtKey() {}

      size_t hashValue() const
	 {
	    unsigned long hash = 1 ;
	    for (size_t i = 0 ; i < m_active ; i++)
	       {
	       hash = (hash >> 5) | (hash << ((sizeof(hash)*8) - 5)) ;
	       hash ^= m_ids[i] ;
	       }
	    return hash ;
	 }
      bool equal(const Object* obj) const
	 {
	    // only works with other objects of our type!
	    const CtxtKey* other = static_cast<const CtxtKey*>(obj) ;
	    if (other->m_active != m_active) return false ;
	    return memcmp(m_ids,other->m_ids,m_active*sizeof(m_ids[0])) == 0 ;
	 }
      Object* clone() const { return new CtxtKey(this) ; }
      Object* shallowCopy() const { return clone() ; }

      void addID(WcWordCorpus::ID id) { m_ids[m_active++] = id ; }
      void rewind() { m_active = 0 ; }
      WcWordCorpus::ID nextID() { return m_ids[m_active++] ; }

   protected:
      WcWordCorpus::ID m_ids[6] ; // max 3 on left and 3 on right
      uint8_t        m_active ;

   } ;

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

static Ptr<ProgressIndicator> progress ;

/************************************************************************/
/*	External Functions						*/
/************************************************************************/

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

static bool tokenized_word(const char *context, WcWordCorpus::ID &token,
			   const WcWordCorpus *corpus, bool auto_numbers)
{
   WcWordCorpus::ID id = corpus->getContextID(context) ;
   if (id != WcWordCorpus::ErrorID)
      {
      token = id ;
      return true ;
      }
   if (auto_numbers && is_number(context))
      {
      token = corpus->numberToken() ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

void WcRemoveAutoClustersFromSeeds(SymHashTable *seeds)
{
   if (seeds)
      {
      for (const auto entry : *seeds)
	 {
	 Symbol* cluster = static_cast<Symbol*>(entry.second) ;
	 if (ClusterInfo::isGeneratedLabel(cluster))
	    seeds->remove(entry.first) ;
	 }
      }
   return ;
}

/************************************************************************/
/*	Methods for class WcWordIDPair					*/
/************************************************************************/

unsigned long WcWordIDPair::hashValue() const
{
   FastHash64 hash ;
   hash += m_word1 ;
   hash += m_word2 ;
   return (unsigned long)(*hash) ;
}

//----------------------------------------------------------------------

bool WcWordIDPair::equal(const WcWordIDPair& other) const
{
   return m_word1 == other.m_word1 && m_word2 == other.m_word2 ;
}

/************************************************************************/
/*	Methods for class WcContext					*/
/************************************************************************/

WcContext::WcContext(const WcWordCorpus* corp, int dir, unsigned lcontext) :
   m_corpus(corp), m_numIDs(0), m_contextlength(0), m_occurrences(0),
   m_leftcontext(lcontext), m_direction(dir)
{
#if 0
   // we don't actually need to clear the ID and symbol arrays, but doing so makes
   //   it easier to look at the structure in a debugger
   std::fill(m_ids,m_ids+lengthof(m_ids),0) ;
   std::fill(m_canon_ids,m_canon_ids+lengthof(m_canon_ids),0) ;
#endif
   return ;
}

//----------------------------------------------------------------------

bool WcContext::sameContext(size_t loc) const
{
   if (m_numIDs == 0)
      return false ;			// we haven't yet seen a previous context, so always different
   for (size_t i = 0 ; i < m_numIDs ; ++i)
      {
      if (m_corpus->getID(loc+i) != m_ids[i])
	 {
	 return false ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

void WcContext::updateCounts(WcIDCountHashTable& counts)
{
   if (!occurrences())
      return ;
   for (int i = 0 ; i < (int)contextSize() ; ++i)
      {
      WcWordCorpus::ID pos_id = corpus()->positionalID(m_canon_ids[i],m_direction*(i+1)) ;
      counts.addCount(pos_id,occurrences()) ;
      }
   m_occurrences = 0 ;
   return ;
}

//----------------------------------------------------------------------

static size_t find_context_term(const WcWordCorpus* corpus, size_t loc, const WcParameters* params,
				  WcWordCorpus::ID& token)
{
   size_t max = corpus->longestContextEquiv() ;
   if (loc + max > corpus->corpusSize())
      max = corpus->corpusSize() - loc ;
   size_t longest_match(0) ;
   if (max)
      {
      StringBuilder phrase ;
      for (size_t i = 0 ; i < max ; ++i)
	 {
	 if (i > 0) phrase += " " ;
	 phrase += corpus->getWordForLoc(loc+i) ;
	 phrase += '\0' ;		// ensure termination when we access the buffer
	 WcWordCorpus::ID tok ;
	 if (tokenized_word(*phrase,tok,corpus,params->autoNumbers()))
	    {
	    token = tok ;
	    longest_match = i+1 ;
	    }
	 phrase.remove() ;		// drop the terminator before adding the next word
	 }
      }
   if (longest_match == 0)
      {
      if (loc >= corpus->corpusSize())
	 token = corpus->newlineID() ;
      else
	 token = corpus->getContextID(loc) ;
      longest_match = 1 ;
      }
   return longest_match ;
}

//----------------------------------------------------------------------

bool WcContext::makeLeftContext(size_t loc, const WcParameters *params)
{
   m_numIDs = 0 ;
   m_contextlength = 0 ;
   for (size_t i = 0 ; i < m_leftcontext ; ++i)
      {
      size_t token_length(1) ;
      // handle hitting the start of the corpus
      size_t limit = m_corpus->longestContextEquiv() ;
      if (limit > loc)
	 limit = loc ;
      WcWordCorpus::ID token(m_corpus->newlineID()) ;
      for (size_t j = limit ; j >= 1 ; --j)
	 {
	 token_length = find_context_term(m_corpus,loc-j,params,token) ;
	 if (token_length == j)
	    break ;
	 }
      m_canon_ids[m_contextlength++] = token ;
      for (size_t j = 1 ; j <= token_length ; ++j)
	 {
	 m_ids[m_numIDs++] = m_corpus->getID(loc-j) ;
	 }
      loc -= token_length ;
      if (token == m_corpus->newlineID())
	 {
	 // we've hit the beginning of the line, so replicate the
	 //   newline token for the rest of the context
	 while (m_contextlength < m_leftcontext)
	    {
	    m_canon_ids[m_contextlength++] = token ;
	    m_ids[m_numIDs++] = token ;
	    ++i ;
	    }
	 }
      }
   // reverse the stored IDs so that they are in the same order as in the WcWordCorpus
   std::reverse(m_ids,m_ids+m_numIDs) ;
   m_occurrences = 1 ;
   return true ;
}

//----------------------------------------------------------------------

bool WcContext::makeRightContext(size_t loc, const WcParameters* params)
{
   m_numIDs = 0 ;
   m_contextlength = 0 ;
   WcWordCorpus::ID newline = m_corpus->newlineID() ;
   WcWordCorpus::ID prev_id = WcWordCorpus::ErrorID ;
   for (size_t i = 0 ; i < params->neighborhoodRight() ; ++i)
      {
      // handle the case of falling off the end of a line or the end of the corpus
      if (prev_id == newline || loc >= m_corpus->corpusSize())
	 {
	 m_ids[m_numIDs++] = newline ;
	 m_canon_ids[m_contextlength++] = newline ;
	 loc++ ;
	 }
      else
	 {
	 WcWordCorpus::ID token(newline) ;
	 size_t token_length(find_context_term(m_corpus,loc,params,token)) ;
	 m_canon_ids[m_contextlength++] = token ;
	 prev_id = token ;
	 for (size_t i = 0 ; i < token_length ; ++i)
	    {
	    m_ids[m_numIDs++] = m_corpus->getID(loc+i) ;
	    }
	 loc += token_length ;
	 }
      }
   m_occurrences = 1 ;
   return true ;
}

//----------------------------------------------------------------------

void WcContext::addLeftContext(WcWordCorpus::Index loc, const WcParameters* params,
			        WcIDCountHashTable& counts)
{
   size_t wc = wordCount() ;
   if (loc >= wc && sameContext(loc-wc))
      {
      addOccurrence() ;
      }
   else
      {
      updateCounts(counts) ;
      makeLeftContext(loc,params) ;
      }
   return ;
}

//----------------------------------------------------------------------

void WcContext::addRightContext(WcWordCorpus::Index loc, const WcParameters* params,
				 WcIDCountHashTable& counts)
{
   // try to re-use the computed context from the previous occurrence by only generating
   //   the context if we have a difference sequence of word IDs this time around
   if (sameContext(loc))
      {
      addOccurrence() ;
      }
   else
      {
      updateCounts(counts) ;
      makeRightContext(loc,params) ;
      }
   return ;
}

/************************************************************************/
/************************************************************************/

static WcTermVector* add_vector(CtxtVecInfo* cvec_info, Symbol* keysym, size_t freq,
   WcIDCountHashTable* counts, const WcWordCorpus* corpus, const WcParameters& params)
{
   WcTermVector* tv = WcTermVector::create(counts,corpus,params) ;
   tv->setKey(keysym) ;
   tv->setWeight(freq) ;
   if (params.m_decay_type != Decay_None)
      {
      if (tv->isSparseVector())
	 tv->weightTerms(params.m_decay_type, params.m_past_boundary_weight) ;
      }
   if (cvec_info && cvec_info->ht->add(keysym,tv))
      {
      // we've already used that keysym, so gensym a new sym
      keysym = SymbolTable::current()->gensym(keysym->c_str()) ;
      cvec_info->ht->add(keysym,tv) ;
      }
   return tv ;
}

//----------------------------------------------------------------------

static bool free_key(Object *key, Object *value)
{
   key->free() ;
   value->free() ;
   return true ;
}

//----------------------------------------------------------------------

static bool free_key(Object *key, size_t /*value*/)
{
   key->free() ;
   return true ;
}

//----------------------------------------------------------------------

static CtxtKey* make_context_key(const CtxtVecInfo* cvec_info, const WcWordCorpus* corpus,
				 WcWordCorpus::Index match, size_t keylen)
{
   WcWordCorpus::Index loc = corpus->getForwardPosition(match) ;
   CtxtKey* contextkey = new CtxtKey ;
   for (size_t i = 1 ; i <= cvec_info->params->left_context ; i++)
      {
      WcWordCorpus::ID tok = corpus->getContextEquivID(loc-i) ;
      contextkey->addID(tok) ;
      }
   for (size_t i = 0 ; i < cvec_info->params->right_context ; i++)
      {
      WcWordCorpus::ID tok = corpus->getContextEquivID(loc+keylen+i) ;
      contextkey->addID(tok) ;
      }
   return contextkey ;
}

//----------------------------------------------------------------------

static bool add_contextual_vector(Object *key, Object* value, CtxtVecInfo* cvec_info, Symbol* keysym,
   const WcWordCorpus* corpus, const WcParameters& params)
{
   CtxtKey* context_key = (CtxtKey*)key ;
   tvContextInfo* info = (tvContextInfo*)value ;
   WcIDCountHashTable* dcounts = &info->counts ;
   auto tv = add_vector(cvec_info,keysym,info->freq,dcounts,corpus,params) ;
   if (tv)
      {
      context_key->rewind() ;
      // set left and right disambiguation contexts on the vector
      ListBuilder disambig ;
      for (size_t i = 0 ; i < cvec_info->params->left_context ; ++i)
	 {
	 WcWordCorpus::ID id = context_key->nextID() ;
	 disambig.push(String::create(corpus->getWord(id))) ;
	 }
      tv->leftConstraint(disambig.move()) ;
      for (size_t i = 0 ; i < cvec_info->params->right_context ; ++i)
	 {
	 WcWordCorpus::ID id = context_key->nextID() ;
	 disambig.push(String::create(corpus->getWord(id))) ;
	 }
      tv->rightConstraint(disambig.move()) ;
      // if the term is a seed, add the associated label to the vector
      const auto seeds = params.equivalenceClasses() ;
      Object* label ;
      if (seeds && seeds->lookup(keysym,&label))
	 {
	 tv->setLabel(static_cast<Symbol*>(label)) ;
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

static bool desireable_term(const WcWordCorpus::ID* key, unsigned keylen, const WcWordCorpus* corpus,
   WcWordIDPairTable* mutualinfo)
{
   if (keylen > 1)
      {
      // skip the phrase if it starts or ends with a newline or stopword
      if (!corpus->getWord(key[keylen-1]))
	 {
	 return false ;
	 }
      // skip the phrase if it is not coherent enough
      // we decide coherence based on mutual info between adjacent pairs of words;
      //   if any pair is below threshold (and thus does not appear in the mutualinfo
      //   table), we declare it not coherent and skip it
      for (unsigned i = 1 ; i < keylen ; ++i)
	 {
	 if (!mutualinfo->contains(key[i-1],key[i]))
	    {
	    return false ;
	    }
	 }
      }
   return true ;
}

//----------------------------------------------------------------------

static bool make_context_vector(const WcWordCorpus::ID* key, unsigned keylen, size_t freq,
   WcWordCorpus::Index first_match, CtxtVecInfo* cvec_info)
{
   const WcWordCorpus* corpus = cvec_info->corpus ;
   const WcParameters* params = cvec_info->params ;
   WcWordCorpus::Index last_match = first_match + freq ;
   size_t disambig = params->left_context + params->right_context ;
   // we need a hash table of contexts, keyed by the words which will become part of the
   //    disambiguation context for substitution when normalizing text
   ScopedObject<ObjHashTable> by_context(10000) ;
   if (disambig)
      {
      // collect context terms which are above the frequency cutoff
      ScopedObject<ObjCountHashTable> context_counts(10000) ;
      context_counts->onRemove(free_key) ;
      for (auto match = first_match ; match < last_match ; ++match)
	 {
	 CtxtKey* contextkey = make_context_key(cvec_info,corpus,match,keylen) ;
	 if (context_counts->addCount(contextkey,1) > 1)
	    {
	    // the key was already in the hash table, so we don't need it to live until the
	    //   hash table is destroyed
	    contextkey->free() ;
	    }
	 }
      size_t minfreq = params->minWordFreq() ;
      for (const auto entry : *context_counts)
	 {
	 size_t count = entry.second ;
	 if (count >= minfreq)
	    {
	    tvContextInfo *dcontexts = new tvContextInfo(corpus,params->neighborhoodLeft(),params->left_context,
							 params->right_context) ;
	    dcontexts->freq = count ;
	    by_context->add(entry.first->clone().move(),dcontexts) ;
	    }
	 }
      }
   ScopedObject<WcIDCountHashTable> counts(10000) ;
   unsigned lcontext = cvec_info->params->neighborhoodLeft() ;
   WcContext left_context(corpus,-1,lcontext) ;
   WcContext right_context(corpus,+1,lcontext) ;
   size_t processed = 0 ;
   size_t remaining = freq ;
   constexpr size_t interval = 50000 ;
   for (auto match = first_match ; match < last_match ; ++match)
      {
      // keep the progress indicator moving when we get down to a couple of threads working on very
      //   high-frequency terms
      if ((++processed % interval) == 0)
	 {
	 (*progress) += interval ;
	 remaining -= interval ;
	 }
      WcWordCorpus::Index loc = corpus->getForwardPosition(match) ;
      left_context.addLeftContext(loc,cvec_info->params,*counts) ;
      right_context.addRightContext(loc+keylen,cvec_info->params,*counts) ;
      if (disambig)
	 {
	 Ptr<CtxtKey> contextkey(make_context_key(cvec_info,corpus,match,keylen)) ;
	 tvContextInfo *dcontexts = (tvContextInfo*)by_context->lookup(contextkey) ;
	 if (dcontexts)
	    {
	    WcContext* left_disambig = &dcontexts->left ;
	    WcContext* right_disambig = &dcontexts->right ;
	    WcIDCountHashTable* dcounts = &dcontexts->counts ;
	    left_disambig->addLeftContext(loc-cvec_info->params->left_context,cvec_info->params,*dcounts) ;
	    right_disambig->addRightContext(loc+keylen+cvec_info->params->right_context,cvec_info->params,*dcounts) ;
	    }
	 }
      }
   left_context.updateCounts(*counts) ;
   right_context.updateCounts(*counts) ;
   // convert the accumulated counts into a term vector, and add it to the hash table
   //   of all term vectors using the word/phrase as the key
   StringBuilder term ;
   term += corpus->getNormalizedWord(key[0]) ;
   for (unsigned i = 1 ; i < keylen ; ++i)
      {
      term += " " ;
      term += corpus->getNormalizedWord(key[i]) ;
      }
   term += '\0' ;			// ensure termination of string when we access the buffer below
   Symbol *keysym = SymbolTable::current()->add(*term) ;
   add_vector(cvec_info,keysym,freq,&counts,corpus,*params) ;
   // iterate over the different disambiguation contexts, adding those whose frequency is above
   //   the clustering threshold to the list of vectors to be clustered
   by_context->onRemove(free_key) ;
   for (const auto entry : *by_context)
      {
      auto context_key = static_cast<CtxtKey*>(entry.first) ;
      add_contextual_vector(context_key,entry.second,cvec_info,keysym,corpus,*params) ;
      }
   (*progress) += remaining ;
   return true ;
}

//----------------------------------------------------------------------

static bool conditional_make_context_vector(const Symbol* key, const WcWordCorpus* corpus, CtxtVecInfo* cvec_info)
{
   SymHashTable* ht = cvec_info->ht ;
   if (ht->contains(key))
      return true ;			// continue iterating
   // split the key into words and convert them into word IDs
   Ptr<List> words(List::createWordList(key->c_str())) ;
   size_t keylen = words->size() ;
   LocalAlloc<WcWordCorpus::ID> keyids(keylen+1) ;
   size_t i = 0 ;
   for (auto w : *words)
      {
      const char *word = w->printableName() ;
      if ((keyids[i++] = corpus->findID(word)) == WcWordCorpus::ErrorID)
	 {
	 // this word is unknown, so bail out
	 return true ;			// continue iterating
	 }
      }
   // we get here if all of the words actually exist in the corpus, so find the
   //   occurrences of the phrase
   WcWordCorpus::Index first_match, last_match ;
   if (corpus->lookup(keyids,keylen,first_match,last_match))
      {
      // we found the phrase, so now collect the contexts of its occurrences
      size_t freq = last_match - first_match + 1 ;
      make_context_vector(keyids,keylen,freq,first_match,cvec_info) ;
      }
   return true ; // continue iterating
}

//----------------------------------------------------------------------

static void analyze_contexts(const WcWordCorpus* corpus, const WcParameters* params,
			     SymHashTable* ht)
{
   Timer timer ;
   unsigned maxphrase = params->phraseLength() ;
   unsigned minphrase = params->allLengths() ? 1 : maxphrase ;
   CtxtVecInfo cvec_info ;
   cvec_info.params = params ;
   cvec_info.corpus = corpus ;
   cvec_info.ht = ht ;
   progress = new ConsoleProgressIndicator(1,corpus->corpusSize()*maxphrase,50,";   ",";   ") ;
   progress->showElapsedTime(true) ;
   size_t minfreq = params->minWordFreq() ;
   auto mi = params->mutualInfoID() ;
   auto enum_fn = [&] (const WcWordCorpus::SufArr*,const WcWordCorpus::ID* key,unsigned keylen, size_t freq,
		       WcWordCorpus::Index first)
		     { return make_context_vector(key,keylen,freq,first,&cvec_info) ; } ;
   auto filter = [=] (const WcWordCorpus::SufArr*, const WcWordCorpus::ID* key, unsigned keylen,
      		      size_t freq, bool all)
		    {
		    bool keep = (freq >= minfreq
		       && corpus->hasAttribute(key[0],WcATTR_DESIRED) && corpus->getWord(key[0])
		       && (all || keylen == 1 || corpus->hasAttribute(key[keylen-1],WcATTR_DESIRED))
		       && (all || desireable_term(key,keylen,corpus,mi))) ;
		    if (!keep) (*progress) += freq ;
		    return keep ;
		    } ;
   if (minphrase == 1)
      {
      // handle single words
      const_cast<WcWordCorpus*>(corpus)->enumerateForwardParallel(1,1,enum_fn,filter,true) ;
      ++minphrase ;
      }
   // handle multi-word phrases
   if (maxphrase >= minphrase)
      const_cast<WcWordCorpus*>(corpus)->enumerateForwardParallel(minphrase,maxphrase,enum_fn,filter,true) ;
   corpus->finishForwardParallel() ;
   auto seeds = params->equivalenceClasses() ;
   // iterate through 'seeds' looking for any terms which didn't get added to 'ht'
   if (seeds)
      {
      progress = new NullProgressIndicator ;
      cout << ";   checking for missing seeds\n" ;
      ConsoleProgressIndicator prog(1,seeds->size(),50,";   ",";   ") ;
      prog.showElapsedTime(true) ;
      for (auto entry : *seeds)
	 {
	 auto keysym = static_cast<const Symbol*>(entry.first) ;
	 conditional_make_context_vector(keysym,corpus,&cvec_info) ;
	 ++prog ;
	 }
      }
   progress = nullptr ;
   cout << ";   processing contexts took " << timer << ".\n" ;
   return ;
}

/************************************************************************/
/************************************************************************/

List *WcLoadFileList(bool use_stdin, const char *listfile)
{
   ListBuilder filelist ;
   if (use_stdin)
      filelist += String::create("-") ;
   else
      {
      if (!listfile || !*listfile) listfile = "-" ;
      CInputFile listfp(listfile) ;
      if (!listfp)
	 {
	 cerr << "Unable to open " << listfile
	      << " to get list of corpus files!\n" ;
	 exit(EXIT_FAILURE) ;
	 }
      while (CharPtr filename { listfp.getTrimmedLine() })
	 {
	 if (**filename && **filename != '#' && **filename != ';')
	    filelist += String::create(filename) ;
	 }
      }
   return filelist.move() ;
}

//----------------------------------------------------------------------

bool by_chance(size_t freq1, size_t freq2, size_t cooccur,
	       size_t corpus_size)
{
   if (corpus_size >= 100 && freq1 > cooccur && freq2 > cooccur)
      {
      double expected = ((double)freq1 / corpus_size) * (double)freq2 ;
      size_t maxfreq = max(freq1,freq2) ;
      // for low-frequency terms, we can use the expected value as-is, but
      //   if the two terms are very frequent, we need to relax the
      //   by-chance check a little or we risk throwing out good
      //   translations
      double discount = 1.05 - (maxfreq/(double)corpus_size)/5.0 ;
      return (cooccur < discount * expected) ;
      }
   return false ;
}

//----------------------------------------------------------------------

double score_mi_chisquared(const WcWordCorpus* corpus,
			   WcWordCorpus::ID word1, WcWordCorpus::ID word2,
			   size_t cooccur, void *)
{
   // the enumeration ensures that the cooccurrence count is always above threshold
   //if (cooccur < min_frequency)
   //   return -1.0 ;			// don't use this pair

   //  N=total, |X|=freq1, |Y|=freq2, C[X,Y]=cooccur
   //
   //         N( (N-C[X,Y])C[X,Y] - |X||Y| )^2
   //      -----------------------------------
   //       |Y| * (N - |Y|) * |X| * (N - |X|)
   //
   // we normalize the above into the range 0..1 by omitting the N in the
   //   numerator
   size_t freq1 = corpus->getFreq(word1) ;
   size_t freq2 = corpus->getFreq(word2) ;
   size_t total = corpus->corpusSize() ;
   double chi = ((double)(total-cooccur)*(double)cooccur -
		 (double)freq1*(double)freq2) ;
   double denom = (double)freq1 * (double)(total-freq1) *
                  (double)freq2 * (double)(total-freq2) ;
   // return a value between 0 and 1, rather than between 0 and N as for the
   // true chi^2 definition
   return chi * chi / denom ;
}

//----------------------------------------------------------------------

double score_mi_correlation(const WcWordCorpus* corpus,
			    WcWordCorpus::ID word1, WcWordCorpus::ID word2,
			    size_t cooccur, void *)
{
   // the enumeration ensures that the cooccurrence count is always above threshold
   //if (cooccur < min_frequency)
   //   return -1.0 ;			// don't use this pair
   size_t freq1 = corpus->getFreq(word1) ;
   size_t freq2 = corpus->getFreq(word2) ;
   size_t freq = (freq1 < freq2) ? freq1 : freq2 ;
   return cooccur / (double)freq ;
}

//----------------------------------------------------------------------

static WcWordIDPairTable* report_MI_pairings(WcWordIDPairTable* mutualinfo, Timer& timer)
{
   if (mutualinfo && mutualinfo->currentSize() > 0)
      {
      cout << ";  found " << mutualinfo->currentSize()
	   << " words with MI pairings in " ;
      }
   else
      {
      mutualinfo->free() ;
      mutualinfo = nullptr ;
      cout << ";  no mutual-information pairings found!  Spent " ;
      }
   cout << timer << endl ;
   return mutualinfo ;
}

//----------------------------------------------------------------------

static bool mi_for_wordpair(const SuffixArray<WcWordCorpus::ID,WcWordCorpus::Index>*,
   const WcWordCorpus::ID* key, unsigned /*keylen*/, size_t cooccur_count, WcWordCorpus::Index,
   const WcParameters* params)
{
   WcWordCorpus* corpus = params->corpus() ;
   if (params->noPeriodMutualInfo())
      {
      WcWordCorpus::ID period = corpus->findID(".") ;
      if (key[0] == period || key[1] == period)
	 return true ;
      }
   WcMIScoreFuncID *fn = params->miScoreFuncID() ;
   void *udata = params->miScoreData() ;
   double thresh = params->miThreshold() ;
   if (fn && fn(corpus,key[0],key[1],cooccur_count,udata) >= thresh)
      {
      size_t freq1 = corpus->getFreq(key[0]) ;
      size_t freq2 = corpus->getFreq(key[1]) ;
      size_t corpus_size = corpus->corpusSize() ;
      if (!by_chance(freq1,freq2,cooccur_count,corpus_size))
	 {
	 WcWordIDPairTable *mutualinfo = params->mutualInfoID() ;
	 mutualinfo->addPair(key[0],key[1]) ;
	 }
      }
   ++(*progress) ;
   return true ;
}

//----------------------------------------------------------------------

WcWordIDPairTable *WcComputeMutualInfo(const WcWordCorpus* corpus,
				       const WcParameters* params)
{
   Timer timer ;
   WcWordIDPairTable *mutualinfo = WcWordIDPairTable::create() ;
   WcParameters local_params(params) ;
   local_params.mutualInfoID(mutualinfo) ;
   local_params.corpus(const_cast<WcWordCorpus*>(corpus)) ;
   if (!local_params.miScoreFuncID())
      {
      local_params.miScoreFuncID(params->chiSquaredMI()?score_mi_chisquared:score_mi_correlation,nullptr) ;
      }
   progress = new ConsoleProgressIndicator(DOT_INTERVAL,0,50,";   ",";   ") ;
   size_t minfreq = params->minWordFreq() ;
   auto vs = corpus->vocabSize() ;
   ((WcWordCorpus*)corpus)->enumerateForwardParallel(2,2,
      [&] (const WcWordCorpus::SufArr* sa,const WcWordCorpus::ID* key,unsigned keylen, size_t freq, WcWordCorpus::Index first)
      { return mi_for_wordpair(sa,key,keylen,freq,first,&local_params) ; },
      [=] (const WcWordCorpus::SufArr* /*sa*/,const WcWordCorpus::ID* key,unsigned keylen, size_t freq, bool /*all*/)
	 { return freq >= minfreq && key[0] < vs && key[keylen-1] < vs ; } ) ;
   progress = nullptr ;
   local_params.mutualInfoID(nullptr) ;
   return report_MI_pairings(mutualinfo,timer) ;
}

//----------------------------------------------------------------------

static bool tag_desired_words(const WcWordCorpus* corpus, const WcParameters* params)
{
   Timer timer ;
   const auto seeds = params->equivalenceClasses() ;
   size_t count(0) ;
   SymbolTable* symtab = SymbolTable::current() ;
   if (params->excludeNumbers() && corpus->numberToken() != corpus->ErrorID)
      corpus->setAttribute(corpus->numberToken(),WcATTR_STOPWORD) ;
   for (WcWordCorpus::ID i = 0 ; i < corpus->vocabSize() ; ++i)
      {
      WcWordCorpus::Index freq = corpus->getFreq(i) ;
      if (freq < params->minWordFreq() || i == corpus->newlineID())
	 continue ;
      const char *name = corpus->getWord(i) ;
      if (!corpus->hasAttribute(i,WcATTR_STOPWORD) &&
	  !(params->excludeNumbers() && is_number(name)))
	 {
	 corpus->setAttribute(i,WcATTR_DESIRED) ;
	 ++count ;
	 }
      else if (seeds)
	 {
	 Symbol* namesym = symtab->add(name) ;
	 if (seeds->contains(namesym))
	    {
	    corpus->setAttribute(i,WcATTR_DESIRED) ;
	    ++count ;
	    }
	 }
      }
   cout << ";   found " << count << " words above threshold.\n" ;
   cout << ";   tagging desired words took " << timer << ".\n" ;
   return true ;
}

//----------------------------------------------------------------------

static void WcPreFilterVectors(SymHashTable *key_words, const WcParameters &params)
{
   if (!key_words)
      return ;
   Timer timer ;
   size_t before_count = key_words->currentSize() ;
   WcVectorFilterFunc *fn = params.preFilterFunc() ;
   void *data = params.preFilterData() ;
   for (const auto entry : *key_words)
      {
      auto tv = static_cast<WcTermVector*>(entry.second) ;
      if (!fn(tv,&params,nullptr,data))
	 {
	 key_words->remove(entry.first) ;
//!!!	 tv->free() ;  //FIXME: crashes with, leaks memory without
	 }
      }
   size_t after_count = key_words->currentSize() ;
   if (after_count < before_count)
      {
      size_t disc = before_count - after_count ;
      cout << ";   discarded " << disc << " of " << before_count << " vectors.\n" ;
      }
   if (params.runVerbosely())
      {
      cout <<";   filtering vectors took " << timer << ".\n" ;
      }
   return ;
}

//----------------------------------------------------------------------

static int compare(const List* o1, const List* o2)
{
   if (o1 == o2) return 0 ;
   if (!o1 || o1 == List::emptyList()) return -1 ; // anything is greater than NIL
   return o1->compare(o2) ;
}

//----------------------------------------------------------------------

static int compare(const Symbol* s1, const Symbol* s2)
{
   if (s1 == s2) return 0 ;
   if (!s1) return -1 ; // anything is greater than NIL
   if (!s2) return +1 ;
   return strcmp(s1->c_str(),s2->c_str()) ;
}

//----------------------------------------------------------------------

static int compare_key_and_context(const Object* o1, const Object* o2)
{
   auto tv1 = (WcTermVector*)(o1) ;
   auto tv2 = (WcTermVector*)(o2) ;
   if (!tv1 || !tv2)
      {
      static bool warned = false ;
      if (!warned)
	 cout << "null pointer in array being sorted!\n" ;
      warned = true ;
      return tv2 ? -1 : (tv1 ? +1 : 0) ;
      }
   int cmp = compare(tv1->key(),tv2->key()) ;
   if (cmp) return cmp ;
   cmp = compare(tv1->leftConstraint(),tv2->leftConstraint()) ;
   if (cmp) return cmp ;
   cmp = compare(tv1->rightConstraint(),tv2->rightConstraint()) ;
   if (cmp) return cmp ;
   return compare(tv1->label(),tv2->label()) ;
}

//----------------------------------------------------------------------

static int key_and_context_lessthan(const Object* o1, const Object* o2)
{
   return compare_key_and_context(o1,o2) < 0 ; 
}

//----------------------------------------------------------------------

static void WcPostFilterVectors(ClusterInfo* clusters, const WcParameters& params)
{
   // generate a list of all the term vectors that have been clustered, sorted by key
   auto tv_list = clusters->allMembers() ;
   std::sort(tv_list->begin(),tv_list->end(),key_and_context_lessthan) ;
   // pass the list to the user callback; it can remove vectors from a cluster by
   //   setting the term vector's cluster() to nullptr
   (void)params.globalPostFilterFunc()(tv_list,&params,params.globalPostFilterData()) ;
   return ;
}

//----------------------------------------------------------------------

static void WcPostFilterClusters(ClusterInfo* clusters, const WcParameters& params)
{
   if (!clusters)
      return ;
   Timer timer ;
   size_t discarded { 0 } ;
   WcVectorFilterFunc* fn { params.postFilterFunc() } ;
   Ptr<SymHashTable> keys ;
   void *data = params.postFilterData() ;
   if (fn && params.postFilterNeedsKeyTable())
      {
      auto members = clusters->allMembers() ;
      keys = SymHashTable::create(2*members->size()) ;
      for (const auto mem : *members)
	 {
	 auto tv = reinterpret_cast<WcTermVector*>(mem) ;
	 keys->add(tv->key(),tv) ;
	 }
      }
   if (clusters && clusters->subclusters())
      {
      for (auto cluster : *clusters->subclusters())
	 {
	 auto clust = reinterpret_cast<ClusterInfo*>(cluster) ;
	 Array* members = const_cast<Array*>(clust->members()) ;
	 bool changed { false } ;
	 for (auto member = members->begin() ; member != members->end() ; ++member)
	    {
	    auto tv = static_cast<WcTermVector*>(*member) ;
	    if (!tv->label()
	       || (!params.keepSingletons() && clust->size() == 1)
	       || (fn && !fn(tv,&params,keys,data)))
	       {
	       // chop the term vector out of the cluster
	       *member = nullptr ;
	       ++discarded ;
	       changed = true ;
	       }
	    }
	 if (changed)
	    clust->shrink_to_fit() ;
	 }
      }
   if (discarded > 0)
      {
      cout << ";   discarded " << discarded << " vectors.\n" ;
      }
   if (params.runVerbosely())
      {
      cout << ";   filtering vectors took " << timer << ".\n" ;
      }
   return ;
}

//----------------------------------------------------------------------

static ClusterInfo* WcFilterClusterMembers(ClusterInfo *clusters, const WcParameters &params)
{
   if (!clusters)
      return clusters ;
   Timer timer ;
   auto result = clusters ;
   WcClusterFilterFunc *fn = params.clusterFilterFunc() ;
   if (fn && clusters && clusters->subclusters())
      {
      size_t discarded { 0 } ;
      void *data { params.clusterFilterData() } ;
      for (auto cl : *clusters->subclusters())
	 {
	 auto clust = const_cast<ClusterInfo*>(static_cast<const ClusterInfo*>(cl)) ;
	 if (!clust)
	    continue ;
	 size_t startsize { clust->numMembers() } ;
	 fn(clust,&params,data) ;
	 discarded += (startsize - clust->numMembers()) ;
	 }
      if (discarded > 0)
	 {
	 cout << ";   discarded " << discarded << " vectors.\n" ;
	 }
      }
   if (params.clusterPostprocFunc() && result)
      {
      size_t clusters_before = result->size() ;
      result = params.clusterPostprocFunc()(result,params.clusterPostprocData()) ;
      size_t clusters_after = result->size() ;
      if (clusters_before > clusters_after)
	 {
	 cout << ";   eliminated/merged " << (clusters_before-clusters_after) << " of "
	      << clusters_before << " clusters.\n" ;
	 }
      }
   if (params.runVerbosely())
      {
      cout << ";   filtering clusters took " << timer << ".\n" ;
      }
   return result ;
}

//----------------------------------------------------------------------

static void process_vectors(WcParameters& params, WcWordCorpus* corpus, int& passnum, SymHashTable* key_words,
   			Fr::VectorMeasure<WcWordCorpus::ID,float>* measure,
		     	CFile& outfp, CFile& tokfp, CFile& tagfp, const char* outfilename,
   			const char* tokfilename, const char* tagfilename)
{
   if (params.ignoreAutoClusters())
      WcRemoveAutoClustersFromSeeds(params.equivalenceClasses()) ;
   Fr::gc() ;
   if (params.runVerbosely() && params.showMemory())
      Fr::memory_stats(cout) ;
   if (params.preFilterFunc())
      {
      cout << "; Pass " << passnum++ << ": pre-filter term vectors\n" ;
      WcPreFilterVectors(key_words,params) ;
      }
   cout << "; Pass " << passnum++ << ": cluster local contexts\n" ;
   Timer timer ;
   ClusterInfo* clusters = cluster_vectors(key_words,&params,corpus,
				    params.equivalenceClasses(),measure,params.runVerbosely()) ;
   if (!clusters)
      {
      cout << ";  clustering failed\n"  ;
      return ;
      }
   cout << ";   " << clusters->numSubclusters() << " clusters found in " << timer << endl ;
   if (params.runVerbosely())
      {
      cout << ";   cluster sizes:" ;
      auto sub = clusters->subclusters() ;
      for (size_t i = 0 ; sub && i < sub->size() && i < 8 ; ++i)
	 {
	 auto member = static_cast<ClusterInfo*>(sub->getNth(i)) ;
	 cout << ' ' << member->numMembers() << '+' << member->numSubclusters() ;
	 }
      if (sub && sub->size() > 8)
	 cout << " ..." ;
      cout << endl ;
      }
   if (params.clusterFilterFunc() || params.clusterPostprocFunc())
      {
      cout << "; Pass " << passnum++ << ": post-filter clusters\n" ;
      clusters = WcFilterClusterMembers(clusters,params) ;
      }
   if (params.postFilterFunc() || params.globalPostFilterFunc())
      {
      cout << "; Pass " << passnum++ << ": post-filter term vectors\n" ;
      if (params.globalPostFilterFunc())
	 WcPostFilterVectors(clusters,params) ;
      WcPostFilterClusters(clusters,params) ;
      }
   cout << "; Pass " << passnum++ << ": output equivalence classes\n" ;
   const char *seedfile = params.equivClassFile() ;
   bool no_auto = params.skipAutoClusters() && !params.reclusterSeeds() ;
   WcOutputClusters(clusters,outfp,seedfile,WcSORT_OUTPUT,outfilename,no_auto) ;
   WcOutputTokenFile(clusters,tokfp,WcSORT_OUTPUT,tokfilename,no_auto,
      params.suppressAutoBrackets()) ;
   WcOutputTaggedCorpus(clusters,tagfp,WcSORT_OUTPUT,tagfilename,no_auto) ;
   // finally, clean up
   clusters->free() ;
   if (params.runVerbosely() && params.showMemory())
      {
      Fr::gc() ;
      Fr::memory_stats(cout) ;
      }
   return ;
}

//----------------------------------------------------------------------

bool WcProcessCorpus(WcWordCorpus* corpus, Fr::VectorMeasure<WcWordCorpus::ID,float>* measure,
   			CFile& outfp, CFile& tokfp, CFile& tagfp,
		     	const WcParameters *global_params,
		     	const char *outfilename, const char *tokfilename, const char *tagfilename)
{
   if (!corpus || !corpus->corpusSize())
      return false ;
   int passnum(1) ;
   WcParameters params(global_params) ;
   cout << "; Pass " << passnum++ <<": check word frequencies\n" ;
   tag_desired_words(corpus,&params) ;
   if (params.wordFreqFunc())
      {
      // optionally invoke a callback that can modify uniwordfreqs, desired_words, or equiv_classes;
      //   can be used to add below-freqthresh words to equiv_classes as "<rare>"
      cout << "; Pass " << passnum++ << ": adjust word frequencies\n" ;
      params.wordFreqFunc()(params,corpus->corpusSize()) ;
      }
   Ptr<WcWordIDPairTable> mutualinfo ;
   if (params.phraseLength() > 1 && params.miThreshold() > 0.0)
      {
      cout << "; Pass " << passnum++ << ": compute pair-wise mutual information\n" ;
      if (params.miThreshold() > 0.0)
	 {
	 mutualinfo = WcComputeMutualInfo(corpus,&params) ;
	 }
      }
   params.mutualInfoID(mutualinfo) ;
   cout << "; Pass " << passnum++ << ": analyze local contexts\n" ;
   Fr::gc();
   ScopedObject<SymHashTable> key_words(corpus->vocabSize()) ;
   if (params.dimensions())
      {
      auto ctxt = new WcTermVector::context_coll ;
      if (ctxt)
	 {
	 ctxt->setDimensions(params.dimensions()) ;
	 ctxt->setBasisDimensions(params.basisPlus(),params.basisMinus()) ;
	 }
      params.contextCollection(ctxt) ;
      }
   analyze_contexts(corpus,&params,key_words) ;
   params.mutualInfoID(nullptr) ;
   mutualinfo = nullptr ;
   corpus->discardText() ;
   cout << ";   " << key_words->currentSize() << " terms found\n" ;
   Fr::gc() ;
   process_vectors(params,corpus,passnum,key_words,measure,outfp,tokfp,tagfp,
		   outfilename,tokfilename,tagfilename) ;
   delete params.contextCollection() ;
   params.contextCollection(nullptr) ;
   delete corpus ;
   progress = nullptr ;
   return true ;
}

//----------------------------------------------------------------------

// end of file wcmain.cpp //

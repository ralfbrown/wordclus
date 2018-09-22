/************************************************************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wctrmvec.cpp	      term vectors				*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 1999,2000,2002,2005,2009,2015,2016,2017,2018 		*/
/*	   Carnegie Mellon University					*/
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
#include <math.h>
#include <stdlib.h>

#include "framepac/memory.h"
#include "framepac/symboltable.h"

#include "wordclus.h"
#include "wctrmvec.h"

using namespace Fr ;

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

// internal class to sort hash table elements
class HashItem
   {
   public:
      WcWordCorpus::ID word ;
      size_t         weight ;

   public:
      HashItem() {}  // for array construction
      HashItem(WcWordCorpus::ID w, size_t wt) { word = w ; weight = wt ; }
      HashItem(const HashItem &orig) { word = orig.word ; weight = orig.weight ; }

      static int compare(const HashItem &o1, const HashItem &o2)
	 {
	    return (o1.word < o2.word) ? -1 : ((o1.word > o2.word) ? +1 : 0) ;
	 }
      static void swap(HashItem &o1, HashItem &o2)
	 {
	    WcWordCorpus::ID t_word = o1.word ;
	    size_t t_weight = o1.weight ;
	    o1.word = o2.word ;
	    o1.weight = o2.weight ;
	    o2.word = t_word ;
	    o2.weight = t_weight ;
	 }
      HashItem &operator = (const HashItem &orig)
	 {
	    word = orig.word ;
	    weight = orig.weight ;
	    return *this ;
	 }
      bool operator < (const HashItem &other) const
	 {
	    return word < other.word ;
	 }
   } ;

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

/************************************************************************/
/************************************************************************/

// explicitly instantiate the hashtable
namespace Fr
{
template class HashTable<unsigned,size_t> ;
}

/************************************************************************/
/*	Methods for WcTermVectorInfo					*/
/************************************************************************/

void WcTermVectorInfo::leftConstraint(const List* c)
{
   m_left_constraint = c ? static_cast<List*>(c->clone().move()) : nullptr ;
   return ;
}

//----------------------------------------------------------------------

void WcTermVectorInfo::rightConstraint(const List* c)
{
   m_right_constraint = c ? static_cast<List*>(c->clone().move()) : nullptr ;
   return ;
}

/************************************************************************/
/*	Methods for VectorMeasureSplitCosine				*/
/************************************************************************/

template <typename VecT>
static double standard_cosine(const VecT* v1, const VecT* v2)
{
   double prod_lengths { v1->length() * v2->length() } ;
   if (prod_lengths == 0.0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   size_t num_terms { v1->numElements() } ;
   size_t other_terms { v2->numElements() } ;
   double cosin(0.0) ;
   for ( ; term1 < num_terms && term2 < other_terms ; )
      {
      auto word_pos_1 = (WcWordCorpus::ID)v1->elementIndex(term1) ;
      auto word_pos_2 = (WcWordCorpus::ID)v2->elementIndex(term2) ;
      if (word_pos_1 < word_pos_2)
	 {
	 ++term1 ;
	 }
      else if (word_pos_1 > word_pos_2)
	 {
	 ++term2 ;
	 }
      else
	 {
	 double wt1 = v1->elementValue(term1++) ;
	 double wt2 = v2->elementValue(term2++) ;
	 cosin += (wt1 * wt2) ;
	 }
      }
   return cosin / prod_lengths ;
}

//----------------------------------------------------------------------------

template <typename VecT>
static double split_cosine(const VecT* v1, const VecT* v2, unsigned left_context, unsigned total_context)
{
   if (v1->length() == 0.0 || v2->length() == 0.0)
      return 0.0 ;
   size_t term1(0) ;
   size_t term2(0) ;
   size_t num_terms { v1->numElements() } ;
   size_t other_terms { v2->numElements() } ;
   double cosin_l { 0.0 } ;
   double cosin_r { 0.0 } ;
   for ( ; term1 < num_terms && term2 < other_terms ; )
      {
      auto word_pos_1 = (WcWordCorpus::ID)v1->elementIndex(term1) ;
      auto word_pos_2 = (WcWordCorpus::ID)v2->elementIndex(term2) ;
      if (word_pos_1 < word_pos_2)
	 {
	 ++term1 ;
	 }
      else if (word_pos_1 > word_pos_2)
	 {
	 ++term2 ;
	 }
      else
	 {
	 int pos1 = WcWordCorpus::offsetOfPosition(word_pos_1,left_context,total_context) ;
	 int pos2 = WcWordCorpus::offsetOfPosition(word_pos_2,left_context,total_context) ;
	 if ((pos1 >= 0) && (pos2 >= 0))
	    {
	    cosin_r += (v1->elementValue(term1) * v2->elementValue(term2)) ;
	    }
	 else if ((pos1 < 0) && (pos2 < 0))
	    {
	    cosin_l += (v1->elementValue(term1) * v2->elementValue(term2)) ;
	    }
	 ++term1 ;
	 ++term2 ;
	 }
      }
   double vlen1_l { 0.0 } ;
   double vlen1_r { 0.0 } ;
   for (term1 = 0 ; term1 < num_terms ; ++term1)
      {
      auto word_pos_1 = (WcWordCorpus::ID)v1->elementIndex(term1) ;
      int pos1 = WcWordCorpus::offsetOfPosition(word_pos_1,left_context,total_context) ;
      double wt1 = v1->elementValue(term1) ;
      if (pos1 >= 0)
	 vlen1_r += (wt1*wt1) ;
      else
	 vlen1_l += (wt1*wt1) ;
      }
   double vlen2_l { 0.0 } ;
   double vlen2_r { 0.0 } ;
   for (term2 = 0 ; term2 < other_terms ; ++term2)
      {
      auto word_pos_2 = (WcWordCorpus::ID)v2->elementIndex(term2) ;
      int pos2 = WcWordCorpus::offsetOfPosition(word_pos_2,left_context,total_context) ;
      double wt2 = v2->elementValue(term2) ;
      if (pos2 >= 0)
	 vlen2_r += (wt2*wt2) ;
      else
	 vlen2_l += (wt2*wt2) ;
      }
   if (vlen1_l > 0.0 && vlen2_l > 0.0)
      cosin_l /= (sqrt(vlen1_l) * sqrt(vlen2_l)) ;
   else
      cosin_l = 0.0 ;
   if (vlen1_r > 0.0 && vlen2_r > 0.0)
      cosin_r /= (sqrt(vlen1_r) * sqrt(vlen2_r)) ;
   else
      cosin_r = 0.0 ;
   double sum = cosin_l + cosin_r ;
   return sum ? (2.0 * cosin_l * cosin_r / sum) : 0.0 ;
}

//----------------------------------------------------------------------------

template <typename IdxT, typename ValT>
double VectorMeasureSplitCosine<IdxT,ValT>::similarity(const Fr::Vector<IdxT,ValT>* v1, const Fr::Vector<IdxT,ValT>* v2) const
{
   double prod_lengths { v1->length() * v2->length() } ;
   if (!prod_lengths)
      return 0.0 ;
   if (v1->isSparseVector())
      {
      auto sv1 = static_cast<const SparseVector<IdxT,ValT>*>(v1) ;
      auto sv2 = static_cast<const SparseVector<IdxT,ValT>*>(v2) ;
      if (m_total_context <= 1)
	 {
	 // no split!
	 return standard_cosine(sv1,sv2) ;
	 }
      return split_cosine(sv1,sv2,m_left_context,m_total_context) ;
      }
   if (m_total_context <= 1)
      {
      // no split!
      return standard_cosine(v1,v2) ;
      }
   return split_cosine(v1,v2,m_left_context,m_total_context) ;
}

/************************************************************************/
/*	Methods for WcTermVectorSparse					*/
/************************************************************************/

template <typename IdxT>
WcTermVectorSparse<IdxT>::WcTermVectorSparse(const WcIDCountHashTable *ht, const WcWordCorpus *c,
   const WcParameters& p)
   : WcTermVectorSparse<IdxT>(c,p)
{
   if (!ht)
      return  ;
   size_t num_terms = ht->currentSize() ;
   this->reserve(num_terms) ;
   LocalAlloc<HashItem,2048> items(num_terms) ;
   size_t index = 0 ;
   double vector_length = 0 ;
   for (const auto entry : *ht)
      {
      items.base()[index].word = entry.first ; // key
      size_t wt = entry.second ; // value
      if (wt == 0) wt = 1 ;
      items.base()[index].weight = wt ;
      vector_length += ((double)wt * wt) ;
      ++index ;
      }
   std::sort(items.base(), items.base()+num_terms) ;
   for (size_t i = 0 ; i < num_terms ; ++i)
      {
      this->m_indices.full[i] = items[i].word ;
      this->m_values.full[i] = items[i].weight ;
      }
   this->m_size = num_terms ;
   this->m_length = sqrt(vector_length) ;
   return  ;
}

//----------------------------------------------------------------------

template <typename IdxT>
void WcTermVectorSparse<IdxT>::weightTerms(WcDecayType decay, double null_weight)
{
   const WcWordCorpus* crp = corpus() ;
   const WcParameters& p = params() ;
   if (!crp)
      {
      // no reweighting possible, so just compute and cache vector length
      (void)this->vectorLength() ;
      return ;
      }
   this->m_length = -1.0 ;		// clear cached vector length

   size_t range = crp->totalContextSize() + 1 ;
   for (size_t term = 0 ; term < this->m_size ; term++)
      {
      int pos = crp->offsetOfPosition(this->m_indices.full[term]) ;
      if (pos != 0)
	 {
	 if (p.m_termfreq_discount != 1.0)
	    this->m_values.full[term] = pow(this->m_values.full[term],p.m_termfreq_discount) ;
	 double freqwt = 1.0 ;
	 auto word = crp->wordForPositionalID(this->m_indices.full[term]) ;
	 if (p.m_decay_beta != 0.0)
	    {
	    double wordfreq = crp->getFreq(word) ;
	    wordfreq /= crp->corpusSize() ;
	    if (p.m_decay_beta > 0.0)
	       freqwt = std::max(exp(-p.m_decay_beta * wordfreq),p.m_decay_gamma);
	    else
	       {
	       double prob = -p.m_decay_beta * wordfreq ;
	       if (prob > 1.0) prob = 1.0 ;
	       freqwt = -log2(prob) ;
	       }
	    }
	 double weight ;
	 if (decay == Decay_Exponential)
	    {
	    weight = exp(-p.m_decay_alpha * fabs((double)pos)) ;
	    }
	 else if (decay == Decay_Linear)
	    weight = ((range + 1) -
		      fabs((double)pos)) / (double)(range+1) ;
	 else if (decay == Decay_Reciprocal)
	    weight = 1.0 / fabs((double)pos) ;
	 else
	    weight = 1.0 ;
	 if (word == crp->newlineID())
	    weight *= null_weight ;
	 this->m_values.full[term] *= (weight * freqwt) ;
	 }
      }
   return ;
}

/************************************************************************/
/*	Methods for WcTermVectorDense					*/
/************************************************************************/

WcTermVectorDense::WcTermVectorDense(const WcIDCountHashTable* ht, const WcWordCorpus* c, const WcParameters& p)
   : WcTermVectorDense(c,p,p.dimensions())
{
   if (!ht)
      return;
   auto ctxt = p.contextCollection() ;
   if (ctxt)
      {
      for (const auto entry : *ht)
	 {
	 auto key = entry.first ;
	 float value = entry.second ;
	 // value *= termWeight(decay,null_weight) ;  //TODO
	 auto context = ctxt->makeTermVector(key) ;
	 this->incr(context,value) ;
	 }
      }
   else
      {
      for (const auto entry : *ht)
	 {
	 auto key = entry.first ;
	 float value = entry.second ;
	 // value *= termWeight(decay,null_weight) ;  //TODO
	 if (key < this->m_size)
	    {
	    this->m_values.full[key] += value ;
	    }
	 }
      }
   return  ;
}

/************************************************************************/
/************************************************************************/

// request explicit instantiations
template class VectorMeasureSplitCosine<WcWordCorpus::ID,float> ;
template class WcTermVectorSparse<WcWordCorpus::ID> ;

// static data for the instantiated templates
template <>
Allocator WcTermVectorSparse<WcWordCorpus::ID>::s_allocator(FramepaC::Object_VMT<WcTermVectorSparse<WcWordCorpus::ID>>::instance(), sizeof(WcTermVectorSparse<WcWordCorpus::ID>)) ;
template <>
const char WcTermVectorSparse<WcWordCorpus::ID>::s_typename[] = "WcTermVectorSparse" ;

Allocator WcTermVectorDense::s_allocator(FramepaC::Object_VMT<WcTermVectorDense>::instance(), sizeof(WcTermVectorDense)) ;
const char WcTermVectorDense::s_typename[] = "WcTermVectorDense" ;

// end of file wctrmvec.cpp //

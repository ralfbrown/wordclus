/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wctrmvec.h	      term vector declarations			*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2003,2005,2006,2008,2009,2010,	*/
/*		2015,2016,2017,2018 Carnegie Mellon University		*/
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

#ifndef __WCTRMVEC_H_INCLUDED
#define __WCTRMVEC_H_INCLUDED

#include "framepac/list.h"
#include "framepac/vecsim.h"
#include "wcparam.h"

//----------------------------------------------------------------------

class WcTermVectorInfo
   {
   public:
      WcTermVectorInfo(const WcWordCorpus* c, const WcParameters& p) : m_corpus(c), m_params(p) {}

      // accessors
      const WcWordCorpus* corpus() const { return m_corpus ; }
      const WcParameters& params() const { return m_params ; }
      const Fr::List* leftConstraint() const { return m_left_constraint; }
      const Fr::List* rightConstraint() const { return m_right_constraint; }

      // manipulators
      void setCorpus(const WcWordCorpus* corp) { m_corpus = corp ; }
      void leftConstraint(const Fr::List* c) ;
      void rightConstraint(const Fr::List* c) ;

   protected:
      const WcWordCorpus* m_corpus ;
      const WcParameters& m_params ;
      Fr::ListPtr	  m_left_constraint ;
      Fr::ListPtr	  m_right_constraint ;
   } ;

//----------------------------------------------------------------------

template <typename IdxT>
class WcTermVectorSparse : public Fr::SparseVector<IdxT,float>
   {
   public:
      typedef Fr::SparseVector<IdxT,float> super ;
   public:
      static WcTermVectorSparse* create(size_t cap = 0) { return new WcTermVectorSparse(cap) ; }
      static WcTermVectorSparse* create(const WcWordCorpus* c, const WcParameters& p, size_t cap = 0)
	 { return new WcTermVectorSparse(c,p,cap) ; }
      static WcTermVectorSparse* create(const WcIDCountHashTable* counts, const WcWordCorpus* c,
	 const WcParameters& p)
	 { return new WcTermVectorSparse(counts,c,p) ; }

      WcTermVectorInfo* info() const { return reinterpret_cast<WcTermVectorInfo*>(this->userData()) ; }

   public:
      // manipulators
      void weightTerms(WcDecayType decay, double null_weight) ;

      const WcWordCorpus* corpus() const { return info()->corpus() ; }
      const WcParameters& params() const { return info()->params() ; }
      // extra context to separate out variant senses while clustering
      const Fr::List* leftConstraint() const
	 { return info() ? info()->leftConstraint() : Fr::List::emptyList() ; }
      const Fr::List* rightConstraint() const
	 { return info() ? info()->rightConstraint() : Fr::List::emptyList() ; }
      void leftConstraint(const Fr::List* c) { if (info()) info()->leftConstraint(c) ; }
      void rightConstraint(const Fr::List* c) { if (info()) info()->rightConstraint(c) ; }

   protected: // creation/destruction
      void* operator new(size_t) { return s_allocator.allocate() ; }
      void operator delete(void* blk, size_t) { s_allocator.release(blk) ; }
      WcTermVectorSparse(size_t capacity = 0) : super(capacity) {}
      WcTermVectorSparse(const WcWordCorpus* c, const WcParameters& p, size_t cap = 0) : super(cap)
	 { this->setUserData(new WcTermVectorInfo(c,p)) ; }
      WcTermVectorSparse(const WcIDCountHashTable* counts, const WcWordCorpus*, const WcParameters&) ;
      ~WcTermVectorSparse()
	 { delete reinterpret_cast<WcTermVectorInfo*>(this->userData()) ; this->setUserData(nullptr) ; }

   protected: // implementation functions for virtual methods
      friend class FramepaC::Object_VMT<WcTermVectorSparse>  ;

      // *** destroying ***
      static void free_(Fr::Object* obj) { delete static_cast<WcTermVectorSparse*>(obj) ; }

   private:
      static Fr::Allocator s_allocator ;
      static const char s_typename[] ;
   } ;

//----------------------------------------------------------------------

class WcTermVectorDense : public Fr::DenseVector<uint32_t,float>
   {
   public:
      typedef Fr::DenseVector<uint32_t,float> super ;
      typedef Fr::ContextVectorCollection<WcWordCorpus::ID,uint32_t,float,false> context_coll ;
   public:
      static WcTermVectorDense* create(size_t cap = 0) { return new WcTermVectorDense(cap) ; }
      static WcTermVectorDense* create(const WcWordCorpus* c, const WcParameters& p, size_t cap = 0)
	 { return new WcTermVectorDense(c,p,cap) ; }
      static WcTermVectorDense* create(const WcIDCountHashTable* counts, const WcWordCorpus* c,
	 const WcParameters& p)
	 { return new WcTermVectorDense(counts,c,p) ; }

      WcTermVectorInfo* info() const { return reinterpret_cast<WcTermVectorInfo*>(this->userData()) ; }

      // manipulators
      void weightTerms(WcDecayType decay, double null_weight) ;
      void incr(const Vector<uint32_t,float>* other, float weight)
	 {
	    ((Vector<uint32_t,float>*)this)->incr(other,weight) ;
	 }

      const WcWordCorpus* corpus() const { return info() ? info()->corpus() : nullptr ; }
      // extra context to separate out variant senses while clustering
      const Fr::List* leftConstraint() const
	 { return info() ? info()->leftConstraint() : Fr::List::emptyList() ; }
      const Fr::List* rightConstraint() const
	 { return info() ? info()->rightConstraint() : Fr::List::emptyList() ; }
      void leftConstraint(const Fr::List* c) { if (info()) info()->leftConstraint(c); }
      void rightConstraint(const Fr::List* c) { if (info()) info()->rightConstraint(c); }

   protected: // creation/destruction
      void* operator new(size_t) { return s_allocator.allocate() ; }
      void operator delete(void* blk, size_t) { s_allocator.release(blk) ; }
      WcTermVectorDense(size_t cap = 0) : super(cap) { }
      WcTermVectorDense(const WcWordCorpus* c, const WcParameters& p, size_t cap = 0) : super(cap)
	 { this->setUserData(new WcTermVectorInfo(c,p)) ; }
      WcTermVectorDense(const WcIDCountHashTable* counts, const WcWordCorpus*, const WcParameters&) ;
      ~WcTermVectorDense()
	 { delete reinterpret_cast<WcTermVectorInfo*>(this->userData()) ; this->setUserData(nullptr) ; }

   protected: // implementation functions for virtual methods
      friend class FramepaC::Object_VMT<WcTermVectorDense>  ;

      // *** destroying ***
      static void free_(Fr::Object* obj) { delete static_cast<WcTermVectorDense*>(obj) ; }

   private:
      static Fr::Allocator s_allocator ;
      static const char s_typename[] ;
   } ;

//----------------------------------------------------------------------

class WcTermVector : public WcTermVectorSparse<WcWordCorpus::ID>
   {
   public:
      typedef WcTermVectorSparse<WcWordCorpus::ID> super ;
      typedef WcTermVectorSparse<WcWordCorpus::ID> sparse_type ;
      typedef WcTermVectorDense dense_type ;
      typedef Fr::ContextVectorCollection<WcWordCorpus::ID,uint32_t,float,false> context_coll ;
   public:
      static WcTermVector* create(size_t cap = 0) { return static_cast<WcTermVector*>(super::create(cap)) ; }
      static WcTermVector* create(const WcIDCountHashTable* counts, const WcWordCorpus* c,
	 const WcParameters& p)
	 {
	    if (p.contextCollection())
	       return (WcTermVector*)(WcTermVectorDense::create(counts,c,p)) ;
	    else
	       return static_cast<WcTermVector*>(super::create(counts,c,p)) ;
	 }

      void incr(const Vector<uint32_t,float>* other, float weight)
	 {
	    if (isSparseVector())
	       this->sparse_type::incr(other,weight) ;
	    else
	       ((Vector<uint32_t,float>*)this)->incr(other,weight) ;
	 }
      sparse_type* sparseVector() const { return isSparseVector() ? (sparse_type*)this : nullptr ; }
      dense_type* denseVector() const { return isSparseVector() ? nullptr : (dense_type*)this ; }

   protected:
      WcTermVector() : super() {}
      ~WcTermVector() {}
   } ;

//----------------------------------------------------------------------------

template <typename IdxT, typename ValT>
class VectorMeasureSplitCosine : public Fr::SimilarityMeasure<IdxT, ValT>
   {
   public:
      typedef Fr::SimilarityMeasure<IdxT, ValT> super ;
   public:
      VectorMeasureSplitCosine(const WcWordCorpus* corpus)
	 : m_left_context(corpus->leftContextSize()), m_total_context(corpus->totalContextSize()) {}
      virtual ~VectorMeasureSplitCosine() {}

      virtual double similarity(const Fr::Vector<IdxT,ValT>* v1, const Fr::Vector<IdxT,ValT>* v2) const ;

   protected:
      virtual const char* myCanonicalName() const { return "SplitCosine" ; }

   protected:
      unsigned m_left_context ;
      unsigned m_total_context ;
   } ;

//----------------------------------------------------------------------------

#endif /* !__WCTRMVEC_H_INCLUDED */

// end of file wctrmvec.h //

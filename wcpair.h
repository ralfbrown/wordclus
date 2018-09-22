/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust --  Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcpair.h	      word clustering (declarations)		*/
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

#ifndef __WCPAIR_H_INCLUDED
#define __WCPAIR_H_INCLUDED

#include "framepac/hashtable.h"
#include "framepac/wordcorpus.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

/************************************************************************/
/*	Types								*/
/************************************************************************/

//----------------------------------------------------------------------

class ASAN(alignas(8)) WcWordIDPair
   {
   public:
      WcWordIDPair() noexcept { m_word1 = m_word2 = 0 ; }
      WcWordIDPair(Fr::WordCorpus::ID w1, Fr::WordCorpus::ID w2) noexcept
	 { if (w1 == (Fr::WordCorpus::ID)-1 && w2 == (Fr::WordCorpus::ID)-1) { w1 = (Fr::WordCorpus::ID)-2 ; w2 = (Fr::WordCorpus::ID)-2 ; }
	    m_word1 = w1 ; m_word2 = w2 ; }
      WcWordIDPair(const WcWordIDPair&) = default ;
      ~WcWordIDPair() = default ;

      Fr::WordCorpus::ID word1() const { return m_word1 ; }
      Fr::WordCorpus::ID word2() const { return m_word2 ; }

      //  methods to support use in Fr::HashTable
      unsigned long hashValue() const ;
      bool equal(const WcWordIDPair* pair) const ;
      bool equal(const WcWordIDPair& pair) const ;
      int compare(const WcWordIDPair* pair) const ;
      int compare(const WcWordIDPair& pair) const ;
      WcWordIDPair clone() const { return WcWordIDPair(*this) ; }
      WcWordIDPair(unsigned long v) { *((unsigned long*)this) = v ; }
      operator unsigned long () const { return *((unsigned long*)this) ; }
      bool operator== (WcWordIDPair other) const { return (unsigned long)(*this) == (unsigned long)other ; }
      bool operator< (WcWordIDPair other) const { return (unsigned long)(*this) < (unsigned long)other ; }
      bool operator< (WcWordIDPair* other) const { return (unsigned long)(*this) < (unsigned long)(*other) ; }
      const WcWordIDPair* operator-> () const { return this ; }
      WcWordIDPair* operator-> () { return this ; }
      unsigned displayLength() const { return 0 ; }
      char *displayValue(char *buf) { return buf ; }
      size_t cStringLength() const ;
      bool toCstring(char* buffer, size_t buflen) const ;
   protected:
      Fr::WordCorpus::ID m_word1 ;
      Fr::WordCorpus::ID m_word2 ;
   } ;

//----------------------------------------------------------------------

namespace Fr {

template <> template <>
inline Object* HashTable<WcWordIDPair,NullObject>::Table::makeObject(WcWordIDPair key)
{
   (void)key ;
   return (Object*)nullptr ;
}

template <>
inline size_t HashTable<WcWordIDPair,NullObject>::hashVal(const char* keyname, size_t* namelen) const
{ *namelen = strlen(keyname) ; return (size_t)keyname ; } //FIXME


template <>
inline bool HashTable<WcWordIDPair,NullObject>::isEqual(const char* keyname, size_t keylen, WcWordIDPair other)
{
   (void)keyname; (void)keylen; (void)other ;
   return false ; //FIXME
}

template <> template <>
inline bool HashTable<WcWordIDPair,Fr::NullObject>::isEqual(WcWordIDPair w1,
							    WcWordIDPair w2) const
{
   return w1.equal(w2) ;
}

extern template class HashTable<WcWordIDPair,NullObject> ;

} // end namespace Fr

//----------------------------------------------------------------------

inline std::ostream& operator<< (std::ostream& out, WcWordIDPair pair)
{
   out << pair.word1() << ':' << pair.word2() ;
   return out ;
}

//----------------------------------------------------------------------

class WcWordIDPairTable : public Fr::HashTable<WcWordIDPair,Fr::NullObject>
   {
   public:
      typedef Fr::HashTable<WcWordIDPair,Fr::NullObject> super ;
   public:
      // NOTE: there's currently a crash when resizing this hash table, so make its initial
      // size big enough to avoid that (16MB is small potatoes compared to the rest)
      static WcWordIDPairTable* create(size_t cap = 2000000) { return new WcWordIDPairTable(cap) ; }

      // manipulators
      bool addPair(Fr::WordCorpus::ID word1, Fr::WordCorpus::ID word2)
         {
	 WcWordIDPair pair(word1,word2) ;
	 bool already_present = add(pair) ;
	 return already_present ;
	 }

      // accessors
      using Fr::HashTable<WcWordIDPair,Fr::NullObject>::contains ;
      bool contains(Fr::WordCorpus::ID word1, Fr::WordCorpus::ID word2) const
         {
	 WcWordIDPair pair(word1,word2) ;
	 return contains(pair) ;
	 }
   protected:
      friend class FramepaC::Object_VMT<WcWordIDPairTable> ;
      void* operator new(size_t) { return s_allocator.allocate() ; }
      void operator delete(void* blk) { s_allocator.release(blk) ; }
      WcWordIDPairTable(size_t cap = 6000000) : Fr::HashTable<WcWordIDPair,Fr::NullObject>(cap) {}
      ~WcWordIDPairTable() = default ;

      static void free_(Fr::Object* o) { delete static_cast<WcWordIDPairTable*>(o) ; }
      static void shallowFree_(Fr::Object* o) { free_(o) ; }
      static Fr::ObjectPtr clone_(const Fr::Object*) { return nullptr ; } // no cloning (yet?)
      static Fr::Object* shallowCopy_(const Fr::Object* o) { return clone_(o) ; }

      static size_t size_(const Fr::Object* o) { return static_cast<const super*>(o)->currentSize() ; }

   private:
      static Fr::Allocator s_allocator ;
      static const char s_typename[] ;
   } ;


#endif /* !__WCPAIR_H_INCLUDED */

// end of file wcpair.h //

/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust --  Word Clustering					*/
/*  Version 2.00							*/
/*	by Ralf Brown							*/
/*									*/
/*  File: wcpairmap.C							*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 2017,2018 Carnegie Mellon University			*/
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

#include "wcpair.h"
#include "framepac/as_string.h"
#include "template/hashtable.cc"

namespace Fr
{

// request explicit instantiation
template class HashTable<WcWordIDPair,NullObject> ;

} // end namespace Fr

/************************************************************************/
/************************************************************************/

Fr::Allocator WcWordIDPairTable::s_allocator(FramepaC::Object_VMT<WcWordIDPairTable>::instance(),sizeof(WcWordIDPairTable)) ;
const char WcWordIDPairTable::s_typename[] = "WcWordIDPairTable" ;

/************************************************************************/
/************************************************************************/

size_t WcWordIDPair::cStringLength() const
{
   return Fr::len_as_string(m_word1) + Fr::len_as_string(m_word2) + 1 ;
}

bool WcWordIDPair::toCstring(char* buffer, size_t buflen) const
{
   buffer = Fr::as_string(m_word1,buffer,buflen) ;
   if (buflen > 0)
      *buffer++ = ':' ;
   if (buflen > 0)
      buffer = Fr::as_string(m_word2,buffer,buflen) ;
   if (buflen > 0)
      {
      *buffer = '\0' ;
      return true ;
      }
   return false ;
}

// end of file wcpairmap.C //



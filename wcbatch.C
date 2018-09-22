/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcbatch.C	      batch-of-lines processing			*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 2015,2016,2017,2018 Carnegie Mellon University	*/
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

#include "wordclus.h"
#include "wcbatch.h"
#include "wcparam.h"

#include "framepac/file.h"
#include "framepac/texttransforms.h"
#include "framepac/threadpool.h"

using namespace Fr ;

#include <chrono>

using namespace std ;

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

class WcWorkOrder
   {
   public:
      LineBatch*          lines ;
      WcProcessFileFunc*  fn ;
      const WcParameters* params ;
      va_list	          args ;
      bool		  success ;
      volatile bool	  in_use ;
   public:
      WcWorkOrder() { lines = nullptr ; markAvailable() ; }
      ~WcWorkOrder() { reset() ; }
      void reset() { delete lines ; lines = nullptr ; }

      bool inUse() const ;
      void markAvailable() ;
      void markUsed() ;
   } ;

/************************************************************************/
/*	Globals							        */
/************************************************************************/

/************************************************************************/
/*	External Functions						*/
/************************************************************************/

/************************************************************************/
/*	Methods for class WcWorkOrder					*/
/************************************************************************/

bool WcWorkOrder::inUse() const
{
   atomic_thread_fence(std::memory_order_acquire) ; // load barrier
   return in_use ;
}

//----------------------------------------------------------------------

void WcWorkOrder::markAvailable()
{
   in_use = false ;
   atomic_thread_fence(std::memory_order_release) ; // store barrier
   return ;
}

//----------------------------------------------------------------------

void WcWorkOrder::markUsed()
{
   in_use = true ;
   atomic_thread_fence(std::memory_order_release) ; // store barrier
   return ;
}

/************************************************************************/
/************************************************************************/

static void process_file_segment(const void *input, void * /*output*/)
{
   WcWorkOrder *order = (WcWorkOrder*)input ;
   if (order->fn)
      order->success = order->fn(*order->lines,order->params,order->args) ;
   else
      order->success = false ;
   delete order->lines ;
   order->lines = nullptr ;
   order->markAvailable() ;
   return ;
}

//----------------------------------------------------------------------

static int available_order(const WcWorkOrder *orders, size_t num_orders)
{
   for (size_t i = 0 ; i < num_orders ; ++i)
      {
      if (!orders[i].inUse())
	 return i ;
      }
   return -1 ;
}

//----------------------------------------------------------------------

bool WcProcessFile(CFile& fp, const WcParameters *params, WcProcessFileFunc *fn, ...)
{
   if (!fp || !fn || !params)
      return false ;
   va_list args ;
   va_start(args,fn) ;
   ThreadPool *tpool = ThreadPool::defaultPool() ;
   // allocate enough request packets to allow a queue to form for
   //   each thread to avoid task switches, but not so many that we end
   //   up wasting memory
   size_t num_orders = 3 * tpool->numThreads() + 4 ;
   WcWorkOrder orders[num_orders] ;
   bool success = true ;
   std::locale* encoding = WcCurrentCharEncoding() ;
   while (!fp.eof())
      {
      LineBatch *lines = fp.getLines(WcLINES_PER_BATCH,params->monoSkip()) ;
      if (!lines)
	 break ;
      if (params->downcaseSource())
	 {
	 for (char* line : *lines)
	    {
	    lowercase_string(line,encoding) ;
	    }
	 }
      int ordernum ;
      while ((ordernum = available_order(orders,num_orders)) < 0)
	 {
	 // wait 1/20 second, then retry
	 this_thread::sleep_for(chrono::milliseconds(50)) ;
	 }
      orders[ordernum].reset() ;
      orders[ordernum].lines = lines ;
      orders[ordernum].params = params ;
      orders[ordernum].fn = fn ;
      va_copy(orders[ordernum].args,args) ;
      orders[ordernum].success = false ;
      orders[ordernum].markUsed() ;
      tpool->dispatch(&process_file_segment,&orders[ordernum],nullptr) ;
      }
   tpool->waitUntilIdle() ;
   va_end(args) ;
   return success ;
}

// end of file wcbatch.C //

/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcbatch.C	      batch-of-lines processing			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 2015 Ralf Brown/Carnegie Mellon University		*/
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

#include "FramepaC.h"
#include "wordclus.h"
#include "wcbatch.h"
#include "wcglobal.h"

/************************************************************************/
/*	Types for this module						*/
/************************************************************************/

class WcWorkOrder
   {
   public:
      WcLineBatch        *lines ;
      WcProcessFileFunc  *fn ;
      const WcParameters *params ;
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

static FrCriticalSection critsect ;

static char *word_delimiters = nullptr ;

/************************************************************************/
/*	External Functions						*/
/************************************************************************/

/************************************************************************/
/*	Helper functions						*/
/************************************************************************/

const char *WcWordDelimiters()
{
   return word_delimiters ;
}

//----------------------------------------------------------------------

void WcSetWordDelimiters(const char *delim)
{
   if (delim)
      {
      word_delimiters = FrNewN(char,256) ;
      if (word_delimiters)
	 {
	 memcpy(word_delimiters,delim,256) ;
	 }
      }
   else
      {
      FrFree(word_delimiters) ;
      word_delimiters = nullptr ;
      }
   return ;
}

//----------------------------------------------------------------------

void WcClearWordDelimiters()
{
   WcSetWordDelimiters(0) ;
   return ;
}

/************************************************************************/
/*	Methods for class WcWorkOrder					*/
/************************************************************************/

bool WcWorkOrder::inUse() const
{
   critsect.loadBarrier() ;
   return in_use ;
}

//----------------------------------------------------------------------

void WcWorkOrder::markAvailable()
{
   in_use = false ;
   critsect.storeBarrier() ;
   return ;
}

//----------------------------------------------------------------------

void WcWorkOrder::markUsed()
{
   in_use = true ;
   critsect.storeBarrier() ;
   return ;
}

/************************************************************************/
/*	Methods for class WcLineBatch					*/
/************************************************************************/

WcLineBatch::WcLineBatch()
{
   m_numlines = 0 ;
   for (size_t i = 0 ; i < lengthof(m_lines); ++i)
      {
      m_lines[i] = nullptr ;
      m_altlines[i] = nullptr ;
      m_tags[i] = nullptr ;
      }
   return ;
}

//----------------------------------------------------------------------

void WcLineBatch::clear()
{ 
   for (size_t i = 0 ; i < numLines() ; ++i)
      {
      FrFree(m_lines[i]) ;
      m_lines[i] = nullptr ;
      FrFree(m_altlines[i]) ;
      m_altlines[i] = nullptr ;
      FrFree(m_tags[i]) ;
      m_tags[i] = nullptr ;
      }
   m_numlines = 0 ;
   return ;
}

//----------------------------------------------------------------------

bool WcLineBatch::fill(FILE *fp, bool /*monoling*/, bool /*reverse_langs*/, int mono_skip,
		       bool downcase_source, unsigned char *charmap, const char *delim)
{
   while (!feof(fp) && numLines() < maxLines())
      {
      // read the next line from the file
      if (mono_skip < 0)		// skip the first member of each sentence pair
	 {
	 char *sent = FrReadCanonLine(fp,true,char_encoding,&Unicode_bswap,0,false,
				      charmap,delim);
	 FrFree(sent) ;
	 }
      char *ssent = FrReadCanonLine(fp,true,char_encoding,&Unicode_bswap,0,false,
				    charmap,delim) ;
      if (mono_skip > 0)		// skip second member of each sentence pair
	 {
	 char *sent = FrReadCanonLine(fp,true,char_encoding,&Unicode_bswap,0,false,
				      charmap,delim);
	 FrFree(sent) ;
	 }
      if (ssent)
	 {
	 if (*ssent)
	    {
	    if (downcase_source)
	       {
	       Fr_strlwr(ssent,char_encoding) ;
	       }
	    m_lines[numLines()] = ssent ;
	    ++m_numlines ;
	    }
	 else
	    FrFree(ssent) ;
	 }
      }
   return numLines() >= maxLines() ;
}

/************************************************************************/
/************************************************************************/

static void process_file_segment(const void *input, void * /*output*/)
{
   FrSymHashTable::registerThread() ;
   FrSymCountHashTable::registerThread() ;
   WcWorkOrder *order = (WcWorkOrder*)input ;
   if (order->fn)
      order->success = order->fn(*order->lines,order->params,order->args) ;
   else
      order->success = false ;
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

static bool active_orders(const WcWorkOrder *orders, size_t num_orders)
{
   for (size_t i = 0 ; i < num_orders ; ++i)
      {
      if (orders[i].inUse())
	 return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

bool WcProcessFile(FILE *fp, const WcParameters *params, WcProcessFileFunc *fn, ...)
{
   if (!fp || !fp || !params)
      return false ;
   bool downcase_source = params->downcaseSource() ;
//!!!   bool dc_keep_case = DcKeepCase(!downcase_source) ;
   unsigned char *charmap = nullptr ;
   const char *delim = WcWordDelimiters() ;
   bool success = true ;
   va_list args ;
   va_start(args,fn) ;
   size_t num_threads = params->numThreads() ; 
   FrThreadPool *tpool = params->threadPool() ? params->threadPool() : new FrThreadPool(num_threads) ;
   bool must_wait = num_threads == 0 ;
   WcWorkOrder orders[2*num_threads+1] ;
   size_t num_orders = lengthof(orders) ;
   while (!feof(fp))
      {
      WcLineBatch *lines = new WcLineBatch ;
      if (!lines)
	 break ;
      bool monoling = params->monolingualOnly() ;
      int mono_skip = params->monoSkip() ;
      lines->fill(fp,monoling,reverse_languages,mono_skip,downcase_source,charmap,delim) ;
      int ordernum ;
      while (true)
	 {
	 ordernum = available_order(orders,num_orders) ;
	 if (ordernum >= 0)
	    break ;
	 FrThreadSleep(100000) ; // wait 1/10 second, then retry
	 }
      orders[ordernum].reset() ;
      orders[ordernum].lines = lines ;
      orders[ordernum].params = params ;
      orders[ordernum].fn = fn ;
      FrCopyVAList(orders[ordernum].args,args) ;
      orders[ordernum].success = false ;
      orders[ordernum].markUsed() ;
      tpool->dispatch(&process_file_segment,&orders[ordernum],0) ;
      }
   while (active_orders(orders,num_orders))
      {
      FrThreadSleep(250000) ; // sleep for a quarter second
      }
   if (must_wait)
      {
      tpool->waitUntilIdle() ;
      }
   va_end(args) ;
   if (!params->threadPool())
      {
      delete tpool ;
      }
//!!!   (void)DcKeepCase(dc_keep_case) ;
   return success ;
}

// end of file wcbatch.C //

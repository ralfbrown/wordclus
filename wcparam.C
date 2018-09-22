/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcparam.h	      WcParameters structure			*/
/*  LastEdit: 21sep2018							*/
/*									*/
/*  (c) Copyright 2018 Carnegie Mellon University			*/
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

#include "framepac/cluster.h"
#include "framepac/stringbuilder.h"
#include "framepac/texttransforms.h"
#include "wordclus.h"
#include "wcparam.h"

using namespace Fr ;

/************************************************************************/
/************************************************************************/

CharPtr WcBuildParameterString(const WcParameters& params)
{
   StringBuilder sb ;
   sb += aprintf(":measure=%s:representative=%s:iterations=%lu:numclusters=%lu:threshold=%g",
      params.clusteringMeasure()?params.clusteringMeasure():"cosine",
      params.clusteringRep()?params.clusteringRep():"centroid",
      params.iterations(),params.desiredClusters(),params.clusteringThreshold()) ;
   if (params.keepSingletons())
      sb += ":+singletons" ;
   if (params.runVerbosely())
      sb += ":+verbosity" ;
   if (params.clusteringSettings())
      {
      sb += ':' ;
      sb += params.clusteringSettings() ;
      }
   return sb.c_str() ;
}

//----------------------------------------------------------------------------

bool WcValidateParameters(const WcParameters& params)
{
   CharPtr paramstr = WcBuildParameterString(params) ;
   // we need to actually instantiate the clustering algorithm, because it might have added valid parameters
   //   which aren't present in the base class
   auto algo = ClusteringAlgo<WcWordCorpus::ID,float>::instantiate(params.clusteringMethod(),"") ;
   bool valid = algo ? algo->parseOptions(paramstr,true) : false ;
   delete algo ;
   if (!valid && params.runVerbosely())
      {
      cerr << "failed validation on clustering parameter string = " << paramstr << endl ;
      }
   return valid ;
}

// end of file wcparam.C //

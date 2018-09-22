/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wordclus.h	      word clustering (declarations)		*/
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

#ifndef __WORDCLUS_H_INCLUDED
#define __WORDCLUS_H_INCLUDED

#include "framepac/cluster.h"
#include "framepac/file.h"
#include "framepac/hashtable.h"
#include "framepac/list.h"
#include "framepac/threshold.h"
#include "framepac/wordcorpus.h"

/************************************************************************/
/*	Manifest Constants						*/
/************************************************************************/

#define WORDCLUST_VERSION "2.00beta"

#define WcSORT_OUTPUT true

#define WcATTR_STOPWORD 0
#define WcATTR_DESIRED  1
#define WcATTR_DELETABLE 2


/************************************************************************/
/*	Types								*/
/************************************************************************/

#ifdef HUGE_CORPUS
typedef Fr::WordCorpusXL WcWordCorpus ;
#else
typedef Fr::WordCorpus WcWordCorpus ;
#endif /* HUGE_CORPUS */

// forward declarations
class WcConfig ;
class WcParameters ;
class WcTermVector ;

//--------------------------------------------------------------------------

enum WcDecayType
   {
      Decay_None,
      Decay_Reciprocal,
      Decay_Linear,
      Decay_Exponential
   } ;

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------

namespace Fr
{
template <>
inline size_t HashTable<unsigned,size_t>::hashVal(const char* keyname, size_t* namelen) const
{ *namelen = strlen(keyname) ; return (size_t)keyname ; } //FIXME


template <>
inline bool HashTable<unsigned,size_t>::isEqual(const char* keyname, size_t keylen, unsigned other)
{
   (void)keyname; (void)keylen; (void)other ;
   return false ; //FIXME
}

typedef HashTable<unsigned,size_t> WcIDCountHashTable ;
} // end namespace Fr

using Fr::WcIDCountHashTable ;

//--------------------------------------------------------------------------

typedef bool WcGlobalFilterFunc(const Fr::Array* tvs, const WcParameters* params, void* user_data) ;
typedef bool WcVectorFilterFunc(const WcTermVector *tv, const WcParameters *params,
                                const Fr::SymHashTable *keys, void *user_data) ;
typedef void WcClusterFilterFunc(Fr::ClusterInfo* clust, const WcParameters *params, void *user_data) ;
// rules for cluster filter func:
//    any member vectors which are to be discarded from cluster should be set to nullptr
//    cluster label may be modified
//    call shrink_to_fit() if any vectors were discarded
typedef Fr::ClusterInfo *WcClusterPostprocFunc(Fr::ClusterInfo *clusters, void *user_data) ;

typedef void WcWordFreqProcFunc(class WcParameters &params, size_t corpus_size) ;

typedef double WcMIScoreFuncID(const WcWordCorpus*,
			       WcWordCorpus::ID word1, WcWordCorpus::ID word2,
			       size_t cooccur, void *udata) ;

/************************************************************************/
/************************************************************************/

extern bool use_nearest ;
extern bool use_RMS_cosine ;

//----------------------------------------------------------------------

// configuration
void apply_WordClus_configuration(WcConfig *config) ;
void WcLoadTermWeights(const char *weights_file) ;
void WcSetWordDelimiters(const char *delim) ;
const char *WcWordDelimiters() ;

// file access
Fr::List *WcLoadFileList(bool use_stdin, const char *listfile) ;

// corpus generation and loading
void init_corpus_parsing(const char* delims = nullptr) ;
Fr::List *load_file_list(const char* listfile) ;
bool generate_indices(WcWordCorpus *corpus, bool reverse) ;
WcWordCorpus* new_corpus(const WcParameters* params, const char *filename = nullptr) ;
WcWordCorpus* load_corpus(const Fr::List *filelist, const WcParameters *params = nullptr) ;
WcWordCorpus* load_or_generate_corpus(const char *filename, const WcParameters* params) ;

// preprocessing
class WcWordIDPairTable *WcComputeMutualInfo(const WcWordCorpus* corpus,
				               const WcParameters* params) ;

void WcRemoveAutoClustersFromSeeds(Fr::ObjHashTable* seeds) ;

// the actual clustering
Fr::ClusterInfo* cluster_vectors(Fr::SymHashTable* ht, const WcParameters* params,
			  const WcWordCorpus* corpus, Fr::SymHashTable* seeds = nullptr,
			  Fr::VectorMeasure<WcWordCorpus::ID,float>* measure = nullptr,
			  bool verbose = false) ;

// top-level processing functions
bool WcProcessCorpus(WcWordCorpus* corpus,  // deletes corpus to save memory!
   		     Fr::VectorMeasure<WcWordCorpus::ID,float>* measure,
		     Fr::CFile& outfp, Fr::CFile& tokfp, Fr::CFile& tagfp,
                     const WcParameters* global_params,
		     const char* outfilename, const char *tokfilename,
		     const char* tagfilename) ;

// output of results
void WcOutputClusters(const Fr::ClusterInfo* clusters, Fr::CFile& outfp,
		      const char* seed_file, bool sort_output = true,
		      const char* output_filename = nullptr,
		      bool skip_auto_clusters = false) ;
void WcOutputTokenFile(const Fr::ClusterInfo* cluster_list, Fr::CFile& tokfp,
		       bool sort_output = true,
		       const char *output_filename = nullptr,
		       bool skip_auto_clusters = false,
		       bool suppress_auto_brackets = false) ;
void WcOutputTaggedCorpus(const Fr::ClusterInfo* cluster_list, Fr::CFile& tagfp,
			  bool sort_output = true,
			  const char* output_filename = nullptr,
			  bool skip_auto_clusters = false) ;

// cleanup
void WcClearWordDelimiters() ;

//----------------------------------------------------------------------------

std::locale* WcCurrentCharEncoding() ;
void WcSetCharEncoding(const char *char_enc) ;
bool WcLowercaseOutput() ;
void WcLowercaseOutput(bool lc) ;

#endif /* !__WORDCLUS_H_INCLUDED */

// end of file wordclus.h //

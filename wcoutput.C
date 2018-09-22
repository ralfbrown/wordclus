/************************************************************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcoutput.cpp	      cluster-output functions			*/
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
#include <cstdio>
#include <cstdlib>

#include "wordclus.h"
#include "wctrmvec.h"

#include "framepac/cluster.h"
#include "framepac/cstring.h"
#include "framepac/file.h"
#include "framepac/stringbuilder.h"
#include "framepac/texttransforms.h"

using namespace Fr ;

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

static void strip_vertical_bars(CharPtr& string)
{
   char* s = (char*)string ;
   char* end = strchr(s,'\0') ;
   if (end[-1] == '|')
      end[-1] = '\0' ;
   if (*s == '|')
      strcpy(s,s+1) ;
   return ;
}

//----------------------------------------------------------------------

inline const char* need_leftbracket(const char* name)
{
   return (strchr(name,'<') == nullptr) ? "<" : "" ;
}

//----------------------------------------------------------------------

inline const char* need_rightbracket(const char* name)
{
   return (strchr(name,'>') == nullptr) ? ">" : "" ;
}

//----------------------------------------------------------------------

static CharPtr extract_source_words(const Symbol *words, std::locale* encoding)
{
   CharPtr source { dup_string(words ? words->c_str() : nullptr) } ;
   if (source && WcLowercaseOutput())
      lowercase_string(source,encoding) ;
   return source ;
}

//----------------------------------------------------------------------

static int cluster_name_lessthan(const Object* o1, const Object* o2)
{
   auto clus1 = static_cast<const ClusterInfo*>(o1) ;
   auto clus2 = static_cast<const ClusterInfo*>(o2) ;
   auto name1 = clus1->label() ;
   auto name2 = clus2->label() ;
   if (!name1)
      return name2 ? 1 : 0 ;
   else if (!name2 || name1 == name2)
      return 0 ;
   return strcmp(name1->c_str(),name2->c_str()) < 0 ;
}

//----------------------------------------------------------------------

static Ptr<Array> sorted_clusters(const ClusterInfo* clusters, bool sort_by_name)
{
   if (!clusters) return nullptr ;
   RefArray* sorted = RefArray::create(clusters->subclusters()) ;
   if (sort_by_name)
      std::sort(sorted->begin(),sorted->end(),cluster_name_lessthan) ;
   return sorted ;
}

//----------------------------------------------------------------------

static int compare_source_word(const Object *o1, const Object *o2)
{
   if (!o1)
      {
      return o2 ? +1 : 0 ;
      }
   else if (!o2)
      {
      return -1 ;
      }
   auto tv1 = static_cast<const WcTermVector*>(o1) ;
   auto tv2 = static_cast<const WcTermVector*>(o2) ;
   auto key1 = tv1->key() ;
   auto key2 = tv2->key() ;
   if (key1 && key2)
      {
      const char* n1 = key1->printableName() ;
      const char* n2 = key2->printableName() ;
      if (!n1) n1 = "" ;
      if (!n2) n2 = "" ;
      int cmp = strcmp(n1,n2) ;
      if (cmp)
	 return cmp ;
      // if the key is the same, we want to order a vector without context before any vectors with context
      bool context1 = tv1->leftConstraint() != nullptr || tv1->rightConstraint() ;
      bool context2 = tv2->leftConstraint() != nullptr || tv2->rightConstraint() ;
      if (!context1 && context2)
	 return -1 ;
      else if (context1 && !context2)
	 return +1 ;
      // among vectors with the same context status, order by frequency
      auto count1 = tv1->weight() ;
      auto count2 = tv2->weight() ;
      if (count1 > count2)
	 return -1 ;
      else if (count1 < count2)
	 return +1 ;
      return 0 ;
      }
   else if (!key1)
      {
      if (key2)
	 return +1 ;
      }
   else // if (!key2)
      return -1 ;
   return o1->compare(o2) ;
}

//----------------------------------------------------------------------

static int source_word_lessthan(const Object* o1, const Object* o2)
{
   return compare_source_word(o1,o2) < 0 ;
}

//----------------------------------------------------------------------

static Ptr<RefArray> sorted_vectors(const ClusterInfo* clusters)
{
   if (!clusters) return nullptr ;
   Ptr<RefArray> sorted { clusters->allMembers() } ;
   if (sorted->size() == 0) return nullptr ;
   std::sort(sorted->begin(),sorted->end(),source_word_lessthan) ;
   return sorted ;
}

//----------------------------------------------------------------------

static void report_cluster_stats(size_t total_pairs, size_t total_clusters,
				 const char* filename = nullptr)
{
   cout << ";   wrote " << total_pairs << " entries in "
	 << total_clusters << " clusters" ;
   if (filename && *filename)
      cout << " to " << filename ;
   cout << "." << endl ;
   return ;
}

/************************************************************************/
/*	Output Functions						*/
/************************************************************************/

static CharPtr make_string(const List* words)
{
   StringBuilder sb ;
   for (const auto w : *words)
      {
      if (sb.size() > 0)
	 sb += ' ' ;
      sb += w->printableName() ;
      }
   return sb.c_str() ;
}

//----------------------------------------------------------------------

static bool write_cluster(CFile& outfp, const char* source, const WcTermVector* vec)
{
   if (source && *source)
      {
      outfp.putc('"') ;
      for ( ; *source ; source++)
	 {
	 if (*source=='"' || *source=='\\') outfp.putc('\\') ;
	 outfp.putc(*source) ;
	 }
      outfp.putc('"') ;
      size_t freq = vec->weight() ;
      const List* left = vec->leftConstraint() ;
      const List* right = vec->rightConstraint() ;
      if (freq || left || right)
	 {
	 outfp.printf("\t%lu",(unsigned long)freq) ;
	 if (left || right)
	    {
	    CharPtr lstring { make_string(left) } ;
	    CharPtr rstring { make_string(right) } ;
	    outfp.printf("\t%s\t%s",*lstring,*rstring) ;
	    }
	 }
      outfp.putc('\n') ;
      return true ;
      }
   return false ;
}

//----------------------------------------------------------------------

void output_cluster(const ClusterInfo* info, CFile& outfp, size_t &total_pairs, size_t &total_clusters)
{
   if (!outfp || !info || info->size() == 0)
      return ;
   auto members = sorted_vectors(info) ;
   if (!members)
      return ;
   const char* cname = info->label() ? info->label()->c_str() : "" ;
   const char* lbracket = need_leftbracket(cname) ;
   const char* rbracket = need_rightbracket(cname) ;
   total_clusters++ ;
   outfp.printf("begin %s%s%s\n",lbracket,cname,rbracket) ;
   std::locale* encoding = WcCurrentCharEncoding() ;
   for (auto v : *members)
      {
      auto tv = static_cast<const WcTermVector*>(v) ;
      if (!tv || !tv->key() /*|| !tv->label()*/)
	 continue ;
      CharPtr src { extract_source_words(tv->key(),encoding) } ;
      if (!src)
	 continue ;
      if (write_cluster(outfp,src,tv))
	 total_pairs++ ;
      }
   outfp.printf("end %s%s%s\n\t;=================\n",lbracket,cname,rbracket);
   outfp.flush() ;
   return ;
}

//----------------------------------------------------------------------

void WcOutputClusters(const ClusterInfo *cluster_info, CFile& outfp, const char *seed_file,
		      bool sort_output, const char *output_filename,
		      bool skip_auto_clusters)
{
   if (!cluster_info || !outfp)
      return ;
   if (!copy_file(seed_file,outfp.fp()))
      outfp.puts("\"EBMT Tokenizations\"\n") ;
   size_t total_pairs(0) ;
   size_t total_clusters(0) ;
   auto clusters = sorted_clusters(cluster_info,sort_output) ;
   for (auto cluster : *clusters)
      {
      auto cl = static_cast<const ClusterInfo*>(cluster) ;
      const char *clustername = cl->label() ? cl->label()->c_str() : nullptr ;
      if (!clustername || !(cl->members() || cl->subclusters()))
	 continue ;
      if (!skip_auto_clusters || !cl->isGeneratedLabel())
	 {
	 output_cluster(cl,outfp,total_pairs,total_clusters) ;
	 }
      }
   outfp.flush() ;
   report_cluster_stats(total_pairs,total_clusters,output_filename) ;
   return ;
}

//----------------------------------------------------------------------

static bool output_token(const ClusterInfo *cluster, CFile& outfp, size_t &total_pairs, size_t &total_clusters,
			 bool suppress_bracket)
{
   if (!outfp || !cluster)
      return false ;
   auto members = sorted_vectors(cluster) ;
   if (!members)
      return true ;
   total_clusters++ ;
   Symbol *name = cluster->label() ;
   const char *clus = name->c_str() ;
   std::locale* encoding = WcCurrentCharEncoding() ;
   for (auto v : *members)
      {
      auto vec = (WcTermVector*)v ;
      if (!vec || !vec->key() || !vec->label())
	 continue ;
      CharPtr src { extract_source_words(vec->key(),encoding) } ;
      if (!src)
	 continue ;
      const char *bracket = suppress_bracket ? "" : need_rightbracket(clus) ;
      outfp.printf("%s\t%s%s",*src,clus,bracket) ;
      const List *left = vec->leftConstraint() ;
      const List *right = vec->rightConstraint() ;
      if (left || right)
	 {
	 CharPtr leftstr { make_string(left) } ;
	 CharPtr rightstr { make_string(right) } ;
	 outfp.printf("\t%s\t%s",*leftstr,*rightstr) ;
	 }
      outfp.putc('\n') ;
      total_pairs++ ;
      }
   outfp.flush() ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcOutputTokenFile(const ClusterInfo *cluster_info, CFile& tokfp, bool sort_output,
		       const char *output_filename, bool skip_auto_clusters,
		       bool suppress_auto_brackets)
{
   if (!cluster_info || !tokfp)
      return ;
   size_t total_pairs(0) ;
   size_t total_clusters(0) ;
   auto clusters = sorted_clusters(cluster_info,sort_output) ;
   for (auto cluster : *clusters)
      {
      auto cl = static_cast<const ClusterInfo*>(cluster) ;
      const char *clustername = cl->label() ? cl->label()->c_str() : nullptr ;
      // skip empty clusters and clusters with no name
      if (!clustername || !(cl->members() || cl->subclusters()))
	 continue ;
      if (!skip_auto_clusters || !cl->isGeneratedLabel())
	 {
	 output_token(cl,tokfp,total_pairs,total_clusters,suppress_auto_brackets) ;
	 }
      }
   tokfp.flush() ;
   report_cluster_stats(total_pairs,total_clusters,output_filename) ;
   return ;
}

//----------------------------------------------------------------------

static void output_entry(CFile& outfp, const char* clus, const char* bracket,
			 const WcTermVector* vec, const char* src, const char* trg)
{
   outfp.printf(";;; (TOKEN %s%s)",clus,bracket);
   size_t freq = vec->weight() ;
   if (freq > 1)
      outfp.printf("(FREQ %lu)",(unsigned long)freq) ;
   const List *left = vec->leftConstraint() ;
   const List *right = vec->rightConstraint() ;
   if (left)
      {
      CharPtr leftstr { make_string(left) } ;
      outfp.printf("(LEFT %s)",*leftstr) ;
      }
   if (right)
      {
      CharPtr rightstr { make_string(right) } ;
      outfp.printf("(RIGHT %s)",*rightstr) ;
      }
   outfp.printf("\n%s\n%s\n\n",src,trg);
   return ;
}

//----------------------------------------------------------------------

static bool output_tagged(const ClusterInfo *cluster, CFile& outfp,
			  size_t &total_pairs, size_t &total_clusters)
{
   if (!outfp || !cluster)
      return false ;
   auto members = sorted_vectors(cluster) ;
   if (!members)
      return true ;
   total_clusters++ ;
   auto name = static_cast<const Symbol*>(cluster->front()) ;
   const char *clus = name->c_str() ;
   auto char_enc = WcCurrentCharEncoding() ;
   for (auto v : *members)
      {
      auto vec = (WcTermVector*)v ;
      if (!vec || !vec->key() || !vec->label())
	 continue ;
      CharPtr src { extract_source_words(vec->key(),char_enc) } ;
      if (!src)
	 continue ;
      strip_vertical_bars(src) ;
      const char *bracket = need_rightbracket(clus) ;
      output_entry(outfp,clus,bracket,vec,src,src) ;
      total_pairs++ ;
      }
   outfp.flush() ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcOutputTaggedCorpus(const ClusterInfo *cluster_info, CFile& tagfp, bool sort_output,
			  const char *output_filename, bool skip_auto_clusters)
{
   if (!cluster_info || !tagfp)
      return;
   size_t total_pairs(0) ;
   size_t total_clusters(0) ;
   (void)skip_auto_clusters;
   auto clusters = sorted_clusters(cluster_info,sort_output) ;
   for (auto cluster : *clusters)
      {
      auto cl = static_cast<const ClusterInfo*>(cluster) ;
      const char *clustername = cl->label() ? cl->label()->c_str() : nullptr ;
      // skip empty clusters and clusters with no name
      if (!clustername || !(cl->members() || cl->subclusters()))
	 continue ;
      if (!skip_auto_clusters || !cl->isGeneratedLabel())
	 {
	 output_tagged(cl,tagfp,total_pairs,total_clusters) ;
	 }
      }
   tagfp.flush() ;
   report_cluster_stats(total_pairs,total_clusters,output_filename) ;
   return ;
}

// end of file wcoutput.cpp //

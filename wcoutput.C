/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcoutput.cpp	      cluster-output functions			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2005,2006,2008,2009,2010,2015	*/
/*		 Ralf Brown/Carnegie Mellon University			*/
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

#include <stdio.h>
#include <stdlib.h>
#include "wcconfig.h"
#include "wordclus.h"
#include "wcglobal.h"

/************************************************************************/
/*	Helper Functions						*/
/************************************************************************/

static void strip_vertical_bars(char *string)
{
   char *end = strchr(string,'\0') ;
   if (end[-1] == '|')
      end[-1] = '\0' ;
   if (*string == '|')
      strcpy(string,string+1) ;
   return ;
}

//----------------------------------------------------------------------

inline const char *need_leftbracket(const char *name)
{
   return (strchr(name,'<') == 0) ? "<" : "" ;
}

//----------------------------------------------------------------------

inline const char *need_rightbracket(const char *name)
{
   return (strchr(name,'>') == 0) ? ">" : "" ;
}

//----------------------------------------------------------------------

static int compare_source_word(const FrObject *o1, const FrObject *o2)
{
   if (o1 && o2)
      {
      FrSymbol *w1 = ((WcTermVector*)o1)->key() ;
      FrSymbol *w2 = ((WcTermVector*)o2)->key() ;
      if (w1 && w2)
	 {
	 int cmp ;
	 if (w1->consp() && w2->consp())
	    {
	    cmp = w1->compare(w2) ;
	    }
	 else
	    {
	    const char *n1 = w1->printableName() ;
	    const char *n2 = w2->printableName() ;
	    if (!n1)
	       n1 = "" ;
	    if (!n2)
	       n2 = "" ;
	    cmp = strcmp(n1,n2) ;
	    }
	 if (cmp)
	    return cmp ;
	 size_t count1 = ((WcTermVector*)o1)->vectorFreq() ;
	 size_t count2 = ((WcTermVector*)o2)->vectorFreq() ;
	 if (count1 > count2)
	    return -1 ;
	 else if (count1 < count2)
	    return +1 ;
	 }
      else if (!w1)
	 {
	 if (w2)
	    return +1 ;
	 }
      else // if (!w2)
	 return -1 ;
      return o1->compare(o2) ;
      }
   else if (!o1)
      return +1 ;
   else if (!o2)
      return -1 ;
   return 0 ;
}

//----------------------------------------------------------------------

static const char *extract_source_word(const FrList *pair)
{
   FrList *srcwords = (FrList*)pair->first() ;
   if (srcwords)
      {
      FrString src(srcwords) ;
      return (FrSymbolTable::add((char*)src.stringValue()))->symbolName() ;
      }
   return 0 ;
}

//----------------------------------------------------------------------

static bool same_source_word(const WcTermVector *tv1,
			     const WcTermVector *tv2)
{
   if (tv1 && tv2)
      {
      FrSymbol *key1 = tv1->key() ;
      FrSymbol *key2 = tv2->key() ;
      const char *name1 = key1->symbolName() ;
      const char *name2 = key2->symbolName() ;
      if (key1 && key1->consp())
	 name1 = extract_source_word((FrList*)key1) ;
      if (key2 && key2->consp())
	 name2 = extract_source_word((FrList*)key2) ;
      if (!name1 || !name2)
	 return false ;
      const char *sep1 = strchr(name1,WC_CONCAT_WORDS_SEP) ;
      const char *sep2 = strchr(name2,WC_CONCAT_WORDS_SEP) ;
      if (sep1 && sep2)
	 {
	 size_t len1 = sep1 - name1 ;
	 size_t len2 = sep2 - name2 ;
	 if (len1 == len2)
	    return strncmp(name1,name2,len1) == 0 ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static FrList *remove_duplicates(FrList *cluster)
{
   // assumes cluster members are sorted by source word
   if (remove_ambig_terms)
      {
      FrList *result = nullptr ;
      FrList **end = &result ;
      while (cluster)
	 {
	 WcTermVector *tv = (WcTermVector*)poplist(cluster) ;
	 WcTermVector *tv2 = cluster ? (WcTermVector*)cluster->first() : 0 ;
	 if (same_source_word(tv,tv2))
	    {
	    // remove all but the first translation for a particular word
	    do {
	       (void)poplist(cluster) ;
	       tv2 = cluster ? (WcTermVector*)cluster->first() : 0 ;
	       } while (same_source_word(tv,tv2)) ;
	    }
	 result->pushlistend(tv,end) ;
	 }
      *end = nullptr ;	// properly terminate the list
      if (result && result->rest() == nullptr && !keep_singletons)
	 {
	 (void)poplist(result) ;
	 result = nullptr ;
	 }
      return result ;
      }
   else
      return cluster ; 
}

//----------------------------------------------------------------------

static void report_cluster_stats(size_t total_pairs, size_t total_clusters,
				 const char *filename = 0)
{
   cout << ";   wrote " << total_pairs << " entries in "
	 << total_clusters << " clusters" ;
   if (filename && *filename)
      cout << " to " << filename ;
   cout << "." << endl ;
   return ;
}

//----------------------------------------------------------------------

static long quick_strtol(const char *line)
{
   long value(0) ;
   while (Fr_isdigit(*line))
      {
      value = 10 * value + (*line - '0') ;
      line++ ;
      }
   return value ;
}

//----------------------------------------------------------------------

static int compare_cluster_names(const FrObject *o1, const FrObject *o2)
{
   FrSymbol *sym1 = (FrSymbol*)((FrList*)o1)->first() ;
   FrSymbol *sym2 = (FrSymbol*)((FrList*)o2)->first() ;
   if (!sym1)
      return sym2 ? -1 : 0 ;
   else if (!sym2)
      return +1 ;
   const char *name1 = sym1->symbolName() ;
   const char *name2 = sym2->symbolName() ;
   size_t prefixlen = sizeof(FrGENERATED_CLUSTER_PREFIX) - 1 ;
   if (strncmp(name1,FrGENERATED_CLUSTER_PREFIX,prefixlen) == 0 &&
       strncmp(name2,FrGENERATED_CLUSTER_PREFIX,prefixlen) == 0)
      {
      long num1 = quick_strtol(name1+prefixlen) ;
      long num2 = quick_strtol(name2+prefixlen) ;
      return num1 - num2 ;
      }
   return strcmp(name1,name2) ;
}

//----------------------------------------------------------------------

static bool is_auto_cluster(const FrList *cluster)
{
   FrSymbol *name = (FrSymbol*)cluster->first() ;
   const char *namestr = name->symbolName() ;
   size_t prefixlen = sizeof(FrGENERATED_CLUSTER_PREFIX) - 1 ;
   return strncmp(namestr,FrGENERATED_CLUSTER_PREFIX,prefixlen) == 0 ;
}

/************************************************************************/
/*	Output Functions						*/
/************************************************************************/

static bool write_cluster(FILE *outfp, const char *source, const char *target,
			  size_t freq, bool monoling)
{
   if (source && *source)
      {
      if (target && *target)
	 {
	 if (reverse_tokens)
	    {
	    const char *tmp = source ;
	    source = target ;
	    target = tmp ;
	    }
	 fputc('"',outfp) ;
	 for ( ; *source ; source++)
	    {
	    if (*source=='"' || *source=='\\') fputc('\\',outfp) ;
	    fputc(*source,outfp) ;
	    }
	 fputs("\"\t\t\"",outfp) ;
	 for ( ; *target ; target++)
	    {
	    if (*target=='"' || *target=='\\') fputc('\\',outfp) ;
	    fputc(*target,outfp) ;
	    }
	 fputc('"',outfp) ;
	 if (freq)
	    fprintf(outfp,"\t%lu",(unsigned long)freq) ;
	 fputc('\n',outfp) ;
	 return true ;
	 }
      else if (monoling)
	 {
	 fputc('"',outfp) ;
	 for ( ; *source ; source++)
	    {
	    if (*source=='"' || *source=='\\') fputc('\\',outfp) ;
	    fputc(*source,outfp) ;
	    }
	 fputc('"',outfp) ;
	 if (freq)
	    fprintf(outfp,"\t%lu",(unsigned long)freq) ;
	 fputc('\n',outfp) ;
	 return true ;
	 }
      else if (verbose)
	 {
	 if (target)
	    fprintf(outfp,";ERR:  %s => \"\"\n",source) ;
	 else
	    fprintf(outfp,";ERR:  %s\n",source) ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static char *extract_source_words(FrObject *words, FrCharEncoding encoding,
				  bool monoling)
{
   char *source = nullptr ;
   if (words)
      {
      if (words->consp())
	 {
	 const FrList *pair = (FrList*)words ;
	 const FrList *srcwords = (FrList*)pair->first() ;
	 if (srcwords)
	    {
	    FrString *src = new FrString(srcwords) ;
	    if (src)
	       {
	       source = FrDupString(src->stringValue()) ;
	       src->freeObject() ;
	       }
	    }
	 }
      else
	 {
	 const char *name = words->printableName() ;
	 if (name)
	    {
	    char buf[FrMAX_SYMBOLNAME_LEN+1] ;
	    strcpy(buf,name) ;
	    char *blank = strchr(buf,WC_CONCAT_WORDS_SEP) ;
	    if (blank)
	       {
	       *blank = '\0' ;
	       source = FrDupString(buf) ;
	       }
	    else if (monoling)
	       {
	       source = FrDupString(buf) ;
	       }
	    }
	 }
      }
   if (source && lowercase_output)
      Fr_strlwr(source,encoding) ;
   return source ;
}

//----------------------------------------------------------------------

static char *extract_target_words(FrObject *words, FrCharEncoding encoding)
{
   char *target = nullptr ;
   if (words)
      {
      if (words->consp())
	 {
	 const FrList *pair = (FrList*)words ;
	 const FrList *trgwords = (FrList*)pair->second() ;
	 if (trgwords)
	    {
	    FrString *trg = new FrString(trgwords) ;
	    if (trg)
	       {
	       target = FrDupString(trg->stringValue()) ;
	       trg->freeObject() ;
	       }
	    }
	 }
      else
	 {
	 const char *name = words->printableName() ;
	 if (name)
	    {
	    char buf[FrMAX_SYMBOLNAME_LEN+1] ;
	    strcpy(buf,name) ;
	    char *blank = strchr(buf,WC_CONCAT_WORDS_SEP) ;
	    if (blank)
	       target = FrDupString(blank + 1) ;
	    }
	 }
      }
   if (target && lowercase_output)
      Fr_strlwr(target,encoding) ;
   return target ;
}

//----------------------------------------------------------------------

void output_cluster(FrList *cluster, FILE *outfp,
		    size_t &total_pairs, size_t &total_clusters,
		    bool monoling)
{
   assertq(outfp && cluster && cluster->consp()) ;
   FrSymbol *name = (FrSymbol*)cluster->first() ;
   cluster = cluster->rest() ;		// skip the cluster ID
   cluster = cluster ? (FrList*)cluster->copy() : 0 ;
   const char *cname = name ? name->symbolName() : "" ;
   cluster = cluster->sort(compare_source_word) ;
   cluster = remove_duplicates(cluster) ;
   if (!cluster)
      return ;
   const char *lbracket = need_leftbracket(cname) ;
   const char *rbracket = need_rightbracket(cname) ;
   total_clusters++ ;
   fprintf(outfp,"begin %s%s%s\n",lbracket,cname,rbracket) ;
   FrCharEncoding encoding = WcCurrentCharEncoding() ;
   while (cluster)
      {
      WcTermVector *vec = (WcTermVector*)poplist(cluster) ;
      if (!vec)
	 continue ;
      FrSymbol *words = (FrSymbol*)vec->key() ;
      size_t freq = vec->vectorFreq() ;
      if (words)
	 {
	 char *src = extract_source_words(words,encoding,monoling) ;
	 if (!src)
	    continue ;
	 char *trg = extract_target_words(words,encoding) ;
	 if (write_cluster(outfp,src,trg,freq,monoling))
	    total_pairs++ ;
	 FrFree(src) ;
	 FrFree(trg) ;
	 }
      }
   fprintf(outfp,"end %s%s%s\n\t;=================\n",lbracket,cname,rbracket);
   fflush(outfp) ;
   return ;
}

//----------------------------------------------------------------------

void WcOutputClusters(const FrList *cluster_list, FILE *outfp,
		      const char *seed_file, bool monoling,
		      bool sort_output, const char *output_filename,
		      bool skip_auto_clusters)
{
   if (cluster_list && outfp)
      {
      if (!FrCopyFile(seed_file,outfp))
	 fputs("\"EBMT Tokenizations\"\n",outfp) ;
      FrList *clusters = (FrList*)cluster_list->copy() ;
      if (sort_output)
	 clusters = clusters->sort(compare_cluster_names) ;
      size_t total_pairs(0) ;
      size_t total_clusters(0) ;
      while (clusters)
	 {
	 FrList *cluster = (FrList*)poplist(clusters) ;
	 FrSymbol *clustername = (FrSymbol*)cluster->first() ;
	 if (!cluster->rest() || !clustername || !*clustername->symbolName())
	    continue ;
	 if (!skip_auto_clusters || !is_auto_cluster(cluster))
	    {
	    output_cluster(cluster,outfp,total_pairs,total_clusters,monoling) ;
	    }
	 }
      fflush(outfp) ;
      report_cluster_stats(total_pairs,total_clusters,output_filename) ;
      }
   return ;
}

//----------------------------------------------------------------------

static bool output_token(FrList *cluster, FILE *outfp,
			 size_t &total_pairs, size_t &total_clusters,
			 bool monoling, bool suppress_bracket)
{
   assertq(outfp && cluster && cluster->consp()) ;
   FrSymbol *name = (FrSymbol*)cluster->first() ;
   cluster = cluster->rest() ;
   cluster = cluster ? ((FrList*)cluster->copy())->sort(compare_source_word) : 0 ;
   cluster = remove_duplicates(cluster) ;
   if (!cluster)
      return true ;
   total_clusters++ ;
   char *clus = name->print() ;
   FrCharEncoding encoding = WcCurrentCharEncoding() ;
   while (cluster)
      {
      WcTermVector *vec = (WcTermVector*)poplist(cluster) ;
      if (!vec)
	 continue ;
      FrSymbol *words = (FrSymbol*)vec->key() ;
      if (words)
	 {
	 char *src = extract_source_words(words,encoding,monoling) ;
	 if (!src)
	    continue ;
	 const char *bracket = suppress_bracket ? "" : need_rightbracket(clus) ;
	 char *trg = extract_target_words(words,encoding) ;
	 if (trg)
	    {
	    fprintf(outfp,"%s\t%s%s\t%s\n",src,clus,bracket,trg) ;
	    total_pairs++ ;
	    }
	 else if (monoling)
	    {
	    fprintf(outfp,"%s\t%s%s\n",src,clus,bracket);
	    total_pairs++ ;
	    }
	 else if (verbose)
	    fprintf(outfp,";ERR: missing source and/or target in cluster %s\n",clus) ;
	 FrFree(src) ;
	 FrFree(trg) ;
	 }
      }
   FrFree(clus) ;
   fflush(outfp) ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcOutputTokenFile(const FrList *cluster_list, FILE *tokfp,
		       bool monoling, bool sort_output,
		       const char *output_filename, bool skip_auto_clusters,
		       bool suppress_auto_brackets)
{
   if (cluster_list && tokfp)
      {
      FrList *clusters = (FrList*)cluster_list->copy() ;
      if (sort_output)
	 clusters = clusters->sort(compare_cluster_names) ;
      size_t total_pairs(0) ;
      size_t total_clusters(0) ;
      while (clusters)
	 {
	 FrList *cluster = (FrList*)poplist(clusters) ;
	 FrSymbol *clustername = (FrSymbol*)cluster->first() ;
	 // skip empty clusters and clusters with no name
	 if (!cluster->rest() || !clustername || !*clustername->symbolName())
	    continue ;
	 if (!skip_auto_clusters || !is_auto_cluster(cluster))
	    {
	    output_token(cluster,tokfp,total_pairs,total_clusters,monoling,suppress_auto_brackets) ;
	    }
	 }
      fflush(tokfp) ;
      report_cluster_stats(total_pairs,total_clusters,output_filename) ;
      }
   return ;
}

//----------------------------------------------------------------------

static void output_entry(FILE *outfp, const char *clus, const char *bracket,
			 size_t freq, const char *src, const char *trg)
{
   fprintf(outfp,";;; (TOKEN %s%s)",clus,bracket);
   if (freq > 1)
      fprintf(outfp,"(FREQ %lu)",(unsigned long)freq) ;
   fprintf(outfp,"\n%s\n%s\n\n",src,trg);
   return ;
}

//----------------------------------------------------------------------

static bool output_tagged(FrList *cluster, FILE *outfp,
			  size_t &total_pairs, size_t &total_clusters,
			  bool monoling)
{
   assertq(outfp && cluster && cluster->consp()) ;
   FrSymbol *name = (FrSymbol*)cluster->first() ;
   cluster = cluster->rest() ;
   cluster = cluster ? ((FrList*)cluster->copy())->sort(compare_source_word) : 0 ;
   cluster = remove_duplicates(cluster) ;
   if (!cluster)
      return true ;
   total_clusters++ ;
   char *clus = name->print() ;
   while (cluster)
      {
      char buf[FrMAX_SYMBOLNAME_LEN+1] ;
      WcTermVector *vec = (WcTermVector*)poplist(cluster) ;
      if (!vec)
	 continue ;
      FrSymbol *words = (FrSymbol*)vec->key() ;
      size_t freq = vec->vectorFreq() ;
      if (words)
	 {
	 char *src = extract_source_words(words,char_encoding,monoling) ;
	 if (!src)
	    continue ;
	 strip_vertical_bars(src) ;
	 char *trg = extract_target_words(words,char_encoding) ;
	 const char *bracket = need_rightbracket(clus) ;
	 if (trg)
	    {
	    strip_vertical_bars(trg) ;
	    output_entry(outfp,clus,bracket,freq,src,trg) ;
	    total_pairs++ ;
	    }
	 else if (monoling)
	    {
	    output_entry(outfp,clus,bracket,freq,src,src) ;
	    total_pairs++ ;
	    }
	 else
	    fprintf(outfp,";;; (COMMENT \"ERR:  %s\")\n",buf) ;
	 FrFree(src) ;
	 FrFree(trg) ;
	 }
      }
   FrFree(clus) ;
   fflush(outfp) ;
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

void WcOutputTaggedCorpus(const FrList *cluster_list, FILE *tagfp,
			  bool monoling, bool sort_output,
			  const char *output_filename, bool skip_auto_clusters)
{
   if (cluster_list && tagfp)
      {
      FrList *clusters = (FrList*)cluster_list->copy() ;
      if (sort_output)
	 clusters = clusters->sort(compare_cluster_names) ;
      size_t total_pairs(0) ;
      size_t total_clusters(0) ;
      while (clusters)
	 {
	 FrList *cluster = (FrList*)poplist(clusters) ;
	 FrSymbol *clustername = (FrSymbol*)cluster->first() ;
	 // skip empty clusters and clusters with no name
	 if (!cluster->rest() || !clustername || !*clustername->symbolName())
	    continue ;
	 if (!skip_auto_clusters || !is_auto_cluster(cluster))
	    {
	    output_tagged(cluster,tagfp,total_pairs,total_clusters,monoling) ;
	    }
	 }
      fflush(tagfp) ;
      report_cluster_stats(total_pairs,total_clusters,output_filename) ;
      }
   return ;
}

// end of file wcoutput.cpp //

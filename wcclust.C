/************************************************************************/
/*									*/
/*  WordClust -- Bilingual Word Clustering				*/
/*  Version 1.40							*/
/*	 by Ralf Brown							*/
/*									*/
/*  File: wcclust.cpp	      term-vector clustering			*/
/*  LastEdit: 08nov2015							*/
/*									*/
/*  (c) Copyright 1999,2000,2001,2002,2005,2006,2009,2015		*/
/*	   Ralf Brown/Carnegie Mellon University			*/
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

#include "wordclus.h"
#include "wcglobal.h"

/************************************************************************/
/*	Compile-Time Configuration					*/
/************************************************************************/

/************************************************************************/
/*	Global variables						*/
/************************************************************************/

bool use_multiple_passes = false ;

static FrSymbol *symNUMBER = nullptr ;

/************************************************************************/
/************************************************************************/

static char *source_word(FrObject *key)
{
   static char source[FrMAX_SYMBOLNAME_LEN+1] ;
   if (key)
      {
      const char *words = key->printableName() ;
      const char *end = strchr(words,WC_CONCAT_WORDS_SEP) ;
      if (!end)
	 end = strchr(words,'\0') ;
      memcpy(source,words,end-words) ;
      source[end-words] = '\0' ;
      }
   else
      source[0] = '\0' ;
   return source ;
}

//----------------------------------------------------------------------

static bool contains_digit(FrSymbol *wordpair)
{
   if (wordpair)
      {
      const char *string = wordpair->symbolName() ;
      for ( ; *string ; string++)
	 {
	 if (Fr_isdigit(*string))
	    return true ;
	 }
      }
   return false ;
}

//----------------------------------------------------------------------

static bool cluster_conflict(const FrTermVector *tv1,
			     const FrTermVector *tv2)
{
   if (keep_numbers_distinct)
      {
      FrSymbol *cluster1 = tv1->cluster() ;
      FrSymbol *cluster2 = tv2->cluster() ;
      if (cluster1 == symNUMBER && !cluster2)
	 return (!contains_digit(tv2->key()) &&
		 !FrIsGeneratedClusterName(tv2->key())) ;
      else if (cluster2 == symNUMBER && !cluster1)
	 return (!contains_digit(tv1->key()) &&
		 !FrIsGeneratedClusterName(tv1->key())) ;
      }
   if (keep_punctuation_distinct && tv1->key() && tv2->key() &&
       is_punct(tv1->key()) != is_punct(tv2->key()))
      return true ;
   return false ;
}

//----------------------------------------------------------------------

static bool is_seed(const FrSymbol *word, const FrObjHashTable *seeds)
{
   if (seeds && word)
      {
      FrString str(word->symbolName()) ;
      return seeds->contains(&str) ;
      }
   return false ;
}

//----------------------------------------------------------------------

static bool collect_key_words(const FrSymbol *word, FrObject *vec, va_list args)
{
   FrVarArg(FrList **,words) ;
   const WcTermVector *tv = (WcTermVector*)vec ;
   if (words && tv)
      {
      FrVarArg(size_t,min_freq) ;
      FrVarArg(size_t,max_freq) ;
      FrVarArg(const FrObjHashTable*,seeds) ;
      FrVarArg(size_t*,highest_freq) ;
      size_t count = tv->vectorFreq() ;
      if (highest_freq && count > *highest_freq)
	 *highest_freq = count ;
      if (is_seed(word,seeds))
	 {
	 pushlist(new FrCons(word,new FrInteger(count)),*words) ;
	 }
      else if (count >= min_freq && count <= max_freq &&
	  (!exclude_numbers || !is_compound_number(word)))
	 {
	 pushlist(new FrCons(word,new FrInteger(count)),*words) ;
	 }
      }
   return true ;			// continue iterating
}

//----------------------------------------------------------------------

static FrList *strip_counts(FrList *counts)
{
   FrList *result = nullptr ;
   while (counts)
      {
      FrObject *o = poplist(counts) ;
      if (o && o->consp())
	 {
	 FrCons *c = (FrCons*)o ;
	 pushlist(c->first(),result) ;
	 free_object(c->consCdr()) ;
	 c->replaca(0) ;
	 free_object(c) ;
	 }
      else
	 pushlist(o,result) ;
      }
   return listreverse(result) ;
}

//----------------------------------------------------------------------

static int compare_key_counts(const FrObject *o1, const FrObject *o2)
{
   if (o1 && o2)
      {
      if (o1->consp() && o2->consp())
	 {
	 size_t count1 = ((FrCons*)o1)->consCdr()->intValue() ;
	 size_t count2 = ((FrCons*)o2)->consCdr()->intValue() ;
	 if (count1 > count2)
	    return -1 ;
	 else if (count1 < count2)
	    return +1 ;
	 else
	    {
	    FrSymbol *word1 = (FrSymbol*)((FrCons*)o1)->first() ;
	    FrSymbol *word2 = (FrSymbol*)((FrCons*)o2)->first() ;
	    if (word1 < word2)
	       return -1 ;
	    else if (word1 > word2)
	       return +1 ;
	    }
	 }
      else
	 return o1->compare(o2) ;
      }
   else if (o1)
      return -1 ;
   else if (o2)
      return +1 ;
   return 0 ;
}

//----------------------------------------------------------------------

static FrList *move_to_front(FrList *keywords, FrObjHashTable *priority,
			     bool run_verbosely)
{
   if (!priority || priority->currentSize() == 0)
      return keywords ;
   if (run_verbosely)
      cout << ";   moving seeds to front of word list" << endl ;
   FrList *hi_pri = nullptr ;
   FrList *lo_pri = nullptr ;
   while (keywords)
      {
      FrObject *w = poplist(keywords) ;
      if (!w)
	 continue ;
      FrString *word ;
      if (w->stringp())
	 word = (FrString*)w ;
      else
	 {
	 word = new FrString(w->printableName()) ;
	 free_object(w) ;
	 }
      if (priority->contains(word))
	 pushlist(word,hi_pri) ;
      else
	 pushlist(word,lo_pri) ;
      }
   return listreverse(hi_pri)->nconc(listreverse(lo_pri)) ;
}

//----------------------------------------------------------------------

static double split_cosine(const FrTermVector *tv,
			   const FrList *cluster1,
			   const FrList *cluster2, double /*best_sim*/, void */*userdata*/,
			   size_t &min_freq)
{
   (void)min_freq ;
   if (tv && cluster2)
      {
      // compare similarity between term vector and centroid of cluster2
      WcTermVector *centroid = (WcTermVector*)cluster2->first() ;
      if (centroid)
	 {
	 double lsim = centroid->splitCosine(tv,-1,1) ;
	 double rsim = centroid->splitCosine(tv,+1,1) ;
	 return 2.0 * lsim * rsim / (lsim + rsim) ;
	 }
      }
   else if (cluster1 && cluster2)
      {
      // compare similarities between centroids of the two clusters
      WcTermVector *cent1 = (WcTermVector*)cluster1->first() ;
      WcTermVector *cent2 = (WcTermVector*)cluster2->first() ;
      if (cent1 && cent2)
	 {
	 double lsim = cent1->splitCosine(cent2,-1,1) ;
	 double rsim = cent1->splitCosine(cent2,+1,1) ;
	 return 2.0 * lsim * rsim / (lsim + rsim) ;
	 }
      }
   return 0.0 ;
}

//----------------------------------------------------------------------

static double split_cosine_tv(const FrTermVector *tv1, const FrTermVector *tv2, void * /*user_data*/)
{
   if (tv1 && tv2)
      {
      const WcTermVector *w1 = (WcTermVector*)tv1 ;
      const WcTermVector *w2 = (WcTermVector*)tv2 ;
      double lsim = w1->splitCosine(w2,-1,1) ;
      double rsim = w1->splitCosine(w2,+1,1) ;
      return 2.0 * lsim * rsim / (lsim + rsim) ;
      }
   return 0.0 ;
}

//----------------------------------------------------------------------
// key_words is a mapping from compound-word to WcTermVector, while
// seeds is a mapping from compound-word to equivalence-class-name

FrList *cluster_vectors(FrSymHashTable *key_words,
			const WcParameters *global_params,
			FrThresholdList *thresholds, FrObjHashTable *seeds,
			bool run_verbosely,
			bool exclude_singletons)
{
   if (!key_words)
      return 0 ;
   FrClusteringSimilarityFunc *usersim = nullptr ;
   FrTermVectorSimilarityFunc *tvsim = nullptr ;
   void *tvsimdata = nullptr ;
   void *userdata = nullptr ;
   if (clustering_metric == FrCM_USER)
      {
      usersim = global_params->clusterSimFunc() ;
      if (usersim)
	 {
	 userdata = global_params->clusterSimData() ;
	 }
      else
	 {
	 usersim = split_cosine ;
	 }
      tvsim = global_params->tvSimFunc() ;
      if (tvsim)
	 {
	 tvsimdata = global_params->tvSimData() ;
	 }
      else
	 {
	 tvsim = split_cosine_tv ;
	 }
      }
   FrClusteringParameters params(clustering_method,clustering_rep,
				 clustering_metric,1.0,usersim,
				 cluster_conflict,thresholds,
				 global_params->desiredClusters(),
				 sum_cluster_sizes) ;
   params.maxIterations(clustering_iter) ;
   params.tvUserSimFunc(tvsim,tvsimdata) ;
   params.userSimFunc(usersim,userdata) ;
   params.numThreads(global_params->numThreads()) ;
   params.hardClusterLimit(global_params->hardClusterLimit()) ;
   params.ignoreBeyondClusterLimit(global_params->ignoreBeyondClusterLimit()) ;
   if (clustering_settings)
      {
      (void)FrParseClusteringParams(clustering_settings,&params) ;
      if (params.desiredClusters() == 0)
	 {
	 size_t desired_clusters = global_params->desiredClusters() ;
	 if (desired_clusters == 0)
	    desired_clusters = (2*key_words->currentSize()+2) / 3 ;
	 else if (desired_clusters > key_words->currentSize())
	    desired_clusters = key_words->currentSize() ;
	 params.desiredClusters(desired_clusters) ;
	 }
      }
   FrList *vectors = nullptr ;
   FrList *kw_list = nullptr ;
   symNUMBER = FrSymbolTable::add(NUMBER_TOKEN) ;
   cout << ";   sorting by term frequency" << endl ;
   size_t highest_freq = 0 ;
   size_t min_freq = global_params->minWordFreq() ;
   size_t max_freq = global_params->maxWordFreq() ;
   key_words->iterate(collect_key_words,&kw_list,min_freq,max_freq,
		      seeds,&highest_freq) ;
   kw_list = strip_counts(kw_list->sort(compare_key_counts)) ;
   // remove the 'stop_terms' highest-frequency terms from clustering
   for (size_t i = 0 ; i < global_params->stopTermCount() && kw_list ; i++)
      {
      free_object(poplist(kw_list)) ;
      }
   if (!kw_list)
      {
      cout << ";   frequency limits have removed all candidates "
	   << "(highest freq is " << highest_freq << ")"
	   << endl ;
      return 0 ;
      }
   cout << ";   " << kw_list->listlength()
        << " terms to be placed in " << params.desiredClusters()
	<< " clusters" << endl ;
   kw_list = move_to_front(kw_list,seeds,run_verbosely) ;
   size_t max_terms = global_params->maxTermCount() ;
   if (max_terms > 0 && max_terms < kw_list->simplelistlength())
      {
      cout << ";   (limiting clustering to " << max_terms << " term vectors)"
	   << endl ;
      (void)kw_list->elide(max_terms,(size_t)~0) ;
      }
   bool ignore_unseen_seeds = global_params->ignoreUnseenSeeds() ;
   FrList *allseeds = (seeds && !ignore_unseen_seeds) ? seeds->allKeys() : 0 ;
   while (kw_list)
      {
      FrString *keyword = (FrString*)poplist(kw_list) ;
      FrSymbol *keysym = FrSymbolTable::add(keyword->printableName()) ;
      FrObject *tv_obj ;
      if (key_words->lookup(keysym,&tv_obj))
	 {
	 WcTermVector *tv = (WcTermVector*)tv_obj ;
	 FrObject *name_obj ;
	 if (seeds && seeds->lookup(keyword,&name_obj))
	    {
	    FrSymbol *name = (FrSymbol*)name_obj ;
	    tv->setCluster(name) ;
	    allseeds = listremove(allseeds,keyword,::equal) ;
	    }
	 else if (is_number(source_word(keyword)))
	    {
	    if (!exclude_numbers)
	       tv->setCluster(symNUMBER) ;
	    }
	 else
	    tv->setCluster(0) ;
	 pushlist(tv,vectors) ;
	 }
      free_object(keyword) ;
      }
   while (allseeds)
      {
      // insert dummy term vectors for any seeds which didn't actually occur
      // in the training data
      FrString *seed = (FrString*)poplist(allseeds) ;
      FrSymbol *seedsym = FrCvt2Symbol(seed) ;
      FrObject *kw_entry ;
      if (key_words->lookup(seedsym,&kw_entry) && kw_entry)
	 continue ;			// already have a term vector for this seed word
      WcTermVector *tv = new WcTermVector ;
      if (!tv)
	 {
	 FrNoMemory("allocating term vector for seed word") ;
	 break ;
	 }
      FrObject *clusname ;
      (void)seeds->lookup(seed,&clusname) ;
      free_object(seed) ;
      tv->setKey(seedsym) ;
      tv->setCluster((FrSymbol*)clusname) ;
      pushlist(tv,vectors) ;
      // add the new term vector to the keywords hash table, so that it gets
      //   freed when we're done clustering
      key_words->add(seedsym,tv) ;
      }
   vectors = listreverse(vectors) ;
   FrList *clusters = FrClusterVectors(vectors, &params, exclude_singletons,
				       /*run_verbosely=*/true,
				       /*copy_vectors=*/false) ;
   vectors->eraseList(false) ;
   return clusters ;
}

// end of file wcclust.cpp //

/****************************** -*- C++ -*- *****************************/
/*									*/
/*  WordClust -- Word Clustering					*/
/*  Version 2.00							*/
/*	by Ralf Brown							*/
/*									*/
/*  File: idhash.C							*/
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

#include "template/hashtable.cc"

namespace Fr
{

// request explicit instantiation
template class HashTable<unsigned,size_t> ;

} // end namespace Fr

// end of file wcidhash.C //



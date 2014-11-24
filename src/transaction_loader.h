/*
 Copyright (c) 2013, 2014, Mohamed Ilyes Dimassi.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 3. All advertising materials mentioning features or use of this software
 must display the following acknowledgement:
 This product includes software developed by the Mohamed Ilyes Dimassi.
 4. Neither the name of the FST http://www.fst.rnu.tn/ nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY Mohamed Ilyes Dimassi ''AS IS'' AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL Mohamed Ilyes Dimassi BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TRANS_LOADER_H_
#define TRANS_LOADER_H_

#include "limits.h"
#include "time.h"
#include "gmp.h"

////////////////////////////////////////////////////////////////////////////////
#define CONCEPTS_COUNT ( 3316 )

#define MAX_CXT_LINE_ITEMS ( 75 )

#define LIMBS_MAX_COUNT ( 3 )

#define TRANS_MAX_COUNT ( 3196 )
////////////////////////////////////////////////////////////////////////////////

//at most 10 characters per item including spaces, more than enough
#define MAX_CXT_LINE_CHARS ( MAX_CXT_LINE_ITEMS * 10 + 2 )

//at most 10 characters per item and object in addition to "<" "," ">" and "\n"
#define MAX_BIN_CPT_LINE_CHARS ( (TRANS_MAX_COUNT + MAX_CXT_LINE_ITEMS) * 10 + 5 )

//at most 10 characters per item or object "\n"
#define MAX_LCM_CPT_LINE_CHARS ( max(TRANS_MAX_COUNT, MAX_CXT_LINE_ITEMS) * 10 + 2 )

#define POOL_SIZE (TRANS_MAX_COUNT)

#define NODE_COUNT_THRESHOLD 120000000
//#define NODE_COUNT_THRESHOLD 2000000

//------------------------------------------------------------------------------
//Types definitions
//------------------------------------------------------------------------------
typedef struct timespec TIMESPEC;

typedef unsigned int T_INT;

//------------------------------------------------------------------------------
//Functions declarations
//------------------------------------------------------------------------------
int max(int a, int b);

int min(int a, int b);

TIMESPEC diffTime(TIMESPEC start, TIMESPEC end);

TIMESPEC sumTime(TIMESPEC start, TIMESPEC end);

struct transaction {
	T_INT buffer[LIMBS_MAX_COUNT];
	T_INT bufferSize;
	T_INT firstSignificantLimb;
	T_INT limbCount;
	//keep track of the bits set to 1 which represent the total count of
	//items in this itemset
	T_INT itemCount;
};
typedef struct transaction Transaction;

struct transactions {
	T_INT itemCount;
	T_INT transactionsCount;
	Transaction *encodedTransactions;
	T_INT itemsPerLimb;
};
typedef struct transactions Transactions;

struct concept {
	T_INT *items;
	T_INT *transactions;
	T_INT itemsCount;
	T_INT transactionsCount;
	//T_INT nonGeneratorsCount;
	//T_INT generatorsCount;
	//double stabilityNonGen;
	//double stabilityGen;
	//mpf_t stabilityNonGenGMP;
	T_INT processed;
};
typedef struct concept Concept;

struct concepts {
	Concept * concepts;
	T_INT count;
};
typedef struct concepts Concepts;

struct transactionset;
typedef struct transactionset Transactionset;

struct alloctranset {
	struct transactionset * transet;

	struct alloctranset * next;
};
typedef struct alloctranset AllocTranset;

struct transactionset {

	// TODO to be removed
	//T_INT isForbidden;
	T_INT transactions;

	//sorting and moving plain memory blocks is expensive.
	//struct transactionset * transPtrs[TRANS_MAX_COUNT];

	T_INT childrenCount;
	//T_INT forbiddenChildrenCount;

	Transaction intersect;

	struct transactionset *children;

	AllocTranset * alloc;
};

void loadContextFile(char * file, Transactions *context);

void loadDATContextFile(char * file, Transactions *context);

void loadConceptsFile(char *file, Concepts *concepts, T_INT transactionsCount, T_INT itemsCount);

void loadLCMConceptsFile(char *file, Concepts *concepts, T_INT transactionsCount, T_INT itemsCount);

void unloadConcepts(Concepts * concepts);

void printfConcept(Concept * concept);

void initTransetPool();

void pushTranset(AllocTranset * toPush);

AllocTranset * popTranset();

void freeTransetRepo();

int compareCptByTransetSize(const void * a, const void * b);

#endif

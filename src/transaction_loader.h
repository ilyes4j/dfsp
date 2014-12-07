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

#include <limits.h>
#include <time.h>
#include <gmp.h>

//initialize the unsigned int buffer bit count
#define UINT_BIT_COUNT (sizeof(T_INT) * CHAR_BIT)

//the last bit in an integer buffer
#define LAST_BIT_INDEX (UINT_BIT_COUNT - 1)

#define NODE_COUNT_THRESHOLD 120000000

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
	T_INT * buffer;
	T_INT bufferSize;
	T_INT firstSignificantLimb;
	T_INT limbCount;

	//keep track of the bits set to 1 which represent the total count of items in
	//this itemset
	T_INT itemCount;
};
typedef struct transaction Transaction;

struct transactions {
	T_INT limbCount;
	T_INT itemCount;
	T_INT transactionsCount;
	Transaction *encodedTransactions;
	T_INT itemsPerLimb;
	T_INT * transBuffArea;
};
typedef struct transactions Transactions;

struct concept {
	T_INT *items;
	T_INT *transactions;
	T_INT itemsCount;
	T_INT transactionsCount;
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

	T_INT transactions;

	T_INT childrenCount;

	Transaction intersect;

	struct transactionset *children;

	AllocTranset * alloc;
};

void loadDATContextFile(char * file, Transactions *context);

void loadLCMConceptsFile(char *file, Concepts *concepts,
		T_INT transactionsCount, T_INT itemsCount);

void unloadConcepts(Concepts * concepts);

void printfConcept(Concept * concept);

void initTransetPool(T_INT transCount, T_INT limbCount);

void pushTranset(AllocTranset * toPush);

AllocTranset * popTranset();

void freeTransetRepo(T_INT transCount);

int compareCptByTransetSize(const void * a, const void * b);

#endif

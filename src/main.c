/*
 Copyright (c) 2013, 2014, 2015, Mohamed Ilyes Dimassi.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 3. All advertising materials mentioning features or use of this software
 must display the following acknowledgment:
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

#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include "stability_processor.h"
#include "transaction_loader.h"
#include "cli.h"

int main(int argc, char *argv[]) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//Performance profiling
	TIMESPEC start, end, diff;

	Transactionset * root;

	Transactions trans;
	Transactions * transPtr;

	Concepts concepts;
	Concepts * conceptsPtr;

	Concept * conceptList;
	Concept * currentConcept;

	uint currCptItemsCnt;
	uint currCptTransCnt;
	uint conceptCounter;
	uint conceptListCount;

	char * selectedContextFile;
	char * selectedConceptsFile;

	uint exploredNodesCount;
	uint approxExploredNodesCount;

	mpz_t currCptNonGenCountGMP;
	mpz_t currCptNonGenLocalCountGMP;
	mpz_t currCptGenCountGMP;
	mpz_t currCptGenLocalCountGMP;
	mpz_t currCptTotalCountGMP;

	mpf_t fltCurrCptGenCountGMP;
	mpf_t fltCurrCptTotalCountGMP;
	mpf_t fltCurrCptStab;

	mpz_t mpzTolRange;
	mpz_t mpzUpperThreshold;
	mpz_t mpzReverseUpperThreshold;

	ssize_t toConcept;
	ssize_t fromConcept;

	uint upperThreshold;
	uint stabPrecision;

	uint err;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------
	transPtr = &trans;
	conceptsPtr = &concepts;

	if (argc < 2) {
		invalidArguments("");
		exit(EXIT_FAILURE);
	}

	selectedContextFile = argv[1];
	selectedConceptsFile = argv[2];
	fromConcept = strtol(argv[3], NULL, 10);
	toConcept = strtol(argv[4], NULL, 10);

	upperThreshold = parseDoubleFraction(argv[5], &err);

	if (err != 0) {
		invalidArguments("Invalid upper stability threshold");
		exit(EXIT_FAILURE);
	}

	stabPrecision = digitsCount(upperThreshold);

	printf("\nDEPTH-FIRST-STABILITY-PROCESSOR V1.0 Beta\n\n");

	printf("Using database %s\n", selectedContextFile);
	printf("Using concepts %s\n", selectedConceptsFile);

	printf("\nLoading transactions...\n");

	loadDATContextFile(selectedContextFile, transPtr);

	printf("Transactions loaded\n");

	printf("\nLoading Formal Concepts...\n");

	loadLCMConceptsFile(selectedConceptsFile, conceptsPtr,
			transPtr->transactionsCount, transPtr->itemCount);

	printf("Formal Concepts Loaded\n\n");

	initTransetPool(transPtr->transactionsCount, transPtr->limbCount);

	printf("Output format :\n");
	printf("[Index][Extent][visited][approx][gain][Time][Stability][Done]\n\n");

	conceptList = concepts.concepts;
	conceptListCount = concepts.count;

	//if negative value it is considered an offset to the concept count
	toConcept = toConcept > 0 ? toConcept : conceptListCount + toConcept;
	//if negative value is way too big set it to end of the list
	toConcept = toConcept > 0 ? toConcept : conceptListCount;
	//if positive value out of bounds set to end of the list
	toConcept = toConcept > conceptListCount ? conceptListCount : toConcept;

	//if negative value it is considered an offset to the concept count
	fromConcept = fromConcept > 0 ? fromConcept : conceptListCount + fromConcept;
	//if negative value is way too big set to start of the list
	fromConcept = fromConcept > 0 ? fromConcept : 0;
	//if positive value out of bounds set to start of the list
	fromConcept = fromConcept < toConcept ? fromConcept : 0;

	printf("Found %u concepts available...\n", conceptListCount);
	printf("Processing concepts [%zd..%zd]...\n\n", fromConcept, toConcept - 1);
	printf("Ignore concepts under 0.%u threshold\n\n", upperThreshold);

	for (conceptCounter = fromConcept; conceptCounter < toConcept;
			conceptCounter++) {

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

		currentConcept = conceptList + conceptCounter;
		currCptItemsCnt = currentConcept->itemsCount;
		currCptTransCnt = currentConcept->transactionsCount;

		//allocate enough memory to hold the non generator counter
		//The non generator counter will hold at most all combinations possible
		//By allocating the maximum amount from the beginning we will prevent
		//on the fly memory reallocation which may cause performance overhead
		mpz_init2(currCptNonGenCountGMP, currCptTransCnt);
		mpz_init2(currCptNonGenLocalCountGMP, currCptTransCnt);
		mpz_init2(currCptGenCountGMP, currCptTransCnt);
		mpz_init2(currCptGenLocalCountGMP, currCptTransCnt);

		//same for the total count itself
		mpz_init2(currCptTotalCountGMP, currCptTransCnt);
		//then set the value of the total count 2^currCptTransCnt which can be
		//obtained efficiently by setting the currCptTransCnt bit
		mpz_setbit(currCptTotalCountGMP, currCptTransCnt);

		findThreshold(&mpzUpperThreshold, &mpzReverseUpperThreshold,
				currCptTransCnt, upperThreshold, stabPrecision);

		//build the tolerance range
		findToleranceRange(&mpzTolRange, stabPrecision, currCptTransCnt);

		//Total - range
		mpz_sub(mpzTolRange, currCptTotalCountGMP, mpzTolRange);

		//empty set counted as a non generator
		mpz_set_ui(currCptNonGenCountGMP, 1);
		mpz_sub_ui(mpzTolRange, mpzTolRange, 1);

		root = (Transactionset *) malloc(sizeof(Transactionset));

		initialize(currentConcept->transactions, currCptTransCnt,
				&currCptGenCountGMP, &currCptGenLocalCountGMP, &mpzTolRange, transPtr,
				currCptItemsCnt, root);

		if (root->childrenCount > 1) {
			processRecursive(root, &currCptGenCountGMP, &currCptGenLocalCountGMP,
					&currCptNonGenCountGMP, &currCptNonGenLocalCountGMP, &mpzTolRange,
					&mpzUpperThreshold, transPtr, currCptItemsCnt, 1);
		}

		exploredNodesCount = getExploredNodesCount();
		approxExploredNodesCount = getApproxExploredNodesCount();

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		diff = diffTime(start, end);

		//Whether using positive or negative counting
		//Stability is obtained using the same method
		mpf_init2(fltCurrCptGenCountGMP, 1000);
		mpf_init2(fltCurrCptTotalCountGMP, 1000);
		mpf_init2(fltCurrCptStab, 1000);

		if (exploredNodesCount < NODE_COUNT_THRESHOLD) {

			mpf_set_z(fltCurrCptGenCountGMP, currCptGenCountGMP);
			mpf_set_z(fltCurrCptTotalCountGMP, currCptTotalCountGMP);
			mpf_div(fltCurrCptStab, fltCurrCptGenCountGMP, fltCurrCptTotalCountGMP);

			mpz_add(currCptTotalCountGMP, currCptGenCountGMP, currCptNonGenCountGMP);

			gmp_printf("%4u; %6u; %10u; %10u; %.2f; %3ld,%-5ld; %.5Ff; %3s\n",
					conceptCounter, currCptTransCnt, exploredNodesCount,
					approxExploredNodesCount,
					(double) approxExploredNodesCount / exploredNodesCount, diff.tv_sec,
					diff.tv_nsec / 10000, fltCurrCptStab,
					hasCrossedThreshold() == 0 ? "YES" : "NO");
			//gmp_printf("%Zd\n", currCptGenCountGMP);
			//gmp_printf("%Zd\n", currCptNonGenCountGMP);
			//gmp_printf("%Zd\n\n", currCptTotalCountGMP);
		}

		pushTranset(root->alloc);
		free(root);

		mpz_clear(currCptNonGenCountGMP);
		mpz_clear(currCptNonGenLocalCountGMP);
		mpz_clear(currCptTotalCountGMP);
		mpz_clear(currCptGenCountGMP);
		mpz_clear(currCptGenLocalCountGMP);

		mpf_clear(fltCurrCptGenCountGMP);
		mpf_clear(fltCurrCptTotalCountGMP);
		mpf_clear(fltCurrCptStab);

		mpz_clear(mpzTolRange);
		mpz_clear(mpzUpperThreshold);
		mpz_clear(mpzReverseUpperThreshold);
	}

	freeTransetRepo(transPtr->transactionsCount);

	unloadConcepts(conceptsPtr);

	free(transPtr->transBuffArea);
	free(transPtr->encodedTransactions);

	printf("\nAll Concepts Processed [OK]\n");

	return EXIT_SUCCESS;
}

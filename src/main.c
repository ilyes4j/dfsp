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

int main(int argc, char *argv[]) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//Performance profiling
	TIMESPEC start, end, diff;

	Transactionset * root;

	Transactions trans;
	Transactions * transPtr;

	Concepts * concepts;
	Concept * conceptList;
	Concept * currentConcept;

	T_INT currCptItemsCnt;
	T_INT currCptTransCnt;
	T_INT conceptCounter;
	T_INT conceptListCount;

	char * selectedContextFile;
	char * selectedConceptsFile;

	T_INT exploredNodesCount;

	T_INT approxExploredNodesCount;

	mpz_t currCptNonGenCountGMP;
	mpz_t currCptNonGenLocalCountGMP;
	mpz_t currCptGenCountGMP;
	mpz_t currCptGenLocalCountGMP;
	mpz_t currCptTotalCountGMP;

	mpf_t fltCurrCptGenCountGMP;
	mpf_t fltCurrCptTotalCountGMP;
	mpf_t fltCurrCptStab;

	//declaration
	mpz_t mpzTolStep;
	mpz_t mpzTolRange;

	size_t precision;
	size_t toConcept;
	size_t fromConcept;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------
	precision = 2;
	fromConcept = 0;
	toConcept = 10;

	transPtr = &trans;

	selectedContextFile = argv[1];
	selectedConceptsFile = argv[2];

	printf("\nDEPTH-FIRST-STABILITY-PROCESSOR V0.1 Beta\n\n");

	printf("Using Database %s\n", selectedContextFile);
	printf("Using Database %s\n", selectedConceptsFile);

	printf("\nLoading transactions...\n");

	loadDATContextFile(selectedContextFile, transPtr);

	printf("Transactions loaded\n");

	printf("\nLoading Formal Concepts...\n");

	concepts = (Concepts *) malloc(sizeof(Concepts));

	loadLCMConceptsFile(selectedConceptsFile, concepts,
			transPtr->transactionsCount, transPtr->itemCount);

	printf("Formal Concepts Loaded\n\n");

	initTransetPool(transPtr->transactionsCount, transPtr->limbCount);

	printf("Output format :\n");
	printf("[Index] [Extent] [visited] [approx] [Time] [status] [Stability]\n");

	printf("\nProcessing concepts...\n\n");

	conceptList = concepts->concepts;
	conceptListCount = concepts->count;

	toConcept = min_szt(toConcept, conceptListCount);

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

		//build the tolerance range
		findRange(&mpzTolRange, &mpzTolStep, precision, currCptTransCnt);

		//Total - range
		mpz_sub(mpzTolRange, currCptTotalCountGMP, mpzTolRange);

		//empty set counted as a non generator
		mpz_set_ui(currCptNonGenCountGMP, 1);
		mpz_sub_ui(mpzTolRange, mpzTolRange, 1);

		root = initialize(currentConcept->transactions, currCptTransCnt,
				&currCptGenCountGMP, &currCptGenLocalCountGMP, &mpzTolRange, transPtr,
				currCptItemsCnt);

		if (root->childrenCount > 1) {
			processRecursive(root, &currCptGenCountGMP, &currCptGenLocalCountGMP,
					&currCptNonGenCountGMP, &currCptNonGenLocalCountGMP, &mpzTolRange,
					transPtr, currCptItemsCnt);
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

			gmp_printf("%4u; %6u; %10u; %10u; %.2f; %3ld,%-5ld; %.5Ff\n",
					conceptCounter, currCptTransCnt, exploredNodesCount,
					approxExploredNodesCount,
					(double) approxExploredNodesCount / exploredNodesCount, diff.tv_sec,
					diff.tv_nsec / 10000, fltCurrCptStab);
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

		mpz_clear(mpzTolStep);
		mpz_clear(mpzTolRange);
	}

	freeTransetRepo(transPtr->transactionsCount);

	unloadConcepts(concepts);

	free(transPtr->transBuffArea);
	free(transPtr->encodedTransactions);

	printf("\nAll Concepts Processed [OK]\n");

	return EXIT_SUCCESS;
}

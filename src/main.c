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
#include "gmp.h"
#include "stability_processor.h"
#include "transaction_loader.h"

extern T_INT nodeCount;

int main(void) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//Performance profiling
	TIMESPEC thediff;

	char contextFiles[][50] = { "db/poc_sample_bi.data", "db/lenses_bi.data",
			"db/post-operative_bi.data", "db/poc_sample_2_bi.data",
			"db/adult-stretch_bi.data", "db/hayes-roth_bi.data", "db/servo_bi.data",
			"db/SPECT_test_bi.data", "db/zoo_bi.data", "db/mushroom_bi.data",
			"db.dat/mushroom.dat", "accidents.dat", "data/chess/chess.dat",
			"connect.dat", "pumsb.dat", "retail.dat", "T10I4D100K.dat",
			"T40I10D100K.dat", "mushroom.dat" };

	char conceptFiles[][50] = { "db/poc_concepts_sample_bi.data",
			"db/lenses_concepts.data", "db/post-operative-concepts.data",
			"db/poc_sample_2_concepts.data", "db/adult-stretch_concepts.data",
			"db/hayes-roth_concepts.data", "db/servo_concepts.data",
			"db/SPECT_test_concepts.data", "db/zoo_concepts.data",
			"db/mushroom_concepts.data", "db.dat/mushroom_lcm_concepts_1000.dat",
			"accidents_concepts.dat", "data/chess/chess_concepts.dat",
			"connect_concepts.dat", "pumsb_concepts.dat", "retail_concepts.dat",
			"T10I4D100K_concepts.dat", "T40I10D100K_concepts.dat",
			"mushroom_concepts.dat" };

	TIMESPEC start, end;

	Transactionset * root;

	Transactions * transactions;

	Concepts * concepts;
	Concept * conceptList;
	Concept * currentConcept;

	T_INT currCptItemsCnt;
	T_INT currCptTransCnt;
	T_INT conceptCounter;
	T_INT conceptListCount;

	T_INT selectedDatabase;

	mpz_t currCptNonGenCountGMP;
	mpz_t currCptGenCountGMP;
	mpz_t currCptGenLocalCountGMP;
	mpz_t currCptTotalCountGMP;

	mpf_t fltCurrCptGenCountGMP;
	mpf_t fltCurrCptTotalCountGMP;
	mpf_t fltCurrCptStab;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------

	selectedDatabase = 12;

	printf("\nDEPTH-FIRST-STABILITY-PROCESSOR V0.1 Beta...\n\n");

	printf("Using Database %s\n", contextFiles[selectedDatabase]);
	printf("Using Database %s\n", conceptFiles[selectedDatabase]);

	printf("\nLoading transactions...\n");

	transactions = (Transactions *) malloc(sizeof(Transactions));
	loadDATContextFile2(contextFiles[selectedDatabase], transactions);

	printf("Transactions loaded [OK]\n");

	printf("\nLoading Formal Concepts...\n");

	concepts = (Concepts *) malloc(sizeof(Concepts));
	loadLCMConceptsFile(conceptFiles[selectedDatabase], concepts,
			transactions->transactionsCount, transactions->itemCount);

	printf("Formal Concepts Loaded [OK]\n\n");

	initTransetPool(transactions->transactionsCount, transactions->limbCount);

	printf("Output format :\n");
	//printf("<Extent,Intent>\n");
	printf("[Index] [Extent size] [visited] [Time] [Stability]\n");

	printf("\nProcessing concepts...\n");

	conceptList = concepts->concepts;
	conceptListCount = concepts->count;

	//	for (conceptCounter = 0; conceptCounter < conceptListCount;
	//			conceptCounter++) {

	for (conceptCounter = 0;
			conceptCounter < 2 && conceptCounter < conceptListCount;
			conceptCounter++) {

		currentConcept = conceptList + conceptCounter;
		currCptItemsCnt = currentConcept->itemsCount;
		currCptTransCnt = currentConcept->transactionsCount;

		//allocate enough memory to hold the non generator counter
		//The non generator counter will hold at most all combinations possible
		//By allocating the maximum amount from the beginning we will prevent
		//on the fly memory reallocation which may cause performance overhead
		mpz_init2(currCptNonGenCountGMP, currCptTransCnt);
		mpz_init2(currCptGenCountGMP, currCptTransCnt);
		mpz_init2(currCptGenLocalCountGMP, currCptTransCnt);

		//same for the total count itself
		mpz_init2(currCptTotalCountGMP, currCptTransCnt);
		//then set the value of the total count 2^currCptTransCnt which can be
		//obtained efficiently by setting the currCptTransCnt bit
		mpz_setbit(currCptTotalCountGMP, currCptTransCnt);

		clock_gettime(CLOCK_REALTIME, &start);

		root = initialize(currentConcept->transactions, currCptTransCnt,
				&currCptGenCountGMP, &currCptGenLocalCountGMP, transactions,
				currCptItemsCnt);

		nodeCount = root->childrenCount;

		if (root->childrenCount > 1 && nodeCount < NODE_COUNT_THRESHOLD) {
			processRecursive(root, &currCptGenCountGMP, &currCptGenLocalCountGMP,
					transactions, currCptItemsCnt);
		}

		clock_gettime(CLOCK_REALTIME, &end);
		thediff = diffTime(start, end);

		//Whether using positive or negative counting
		//Stability is obtained using the same method
		mpf_init2(fltCurrCptGenCountGMP, 1000);
		mpf_init2(fltCurrCptTotalCountGMP, 1000);
		mpf_init2(fltCurrCptStab, 1000);

		if (nodeCount < NODE_COUNT_THRESHOLD) {

			mpf_set_z(fltCurrCptGenCountGMP, currCptGenCountGMP);
			mpf_set_z(fltCurrCptTotalCountGMP, currCptTotalCountGMP);
			mpf_div(fltCurrCptStab, fltCurrCptGenCountGMP, fltCurrCptTotalCountGMP);

			gmp_printf("%4u; %6u; %10u; %3ld,%-5ld; %.5Ff\n", conceptCounter,
					currCptTransCnt, nodeCount, thediff.tv_sec, thediff.tv_nsec / 10000,
					fltCurrCptStab);
		}

		pushTranset(root->alloc);
		free(root);

		mpz_clear(currCptNonGenCountGMP);
		mpz_clear(currCptTotalCountGMP);
		mpz_clear(currCptGenCountGMP);
		mpz_clear(currCptGenLocalCountGMP);

		mpf_clear(fltCurrCptGenCountGMP);
		mpf_clear(fltCurrCptTotalCountGMP);
		mpf_clear(fltCurrCptStab);
	}

	freeTransetRepo(transactions->transactionsCount);

	unloadConcepts(concepts);

	free(transactions->transBuffArea);
	free(transactions->encodedTransactions);
	free(transactions);

	printf("Concepts Processed [OK]\n");

	return EXIT_SUCCESS;
}

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

#include "transaction_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

const T_INT UINT_BYTE_COUNT = sizeof(T_INT);

//initialize the unsigned int buffer bit count
const T_INT UINT_BIT_COUNT = sizeof(T_INT) * CHAR_BIT;

//the last bit in an integer buffer
const T_INT LAST_BIT_INDEX = sizeof(T_INT) * CHAR_BIT - 1;

Transactionset ** srcPtrs;
Transactionset ** transPtrs;

Transaction backupInter;
Transaction leadInter;

AllocTranset * head;

AllocTranset * transetRepo;

T_INT remainingCount;

TIMESPEC diffTime(TIMESPEC start, TIMESPEC end) {
	TIMESPEC temp;
	if ((end.tv_nsec - start.tv_nsec) < 0) {
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}

TIMESPEC sumTime(TIMESPEC start, TIMESPEC end) {
	TIMESPEC temp;
	temp.tv_nsec = end.tv_nsec + start.tv_nsec;
	temp.tv_sec = end.tv_sec - start.tv_sec + (temp.tv_nsec / 1000000000);
	temp.tv_nsec = (temp.tv_nsec % 1000000000);
	return temp;
}

int max(int a, int b) {
	return a > b ? a : b;
}

int min(int a, int b) {
	return a > b ? b : a;
}

void loadConceptsFile(char *file, Concepts *concepts, T_INT transactionsCount,
		T_INT itemsCount) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//a pointer to the file holding the transactions' list
	FILE *filePointer;

	T_INT conceptsCount;

	T_INT conceptsCounter;

	Concept * conceptList;

	Concept * currentConcept;

	//holds one line of the concepts file
	char line[MAX_BIN_CPT_LINE_CHARS];

	char *strItem;

	char *extent, *intent;

	T_INT * transactionsBuffer;

	T_INT * itemsBuffer;

	T_INT transactionsCounter;

	T_INT itemsCounter;

	T_INT element;

	T_INT * conceptTransactions;

	T_INT * conceptItems;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------

	//read the transactions file
	filePointer = fopen(file, "r");

	//exit if there is any problem reading that file
	if (filePointer == NULL) {
		printf("Error while reading input file !");
		exit(EXIT_FAILURE);
	}

	conceptsCount = 0;

	if (fgets(line, MAX_BIN_CPT_LINE_CHARS, filePointer) == NULL) {
		printf("Error while parsing the transactions count line !\n");
		exit(EXIT_FAILURE);
	}
	strItem = strtok(line, " \n");
	if (strItem == NULL) {
		printf("Error while reading the concepts count!\n");
		exit(EXIT_FAILURE);
	}
	conceptsCount = strtoul(strItem, NULL, 10);
	if (conceptsCount == 0) {
		printf("Error while parsing the concepts count !\n");
		exit(EXIT_FAILURE);
	}

	transactionsBuffer = (T_INT *) malloc(
			sizeof(UINT_BYTE_COUNT) * transactionsCount);

	itemsBuffer = (T_INT *) malloc(sizeof(UINT_BYTE_COUNT) * itemsCount);

	conceptList = (Concept *) malloc(sizeof(Concept) * conceptsCount);

	conceptsCounter = 0;

	//reading the file line by line
	while (fgets(line, MAX_BIN_CPT_LINE_CHARS, filePointer) != NULL) {

		extent = strtok(line, ",\n");
		if (extent == NULL) {
			printf("Error emply extent !\n");
			exit(EXIT_FAILURE);
		}

		intent = strtok(NULL, "\n");
		if (intent == NULL) {
			printf("Error emply intent !\n");
			exit(EXIT_FAILURE);
		}

		transactionsCounter = 0;
		itemsCounter = 0;

		strItem = strtok(extent, " ");
		while (strItem != NULL) {
			element = strtoul(strItem, NULL, 10);
			transactionsBuffer[transactionsCounter] = element;
			transactionsCounter++;
			strItem = strtok(NULL, " \n");
		}

		strItem = strtok(intent, " ");
		while (strItem != NULL) {
			element = strtoul(strItem, NULL, 10);
			itemsBuffer[itemsCounter] = element;
			itemsCounter++;
			strItem = strtok(NULL, " \n");
		}

		conceptTransactions = (T_INT *) malloc(
				sizeof(UINT_BYTE_COUNT) * transactionsCounter);
		memcpy(conceptTransactions, transactionsBuffer,
				sizeof(UINT_BYTE_COUNT) * transactionsCounter);

		conceptItems = (T_INT *) malloc(sizeof(UINT_BYTE_COUNT) * itemsCounter);
		memcpy(conceptItems, itemsBuffer,
				sizeof(UINT_BYTE_COUNT) * itemsCounter);

		currentConcept = conceptList + conceptsCounter;
		currentConcept->transactions = conceptTransactions;
		currentConcept->transactionsCount = transactionsCounter;
		currentConcept->items = conceptItems;
		currentConcept->itemsCount = itemsCounter;

		conceptsCounter++;
	}

	//sort concepts ascending as larger concepts takes more time to process
	qsort(conceptList, conceptsCount, sizeof(Concept), compareCptByTransetSize);

	concepts->concepts = conceptList;
	concepts->count = conceptsCount;

	free(transactionsBuffer);
	free(itemsBuffer);
	fclose(filePointer);
}

void loadLCMConceptsFile(char *file, Concepts *concepts,
		T_INT transactionsCount, T_INT itemsCount) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//a pointer to the file holding the transactions' list
	FILE *filePointer;

	T_INT conceptsCount;

	T_INT conceptsCounter;

	Concept * conceptList;

	Concept * currentConcept;

	//holds one line of the concepts file
	char line[MAX_LCM_CPT_LINE_CHARS];

	char *strItem;

	T_INT * transactionsBuffer;

	T_INT * itemsBuffer;

	T_INT transactionsCounter;

	T_INT itemsCounter;

	T_INT element;

	T_INT * conceptTransactions;

	T_INT * conceptItems;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------

	//read the transactions file
	filePointer = fopen(file, "r");

	//exit if there is any problem reading that file
	if (filePointer == NULL) {
		printf("Error while reading input file !");
		exit(EXIT_FAILURE);
	}

	conceptsCount = CONCEPTS_COUNT;

	transactionsBuffer = (T_INT *) malloc(
			sizeof(UINT_BYTE_COUNT) * transactionsCount);

	itemsBuffer = (T_INT *) malloc(sizeof(UINT_BYTE_COUNT) * itemsCount);

	conceptList = (Concept *) malloc(sizeof(Concept) * conceptsCount);

	conceptsCounter = 0;

	//reading the file line by line
	while (fgets(line, MAX_LCM_CPT_LINE_CHARS, filePointer) != NULL) {

		transactionsCounter = 0;
		itemsCounter = 0;

		strItem = strtok(line, " ");
		while (strItem != NULL) {
			element = strtoul(strItem, NULL, 10);
			itemsBuffer[itemsCounter] = element;
			itemsCounter++;
			strItem = strtok(NULL, " \n");
		}

		if (fgets(line, MAX_LCM_CPT_LINE_CHARS, filePointer) == NULL) {
			printf("Missing extent line !");
			exit(EXIT_FAILURE);
		}

		strItem = strtok(line, " ");
		while (strItem != NULL) {
			element = strtoul(strItem, NULL, 10);
			transactionsBuffer[transactionsCounter] = element;
			transactionsCounter++;
			strItem = strtok(NULL, " \n");
		}

		conceptItems = (T_INT *) malloc(sizeof(UINT_BYTE_COUNT) * itemsCounter);
		memcpy(conceptItems, itemsBuffer,
				sizeof(UINT_BYTE_COUNT) * itemsCounter);

		conceptTransactions = (T_INT *) malloc(
				sizeof(UINT_BYTE_COUNT) * transactionsCounter);
		memcpy(conceptTransactions, transactionsBuffer,
				sizeof(UINT_BYTE_COUNT) * transactionsCounter);

		currentConcept = conceptList + conceptsCounter;
		currentConcept->transactions = conceptTransactions;
		currentConcept->transactionsCount = transactionsCounter;
		currentConcept->items = conceptItems;
		currentConcept->itemsCount = itemsCounter;

		conceptsCounter++;
	}

	//sort concepts ascending as larger concepts takes more time to process
	qsort(conceptList, conceptsCount, sizeof(Concept), compareCptByTransetSize);

	concepts->concepts = conceptList;
	concepts->count = conceptsCount;

	free(transactionsBuffer);
	free(itemsBuffer);
	fclose(filePointer);
}

void unloadConcepts(Concepts * concepts) {

	T_INT counter;
	T_INT count;
	Concept * conceptPtr;
	Concept * currConcept;

	conceptPtr = concepts->concepts;
	count = concepts->count;

	for (counter = 0; counter < count; counter++) {
		currConcept = conceptPtr + counter;
		free(currConcept->transactions);
		free(currConcept->items);
	}

	free(concepts->concepts);
	free(concepts);
}

void loadDATContextFile(char * file, Transactions *context) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//a pointer to the file holding the transactions' list
	FILE *filePointer;

	//holds one line of the transactions' file
	char line[MAX_CXT_LINE_CHARS];

	//holder for one item as a string from the transaction line
	char *strItem;

	//Transactions count
	T_INT transactionsCount;

	//Transactions count
	T_INT itemsCount;

	//Transactions count
	T_INT limbCount;

	//a reference to an array containing all transactions
	Transaction * transactions;

	//the currently processed transaction
	Transaction * currentTransaction;

	T_INT currentTransactionIndex;

	T_INT *lineBuffer;

	T_INT currentLineItem = 0;

	T_INT currentLineItemsIndex;

	T_INT *transactionBuffer;

	T_INT minLimbIndex;

	T_INT maxLimbIndex;

	T_INT currentItemLimbIndex;

	T_INT currentItemPosition;

	T_INT currentLimbValue;

	T_INT * currentTransactionBuffer;

	//count the number of items found in each transaction
	T_INT currTransItemsCnt;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------

	//read the transactions file
	filePointer = fopen(file, "r");

	//exit if there is any problem reading that file
	if (filePointer == NULL) {
		printf("Error while reading input file !");
		exit(EXIT_FAILURE);
	}

	//setting up the transactions count
	transactionsCount = TRANS_MAX_COUNT;

	//reading the items count
	itemsCount = MAX_CXT_LINE_ITEMS;

	transactions = (Transaction *) malloc(
			sizeof(Transaction) * transactionsCount);

	//initialize the items buffer
	lineBuffer = (T_INT *) malloc(UINT_BYTE_COUNT * MAX_CXT_LINE_ITEMS);

	limbCount = ((itemsCount - 1) / UINT_BIT_COUNT) + 1;

	transactionBuffer = (T_INT *) malloc(UINT_BYTE_COUNT * limbCount);

	//initialize the processed transactions count
	currentTransactionIndex = 0;

	//read the file line by line
	while (fgets(line, MAX_CXT_LINE_CHARS, filePointer) != NULL) {

		currentTransaction = transactions + currentTransactionIndex;

		currentLineItemsIndex = 0;

		//Building the transaction from the current line of the file
		strItem = strtok(line, " \n");
		while (strItem != NULL) {
			currentLineItem = strtoul(strItem, NULL, 10);

			strItem = strtok(NULL, " \n");
			lineBuffer[currentLineItemsIndex] = currentLineItem;
			currentLineItemsIndex++;
		}

		//ensure the items count on this line corresponds to the count indicated
		//at the begining of the file
		if (currentLineItem > itemsCount) {
			printf("Error at line %d, items count overflow %d > %d !\n",
					currentTransactionIndex, currentLineItem, itemsCount);
			exit(EXIT_FAILURE);
		}

		maxLimbIndex = 0;
		minLimbIndex = limbCount;
		//memset operates at byte level, we need to set a transaction buffer
		//made of limbCount UNIT to 0. There is UINT_BYTE_COUNT bytes in each
		//UINT => The trasaction buffer has UINT_BYTE_COUNT * limbCount bytes
		memset(transactionBuffer, 0, UINT_BYTE_COUNT * limbCount);

		//initialize the items count for the current transaction
		currTransItemsCnt = currentLineItemsIndex;

		for (currentLineItemsIndex = 0;
				currentLineItemsIndex < currTransItemsCnt;
				currentLineItemsIndex++) {

			currentLineItem = lineBuffer[currentLineItemsIndex];

			currentItemLimbIndex = currentLineItem / UINT_BIT_COUNT;
			currentItemPosition = currentLineItem % UINT_BIT_COUNT;
			currentLimbValue = transactionBuffer[currentItemLimbIndex];
			maxLimbIndex = max(maxLimbIndex, currentItemLimbIndex);
			minLimbIndex = min(minLimbIndex, currentItemLimbIndex);

			currentLimbValue |= 1 << currentItemPosition;
			transactionBuffer[currentItemLimbIndex] = currentLimbValue;
		}

		//the limb count is the last limb index + 1
		maxLimbIndex++;

		currentTransactionBuffer = currentTransaction->buffer;

		memcpy(currentTransactionBuffer, transactionBuffer,
				UINT_BYTE_COUNT * maxLimbIndex);

		//assembling
		currentTransaction->bufferSize = maxLimbIndex;
		currentTransaction->firstSignificantLimb = minLimbIndex;
		currentTransaction->limbCount = maxLimbIndex;
		currentTransaction->itemCount = currTransItemsCnt;

		currentTransactionIndex++;
	}

	//assembling
	context->itemsPerLimb = UINT_BIT_COUNT;
	context->itemCount = itemsCount;
	context->transactionsCount = transactionsCount;
	context->encodedTransactions = transactions;

	free(transactionBuffer);
	free(lineBuffer);

	fclose(filePointer);
}

void loadContextFile(char * file, Transactions *context) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//a pointer to the file holding the transactions' list
	FILE *filePointer;

	//holds one line of the transactions' file
	char line[MAX_CXT_LINE_CHARS];

	//holder for one item as a string from the transaction line
	char *strItem;

	//Transactions count
	T_INT transactionsCount;

	//Transactions count
	T_INT itemsCount;

	//Transactions count
	T_INT limbCount;

	//a reference to an array containing all transactions
	Transaction * transactions;

	//the currently processed transaction
	Transaction * currentTransaction;

	T_INT currentTransactionIndex;

	T_INT *lineBuffer;

	T_INT currentLineItem;

	T_INT currentLineItemsIndex;

	T_INT *transactionBuffer;

	T_INT minLimbIndex;

	T_INT maxLimbIndex;

	T_INT currentItemLimbIndex;

	T_INT currentItemPosition;

	T_INT currentLimbValue;

	T_INT * currentTransactionBuffer;

	//count the number of items found in each transaction
	T_INT currTransItemsCnt;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------

	//read the transactions file
	filePointer = fopen(file, "r");

	//exit if there is any problem reading that file
	if (filePointer == NULL) {
		printf("Error while reading input file !");
		exit(EXIT_FAILURE);
	}

	//reading the transactions count
	transactionsCount = 0;

	if (fgets(line, MAX_CXT_LINE_CHARS, filePointer) == NULL) {
		printf("Error while parsing the transactions count line !\n");
		exit(EXIT_FAILURE);
	}
	strItem = strtok(line, " \n");
	if (strItem == NULL) {
		printf("Error while reading the transactions count!\n");
		exit(EXIT_FAILURE);
	}
	transactionsCount = strtoul(strItem, NULL, 10);
	if (transactionsCount == 0) {
		printf("Error while parsing the transactions count !\n");
		exit(EXIT_FAILURE);
	}

	//reading the items count
	itemsCount = 0;

	if (fgets(line, MAX_CXT_LINE_CHARS, filePointer) == NULL) {
		printf("Error while parsing the items count line !\n");
		exit(EXIT_FAILURE);
	}
	strItem = strtok(line, " \n");
	if (strItem == NULL) {
		printf("Error while reading the items count!\n");
		exit(EXIT_FAILURE);
	}
	itemsCount = strtoul(strItem, NULL, 10);
	if (itemsCount == 0) {
		printf("Error while parsing the items count !\n");
		exit(EXIT_FAILURE);
	}

	transactions = (Transaction *) malloc(
			sizeof(Transaction) * transactionsCount);

	//initialize the items buffer
	lineBuffer = (T_INT *) malloc(UINT_BYTE_COUNT * MAX_CXT_LINE_ITEMS);

	limbCount = ((itemsCount - 1) / UINT_BIT_COUNT) + 1;

	transactionBuffer = (T_INT *) malloc(UINT_BYTE_COUNT * limbCount);

	//initialize the processed transactions count
	currentTransactionIndex = 0;

	//read the file line by line
	while (fgets(line, MAX_CXT_LINE_CHARS, filePointer) != NULL) {

		currentTransaction = transactions + currentTransactionIndex;

		currentLineItemsIndex = 0;

		//Building the transaction from the current line of the file
		strItem = strtok(line, " \n");
		while (strItem != NULL) {
			currentLineItem = strtoul(strItem, NULL, 10);

			//ensure the read value is binary
			if ((currentLineItem >> 1) != 0) {
				printf("Error expected values = {0,1}, found %d !\n",
						currentLineItem);
				exit(EXIT_FAILURE);
			}

			strItem = strtok(NULL, " \n");
			lineBuffer[currentLineItemsIndex] = currentLineItem;
			currentLineItemsIndex++;
		}

		//ensure the items count on this line corresponds to the count indicated
		//at the begining of the file
		if (currentLineItemsIndex != itemsCount) {
			printf(
					"Error at line %d, the line items count is incorrect %d != %d !\n",
					currentTransactionIndex, currentLineItemsIndex, itemsCount);
			exit(EXIT_FAILURE);
		}

		maxLimbIndex = 0;
		minLimbIndex = limbCount;
		//memset operates at byte level, we need to set a transaction buffer
		//made of limbCount UNIT to 0. There is UINT_BYTE_COUNT bytes in each
		//UINT => The trasaction buffer has UINT_BYTE_COUNT * limbCount bytes
		memset(transactionBuffer, 0, UINT_BYTE_COUNT * limbCount);

		//initialize the items count for the current transaction
		currTransItemsCnt = 0;

		for (currentLineItemsIndex = 0; currentLineItemsIndex < itemsCount;
				currentLineItemsIndex++) {
			if (lineBuffer[currentLineItemsIndex] == 0) {
				continue;
			}

			currentItemLimbIndex = currentLineItemsIndex / UINT_BIT_COUNT;
			currentItemPosition = currentLineItemsIndex % UINT_BIT_COUNT;
			currentLimbValue = transactionBuffer[currentItemLimbIndex];
			maxLimbIndex = max(maxLimbIndex, currentItemLimbIndex);
			minLimbIndex = min(minLimbIndex, currentItemLimbIndex);

			currentLimbValue |= 1 << currentItemPosition;
			transactionBuffer[currentItemLimbIndex] = currentLimbValue;
			currTransItemsCnt++;
		}

		//the limb count is the last limb index + 1
		maxLimbIndex++;

		currentTransactionBuffer = currentTransaction->buffer;

		memcpy(currentTransactionBuffer, transactionBuffer,
				UINT_BYTE_COUNT * maxLimbIndex);

		//assembling
		currentTransaction->bufferSize = maxLimbIndex;
		currentTransaction->firstSignificantLimb = minLimbIndex;
		currentTransaction->limbCount = maxLimbIndex;
		currentTransaction->itemCount = currTransItemsCnt;

		currentTransactionIndex++;
	}

	//assembling
	context->itemsPerLimb = UINT_BIT_COUNT;
	context->itemCount = itemsCount;
	context->transactionsCount = transactionsCount;
	context->encodedTransactions = transactions;

	free(transactionBuffer);
	free(lineBuffer);

	fclose(filePointer);
}

void printfConcept(Concept * concept) {

	T_INT i;
	T_INT count;
	T_INT * items;

	printf("<");
	count = concept->transactionsCount;
	items = concept->transactions;
	for (i = 0; i < count; i++) {
		printf("%d ", items[i]);
	}
	printf(",");
	count = concept->itemsCount;
	items = concept->items;
	for (i = 0; i < count; i++) {
		printf("%d ", items[i]);
	}
	printf(">");
}

void initTransetPool() {

	srcPtrs = (Transactionset **) malloc(
			sizeof(Transactionset*) * TRANS_MAX_COUNT);
	transPtrs = (Transactionset **) malloc(
			sizeof(Transactionset*) * TRANS_MAX_COUNT);

	transetRepo = (AllocTranset *) malloc(sizeof(AllocTranset) * POOL_SIZE);

	T_INT counter;

	AllocTranset * current;

	AllocTranset * next;

	current = transetRepo;
	next = current + 1;

	for (counter = 0; counter < POOL_SIZE; counter++) {

		current->transet = (Transactionset *) malloc(
				sizeof(Transactionset) * POOL_SIZE);
		current->next = next;

		current = next;
		next++;
	}

	head = transetRepo;

	remainingCount = POOL_SIZE;
}

AllocTranset * popTranset() {

#ifdef DEBUG
	if (remainingCount == 0) {
		printf("Warning : Forbidden operation !\n");
		printf("All blocks are taken, cannot retrieve another block\n");
		exit(EXIT_FAILURE);
	}
#endif

	remainingCount--;

	AllocTranset * current;

	current = head;

	head = head->next;

	return current;
}

void pushTranset(AllocTranset * toPush) {

#ifdef DEBUG
	if (remainingCount == POOL_SIZE) {
		printf("Warning : Forbidden operation !\n");
		printf("All blocks have been released.\n");
		printf("The same block can only be released once.\n");
		exit(EXIT_FAILURE);
	}
#endif

	remainingCount++;

	toPush->next = head;
	head = toPush;
}

void freeTransetRepo() {

	T_INT counter;

	AllocTranset * current;

#ifdef DEBUG
	if (remainingCount != POOL_SIZE) {
		printf("Warning : major memory leak !\n");
		printf("%u memory blocks are still in use.\n", POOL_SIZE);
	}
#endif

	current = transetRepo;

	for (counter = 0; counter < POOL_SIZE; counter++) {

		current = transetRepo + counter;

		free(current->transet);
	}

	remainingCount = 0;
	free(transetRepo);

	free(srcPtrs);
	free(transPtrs);
}

int compareCptByTransetSize(const void * a, const void * b) {
	return (((Concept*) a)->transactionsCount
			- ((Concept*) b)->transactionsCount);
}

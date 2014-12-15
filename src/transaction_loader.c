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

T_INT * limbCube;

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

size_t max_szt(size_t a, size_t b) {
	return a > b ? a : b;
}

size_t min_szt(size_t a, size_t b) {
	return a > b ? b : a;
}

T_INT max_ui(T_INT a, T_INT b) {
	return a > b ? a : b;
}

T_INT min_ui(T_INT a, T_INT b) {
	return a > b ? b : a;
}

void getConceptsFileParams(FILE *filePointer, size_t * linesCountPtr,
		size_t * lineSizePtr) {

	char *line;

	size_t len = 0;

	size_t lineSize = 0;

	size_t lineCount = 0;

	ssize_t read;

	//processing
	line = NULL;

	while ((read = getline(&line, &len, filePointer)) != -1) {

		lineCount++;
		lineSize = max_szt(read, lineSize);
	}

	//add extra room for the null termination char
	lineSize++;

	*lineSizePtr = lineSize;
	*linesCountPtr = lineCount;

	free(line);
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
	char *line;

	char *strItem;

	T_INT * transactionsBuffer;

	T_INT * itemsBuffer;

	T_INT transactionsCounter;

	T_INT itemsCounter;

	T_INT element;

	T_INT * conceptTransactions;

	T_INT * conceptItems;

	size_t lineSize;

	size_t linesCount;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------

	//read the transactions file
	filePointer = fopen(file, "r");

	//exit if there is any problem reading that file
	if (filePointer == NULL) {
		printf("Error while opening concepts file %s !", file);
		exit(EXIT_FAILURE);
	}

	getConceptsFileParams(filePointer, &linesCount, &lineSize);

	if (linesCount % 2 != 0) {
		printf("Error while parsing concept file !\n");
		exit(EXIT_FAILURE);
	}

	//reset the file pointer to the start of the file for a second full scan
	fseek(filePointer, 0, SEEK_SET);

	conceptsCount = linesCount / 2;

	transactionsBuffer = (T_INT *) malloc(sizeof(T_INT) * transactionsCount);

	itemsBuffer = (T_INT *) malloc(sizeof(T_INT) * itemsCount);

	conceptList = (Concept *) malloc(sizeof(Concept) * conceptsCount);

	line = (char *) malloc(sizeof(char) * lineSize);

	conceptsCounter = 0;

	//reading the file line by line
	while (fgets(line, lineSize, filePointer) != NULL) {

		transactionsCounter = 0;
		itemsCounter = 0;

		strItem = strtok(line, " ");
		while (strItem != NULL) {
			element = strtoul(strItem, NULL, 10);
			itemsBuffer[itemsCounter] = element;
			itemsCounter++;
			strItem = strtok(NULL, " \n");
		}

		if (fgets(line, lineSize, filePointer) == NULL) {
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

		conceptItems = (T_INT *) malloc(sizeof(T_INT) * itemsCounter);
		memcpy(conceptItems, itemsBuffer, sizeof(T_INT) * itemsCounter);

		conceptTransactions = (T_INT *) malloc(sizeof(T_INT) * transactionsCounter);
		memcpy(conceptTransactions, transactionsBuffer,
				sizeof(T_INT) * transactionsCounter);

		currentConcept = conceptList + conceptsCounter;
		currentConcept->transactions = conceptTransactions;
		currentConcept->transactionsCount = transactionsCounter;
		currentConcept->items = conceptItems;
		currentConcept->itemsCount = itemsCounter;

		conceptsCounter++;
	}

	//sort concepts ascending as larger concepts usually takes more time to process
	qsort(conceptList, conceptsCount, sizeof(Concept), compareCptByTransetSize);

	concepts->concepts = conceptList;
	concepts->count = conceptsCount;

	free(line);
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

void getContextFileParams(FILE *filePointer, size_t * transCount,
		size_t * itemsCount, size_t * lineSize) {

	char *line;

	size_t len = 0;

	size_t maxlineSize = 0;

	size_t maxItemsCount = 0;

	size_t maxTransCount = 0;

	size_t startIndex;

	ssize_t read;

	unsigned long currentLineItemsCount;

	//processing

	//initialize the line pointer to null, if not it's value is not garanteed to
	//be NULL which causes an address of randomly selected memory adresses address
	//to be passed to getline which will assume that the line buffer has already
	//been allocated and that there is no need to allocate any memory space
	line = NULL;

	while ((read = getline(&line, &len, filePointer)) != -1) {

		maxTransCount++;

		maxlineSize = max_szt(read, maxlineSize);
		read--;

		if (line[read] == '\n') {
			read--;
		}

		for (; read >= 0; read--) {
			if (line[read] != ' ') {
				line[read + 1] = '\0';
				break;
			}
		}

		startIndex = 0;

		for (; read >= 0; read--) {
			if (line[read] == ' ') {
				startIndex = read + 1;
				break;
			}
		}

		currentLineItemsCount = strtoul(line + startIndex, NULL, 10);

		maxItemsCount = max_ui(maxItemsCount, currentLineItemsCount);
	}

	//add extra room for the null termination char
	maxlineSize++;

	*itemsCount = maxItemsCount;
	*lineSize = maxlineSize;
	*transCount = maxTransCount;

	free(line);
}

void loadDATContextFile(char * file, Transactions *context) {

	//--------------------------------------------------------------------------
	// Variables declaration
	//--------------------------------------------------------------------------

	//a pointer to the file holding the transactions' list
	FILE *filePointer;

	//holds one line of the transactions' file
	char *line;

	//holder for one item as a string from the transaction line
	char *strItem;

	//Transactions count
	size_t transactionsCount;

	//Transactions count
	size_t itemsCount;

	size_t lineSize;

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

	T_INT transBuffSize;

	T_INT *transBuffArea;

	//--------------------------------------------------------------------------
	// Processing
	//--------------------------------------------------------------------------

	//read the transactions file
	filePointer = fopen(file, "r");

	//exit if there is any problem reading that file
	if (filePointer == NULL) {
		printf("Error while opening context file %s !", file);
		exit(EXIT_FAILURE);
	}

	getContextFileParams(filePointer, &transactionsCount, &itemsCount, &lineSize);

	//reset the file pointer to the start of the file for a second full scan
	fseek(filePointer, 0, SEEK_SET);

	transactions = (Transaction *) malloc(
			sizeof(Transaction) * transactionsCount);

	//allocate enough room to store the largest line in the file
	line = (char *) malloc(sizeof(char) * lineSize);

	//initialize the items buffer
	lineBuffer = (T_INT *) malloc(sizeof(T_INT) * itemsCount);

	limbCount = ((itemsCount - 1) / UINT_BIT_COUNT) + 1;

	transBuffSize = sizeof(T_INT) * limbCount;

	transactionBuffer = (T_INT *) malloc(transBuffSize);

	transBuffArea = (T_INT *) malloc(transBuffSize * transactionsCount);

	//initialize the processed transactions count
	currentTransactionIndex = 0;

	//read the file line by line
	for (currentTransactionIndex = 0; fgets(line, lineSize, filePointer) != NULL;
			currentTransactionIndex++) {

		currentTransaction = transactions + currentTransactionIndex;

		currentLineItemsIndex = 0;

		//Building the transaction from the current line of the file
		strItem = strtok(line, " \n");
		while (strItem != NULL) {
			currentLineItem = strtoul(strItem, NULL, 10);

			strItem = strtok(NULL, " \n");
			lineBuffer[currentLineItemsIndex] = currentLineItem;
			currentLineItemsIndex++;

			if (currentLineItemsIndex >= itemsCount) {
				printf("Error while parsing context file !");
				exit(EXIT_FAILURE);
			}
		}

		maxLimbIndex = 0;
		minLimbIndex = limbCount;
		//memset operates at byte level, we need to set a transaction buffer
		//made of limbCount UNIT to 0. There is UINT_BYTE_COUNT bytes in each
		//UINT => The transaction buffer has UINT_BYTE_COUNT * limbCount bytes
		memset(transactionBuffer, 0, transBuffSize);

		//initialize the items count for the current transaction
		currTransItemsCnt = currentLineItemsIndex;

		for (currentLineItemsIndex = 0; currentLineItemsIndex < currTransItemsCnt;
				currentLineItemsIndex++) {

			currentLineItem = lineBuffer[currentLineItemsIndex];

			currentItemLimbIndex = currentLineItem / UINT_BIT_COUNT;
			currentItemPosition = currentLineItem % UINT_BIT_COUNT;
			currentLimbValue = transactionBuffer[currentItemLimbIndex];
			maxLimbIndex = max_ui(maxLimbIndex, currentItemLimbIndex);
			minLimbIndex = min_ui(minLimbIndex, currentItemLimbIndex);

			currentLimbValue |= 1 << currentItemPosition;
			transactionBuffer[currentItemLimbIndex] = currentLimbValue;
		}

		//the limb count is the last limb index + 1
		maxLimbIndex++;

		currentTransactionBuffer = currentTransaction->buffer = transBuffArea
				+ (limbCount * currentTransactionIndex);

		memcpy(currentTransactionBuffer, transactionBuffer,
				sizeof(T_INT) * maxLimbIndex);

		//assembling
		currentTransaction->bufferSize = maxLimbIndex;
		currentTransaction->firstSignificantLimb = minLimbIndex;
		currentTransaction->limbCount = maxLimbIndex;
		currentTransaction->itemCount = currTransItemsCnt;
	}

	//assembling
	context->itemsPerLimb = UINT_BIT_COUNT;
	context->itemCount = itemsCount;
	context->transactionsCount = transactionsCount;
	context->encodedTransactions = transactions;
	context->transBuffArea = transBuffArea;
	context->limbCount = limbCount;

	free(transactionBuffer);
	free(lineBuffer);
	free(line);

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

void initTransetPool(T_INT transCount, T_INT limbCount) {

	T_INT levelCounter, rowCounter;

	AllocTranset * current;

	AllocTranset * next;

	Transactionset * transet;

	T_INT * limbCursor;

	srcPtrs = (Transactionset **) malloc(sizeof(Transactionset*) * transCount);
	transPtrs = (Transactionset **) malloc(sizeof(Transactionset*) * transCount);

	backupInter.buffer = (T_INT *) malloc(sizeof(T_INT) * limbCount);
	leadInter.buffer = (T_INT *) malloc(sizeof(T_INT) * limbCount);

	transetRepo = (AllocTranset *) malloc(sizeof(AllocTranset) * transCount);

	limbCube = (T_INT *) malloc(
			sizeof(T_INT) * limbCount * transCount * transCount);

	limbCursor = limbCube;

	current = transetRepo;
	next = current + 1;

	for (levelCounter = 0; levelCounter < transCount; levelCounter++) {

		transet = (Transactionset *) malloc(sizeof(Transactionset) * transCount);

		for (rowCounter = 0; rowCounter < transCount; rowCounter++) {
			(transet + rowCounter)->intersect.buffer = limbCursor;
			limbCursor += limbCount;
		}

		current->transet = transet;
		current->next = next;

		current = next;
		next++;
	}

	head = transetRepo;

	remainingCount = transCount;
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

//#ifdef DEBUG
//	if (remainingCount == POOL_SIZE) {
//		printf("Warning : Forbidden operation !\n");
//		printf("All blocks have been released.\n");
//		printf("The same block can only be released once.\n");
//		exit(EXIT_FAILURE);
//	}
//#endif

	remainingCount++;

	toPush->next = head;
	head = toPush;
}

void freeTransetRepo(T_INT transCount) {

	T_INT counter;

	AllocTranset * current;

//#ifdef DEBUG
//	if (remainingCount != POOL_SIZE) {
//		printf("Warning : major memory leak !\n");
//		printf("%u memory blocks are still in use.\n", POOL_SIZE);
//	}
//#endif

	current = transetRepo;

	for (counter = 0; counter < transCount; counter++) {

		current = transetRepo + counter;

		free(current->transet);
	}

	remainingCount = 0;

	free(transetRepo);
	free(srcPtrs);
	free(transPtrs);
	free(limbCube);

	free(backupInter.buffer);
	free(leadInter.buffer);
}

int compareCptByTransetSize(const void * a, const void * b) {
	return (((Concept*) a)->transactionsCount - ((Concept*) b)->transactionsCount);
}

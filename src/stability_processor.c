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

#include "stability_processor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

T_INT nodeCount;

T_INT approxCount;

T_INT doneApprox;

extern Transactionset ** srcPtrs;
extern Transactionset ** transPtrs;

extern Transaction backupInter;
extern Transaction leadInter;

void findRange(mpz_t * mpzTolRange, mpz_t * mpzTolStep, size_t tolerance,
		size_t extentSize) {

	//processing
	mpz_init2(*mpzTolStep, sizeof(T_INT));
	mpz_ui_pow_ui(*mpzTolStep, 10, tolerance);

	mpz_init2(*mpzTolRange, extentSize);
	mpz_setbit(*mpzTolRange, extentSize);

	mpz_fdiv_q(*mpzTolRange, *mpzTolRange, *mpzTolStep);
}

T_INT getExploredNodesCount() {
	return nodeCount;
}

T_INT getApproxExploredNodesCount() {
	return approxCount;
}

T_INT hasDoneApprox() {
	return doneApprox;
}

//Build the root Transactionset. The root Transactionset contains an array of
//children Transactionsets. Each child is a single-transactions Transactionset
//in the formal concept extent. For each single-transaction Transactionset is
//flagged as a generator or a non generator. A generator is flagged as forbidden
//and will later be ignored from the exploration process
Transactionset * initialize(T_INT *items, T_INT itemsCount,
		mpz_t * genTotalCountGMP, mpz_t * genLocalCountGMP, mpz_t * rangeCountGMP,
		Transactions * transactions, T_INT refCount) {

	AllocTranset * alloc;

	T_INT transactionIndex;

	Transaction * transactionsList;
	Transaction * currentTransaction;
	T_INT currentTransactionItemsCount;

	Transaction * currentIntersect;

	//counter for the available transactions
	T_INT i;

	//counter only for the transactions added
	T_INT j;

	Transactionset * elements;
	Transactionset * current;
	Transactionset * ret;
	T_INT totalChildrenForbiddenCount;

	//processing
	transactionsList = transactions->encodedTransactions;

	alloc = popTranset();

	elements = alloc->transet;

	//all potential children are forbidden until proven to be non generators which
	//will cause this counter to be decremented
	totalChildrenForbiddenCount = itemsCount;

	for (i = 0, j = 0; i < itemsCount; i++) {

		//retrieving the object index
		transactionIndex = items[i];

		//Retrieving the itemset for that object
		currentTransaction = transactionsList + transactionIndex;

		//is a generator
		if (currentTransaction->itemCount == refCount) {
			continue;
		}

		totalChildrenForbiddenCount--;

		//setting a pointer on the next free transet memory space
		current = elements + j;

		//setting the suffix of the transactionset to be used later when building
		//this node previous siblings children
		current->transactions = transactionIndex;

		//initializing the node children
		current->children = NULL;
		//initializing the node children
		current->childrenCount = 0;

		currentTransactionItemsCount = currentTransaction->bufferSize;
		currentIntersect = &(current->intersect);
		currentIntersect->bufferSize = currentTransactionItemsCount;
		currentIntersect->limbCount = currentTransactionItemsCount;
		currentIntersect->firstSignificantLimb =
				currentTransaction->firstSignificantLimb;
		currentIntersect->itemCount = currentTransaction->itemCount;
		memcpy(currentIntersect->buffer, currentTransaction->buffer,
				sizeof(T_INT) * currentTransactionItemsCount);

		j++;
	}

	ret = (Transactionset *) malloc(sizeof(Transactionset));
	ret->children = elements;
	ret->alloc = alloc;
	ret->childrenCount = j;

	//initialize the number of explored candidates
	nodeCount = j;

	//GMP positive counting
	//counting all generators and their supersets using GMP Bignum structures
	mpz_set_ui(*genLocalCountGMP, 0);
	for (i = itemsCount - 1, j = 0; j < totalChildrenForbiddenCount; i--, j++) {
		mpz_setbit(*genLocalCountGMP, i);
	}
	mpz_add(*genTotalCountGMP, *genTotalCountGMP, *genLocalCountGMP);

	//initialize approximation to not yet found
	doneApprox = 0;

	//update the window guard
	mpz_sub(*rangeCountGMP, *rangeCountGMP, *genLocalCountGMP);

	return ret;
}

//Build the resulting intersection between Transaction * result and
//Transaction * operand and store the result in Transaction * result
void buildIntersection(Transaction * result, Transaction * left,
		Transaction * right) {

	T_INT intersectCount;
	T_INT res1stSigLimb;
	T_INT resLimbCount;
	T_INT *leftBuffer, *rightBuffer, *resBuffer;

	res1stSigLimb = max_ui(left->firstSignificantLimb, right->firstSignificantLimb);
	resLimbCount = min_ui(left->limbCount, right->limbCount);

	leftBuffer = left->buffer;
	rightBuffer = right->buffer;
	resBuffer = result->buffer;

	result->firstSignificantLimb = res1stSigLimb;
	result->limbCount = resLimbCount;

	intersectCount = 0;

	for (; res1stSigLimb < resLimbCount; res1stSigLimb++) {
		resBuffer[res1stSigLimb] = leftBuffer[res1stSigLimb]
				& rightBuffer[res1stSigLimb];
		intersectCount += __builtin_popcount(resBuffer[res1stSigLimb]);
	}

	result->itemCount = intersectCount;
}

//This recursive function is the main character in the exploration process
//Each node is breed with its (non-generators) forward sibling to generate its
//immediate children. Each child is then checked to determine whether it is a
//generator. This function is then called recursively to process each child.
void processRecursive(Transactionset * current, mpz_t *genTotalCountGMP,
		mpz_t *genLocalCountGMP, mpz_t * nongenTotalCountGMP,
		mpz_t * nongenLocalCountGMP, mpz_t * rangeCountGMP,
		Transactions * transactions, T_INT refCount) {

	T_INT startIdx;

	//at the end this counter will equals the non generators count
	T_INT endIdx;

	Transaction * backupInterPtr;

	Transaction * leadInterPtr;

	Transaction * swapInterPrt;

	//declaration
	T_INT j, k, l, currentCount, leftEltPotChildrenCnt, variationValue;

	int i;

	Transactionset * currentChildren;
	Transactionset * leftElement;
	Transactionset * leftEltChildren;
	Transactionset * rightElement;
	Transactionset * leftEltCurrentChild;

	T_INT forbiddenLeftEltChildrenCount;
	T_INT nonGenCurrCnt;

	Transaction * leftEltIntersect;
	Transaction * leftEltCurChildIntersect;

	int counter;

	Transactionset * srcTranset;

	Transaction * srcIntersect;

	AllocTranset * alloc;

	//operations
	//get the current elment's children count
	currentCount = current->childrenCount;

	//current element children
	currentChildren = current->children;

	//initialize the reference array
	for (i = 0; i < currentCount; i++) {
		srcPtrs[i] = currentChildren + i;
	}

	qsort(srcPtrs, currentCount, sizeof(Transactionset*),
			compareTransetPtrByCardDesc);

	//loop through the first currentCount - 1 elements and cross them against
	//the following elements to form the elements children, forbidden elemnts
	//should be excluded
	//exclude forbidden elements
	k = currentCount - 1;

	//all elements from this level have the same items count which can be
	//Retrieved from the first element

	startIdx = 0;
	endIdx = currentCount - 1;
	backupInterPtr = &backupInter;
	leadInterPtr = &leadInter;

	//copy last non generator intersect buffer to the current node subintersect
	counter = k;

	//srcTranset = currentChildren + counter;
	srcTranset = srcPtrs[counter];

	srcIntersect = &(srcTranset->intersect);

	backupInter.limbCount = srcIntersect->limbCount;
	backupInter.bufferSize = srcIntersect->bufferSize;
	backupInter.firstSignificantLimb = srcIntersect->firstSignificantLimb;

	memcpy(backupInter.buffer, srcIntersect->buffer,
			srcIntersect->limbCount * sizeof(T_INT));

	//do not forget to update the counter which will be used in the next loop
	counter--;

	transPtrs[endIdx] = srcTranset;
	endIdx--;

	//flag forbidden non generators
	for (; counter >= 0; counter--) {

		srcTranset = srcPtrs[counter];
		//srcTranset = currentChildren + counter;

		buildIntersection(leadInterPtr, backupInterPtr,
				transactions->encodedTransactions + srcTranset->transactions);

		if (leadInterPtr->itemCount == refCount) {

			transPtrs[startIdx] = srcTranset;
			startIdx++;

		} else {

			swapInterPrt = leadInterPtr;
			leadInterPtr = backupInterPtr;
			backupInterPtr = swapInterPrt;

			transPtrs[endIdx] = srcTranset;
			endIdx--;
		}
	}

	//GMP negative counting
	//counting all non generators and their supersets
	//TODO use limb direct access to fill whole limbs with ones using a single
	//operation
	mpz_set_ui(*nongenLocalCountGMP, 0);
	mpz_setbit(*nongenLocalCountGMP, currentCount - startIdx);
	mpz_sub_ui(*nongenLocalCountGMP, *nongenLocalCountGMP, 1);
	mpz_add(*nongenTotalCountGMP, *nongenTotalCountGMP, *nongenLocalCountGMP);
	mpz_add_ui(*nongenTotalCountGMP, *nongenTotalCountGMP, startIdx);

	if (doneApprox == 0) {
		mpz_sub(*rangeCountGMP, *rangeCountGMP, *nongenLocalCountGMP);
		mpz_sub_ui(*rangeCountGMP, *rangeCountGMP, startIdx);
		if ( mpz_sgn(*rangeCountGMP) == -1) {
			approxCount = nodeCount;
			doneApprox = 1;
		}
	}

	//crossing session level
	for (i = 0; i < startIdx; i++) {

		//retrieve the current crossing element
		leftElement = transPtrs[i];

		//left side of the crossing elements count
		leftEltPotChildrenCnt = currentCount - i - 1;

		alloc = popTranset();
		leftEltChildren = alloc->transet;

		//Retrieve the prefix of the next generation from the elements in the
		//current
		leftEltIntersect = &(leftElement->intersect);

		forbiddenLeftEltChildrenCount = 0;

		for (j = i + 1, nonGenCurrCnt = 0; j < currentCount; j++) {

			//debug variable

			rightElement = transPtrs[j];

			leftEltCurrentChild = leftEltChildren + nonGenCurrCnt;
			leftEltCurChildIntersect = &(leftEltCurrentChild->intersect);

			variationValue = rightElement->transactions;

			buildIntersection(leftEltCurChildIntersect, leftEltIntersect,
					transactions->encodedTransactions + variationValue);

			if (leftEltCurChildIntersect->itemCount == refCount) {
				forbiddenLeftEltChildrenCount++;
				continue;
			}

			nodeCount++;

			leftEltCurrentChild->transactions = variationValue;
			leftEltCurrentChild->children = NULL;
			leftEltCurrentChild->childrenCount = 0;

			nonGenCurrCnt++;
		}

		leftElement->children = leftEltChildren;
		leftElement->alloc = alloc;
		leftElement->childrenCount = nonGenCurrCnt;

		//GMP positive counting
		//counting all generators and their supersets using GMP Bignum structures
		mpz_set_ui(*genLocalCountGMP, 0);
		for (j = leftEltPotChildrenCnt - 1, l = 0;
				l < forbiddenLeftEltChildrenCount; j--, l++) {
			mpz_setbit(*genLocalCountGMP, j);
		}
		mpz_add(*genTotalCountGMP, *genTotalCountGMP, *genLocalCountGMP);

		if (doneApprox == 0) {
			mpz_sub(*rangeCountGMP, *rangeCountGMP, *genLocalCountGMP);
			if ( mpz_sgn(*rangeCountGMP) == -1) {
				approxCount = nodeCount;
				doneApprox = 1;
			}
		}

		if (nodeCount < NODE_COUNT_THRESHOLD) {
			if (nonGenCurrCnt > 1) {
				processRecursive(leftElement, genTotalCountGMP, genLocalCountGMP,
						nongenTotalCountGMP, nongenLocalCountGMP, rangeCountGMP,
						transactions, refCount);
			} else if (nonGenCurrCnt == 1) {
				mpz_add_ui(*nongenLocalCountGMP, *nongenLocalCountGMP, 1);
				mpz_add_ui(*nongenTotalCountGMP, *nongenTotalCountGMP, 1);

				if (doneApprox == 0) {
					mpz_sub_ui(*rangeCountGMP, *rangeCountGMP, 1);
					if ( mpz_sgn(*rangeCountGMP) == -1) {
						approxCount = nodeCount;
						doneApprox = 1;
					}
				}
			}
		}

		pushTranset(leftElement->alloc);
	}
}

//Debug-only function
//Count all nodes in a Transactionset recursively
T_INT elementsCount(Transactionset * root) {

	Transactionset * children;
	Transactionset * currentChild;

	T_INT childrenCount;
	T_INT i;
	T_INT eltCount;

	children = root->children;
	childrenCount = root->childrenCount;

	eltCount = childrenCount;

	for (i = 0; i < childrenCount; i++) {
		currentChild = children + i;

		eltCount += elementsCount(currentChild);
	}

	return eltCount;
}

int compareTransetByCardAsc(const void * a, const void * b) {
	return (((Transactionset*) a)->intersect.itemCount
			- ((Transactionset*) b)->intersect.itemCount);
}

int compareTransetByCardDesc(const void * a, const void * b) {
	return (((Transactionset*) b)->intersect.itemCount
			- ((Transactionset*) a)->intersect.itemCount);
}

int compareTransetPtrByCardAsc(const void * a, const void * b) {
	return ((*(Transactionset**) a)->intersect.itemCount
			- (*(Transactionset**) b)->intersect.itemCount);
}

int compareTransetPtrByCardDesc(const void * a, const void * b) {
	return ((*(Transactionset**) b)->intersect.itemCount
			- (*(Transactionset**) a)->intersect.itemCount);
}

int compareConceptByProc(const void * a, const void * b) {
	return (((Concept*) b)->processed - ((Concept*) a)->processed);
}

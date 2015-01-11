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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "cli.h"

uint parseDoubleFraction(const char * const strDouble, uint * const err) {

	uint counter;
	char c;
	uint state;

	const char * cursor = strDouble;
	const char * fraction = strDouble;

	//NULL or empty string is not tolerated
	if (cursor == NULL || strlen(cursor) == 0) {
		*err = 1;
		return 0;
	}

	//looking for many zeros followed by a dot then a random number
	state = 0;
	counter = 0;
	while ((c = *cursor) != '\0') {
		if (state == 0) {
			if (c == '0') {

			} else if (c > '0' && c <= '9') {
				//we found other digits than a zero, any other character is forbidden at
				//this point and we will be looking for integers from now on
				counter++;
				state = 1;
			} else {
				state = 2;
				break;
			}

		}	//at this point we're only accepting non zeroes integers, anything else is
		//unacceptable
		else if (c < '0' && c > '9') {
			*err = 1;
			return 0;
		} else {
			counter++;
		}
		cursor++;
	}

	//only zeroes is not enough
	if (state == 0) {
		*err = 1;
		return 0;
	}

	//we found an integer containing at most 10 digits it's valid
	if (state == 1) {
		if (counter < 11) {
			*err = 0;
			return strtoul(strDouble, NULL, 10);
		} else {
			*err = 1;
			return 0;
		}
	}

	//character other that 0 to 9 or dot or comma
	if (c != '.' && c != ',') {
		*err = 1;
		return 0;
	}

	cursor++;
	fraction = cursor;
	state = 0;
	counter = 0;
	while ((c = *cursor) != '\0') {

		if (c < '0' || c > '9' || counter > 10) {
			state = 1;
		}

		counter++;
		cursor++;
	}

	//character other that 0 to 9 or more than 10 characters or none
	if (state != 0 || counter == 0) {
		*err = 1;
		return 0;
	}

	*err = 0;
	return strtoul(fraction, NULL, 10);
}

//inform the user that the provided arguments are invalid and display the manual
void invalidArguments(char * message) {

	char buf[CHUNK];
	FILE *file;
	size_t nread;

	printf("%s\n", message);

	file = fopen(manual, "r");
	if (file) {
		while ((nread = fread(buf, 1, sizeof buf, file)) > 0) {
			fwrite(buf, 1, nread, stdout);
		}
		fclose(file);
	}
}

unsigned int digitsCount(unsigned int value) {

	//less than 2 digits
	if (value < 10)
		return 1;

	//less than 3 digits
	if (value < 100)
		return 2;

	//less than 4 digits
	if (value < 1000)
		return 3;

	//less than 5 digits
	if (value < 10000)
		return 4;

	//less than 6 digits
	if (value < 100000)
		return 5;

	//less than 7 digits
	if (value < 1000000)
		return 6;

	//less than 8 digits
	if (value < 10000000)
		return 7;

	//less than 9 digits
	if (value < 100000000)
		return 8;

	if (value < 1000000000)
		return 9;

	return 10;
}

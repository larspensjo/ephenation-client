// Copyright 2012 The Ephenation Authors
//
// This file is part of Ephenation.
//
// Ephenation is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// Ephenation is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Ephenation.  If not, see <http://www.gnu.org/licenses/>.
//

//
// encrypt/decrypt based on RC4, as defined in http://en.wikipedia.org/wiki/RC4
//

#include "crypt.h"

static unsigned char S[256];
static unsigned int i, j;

static void swap(unsigned char *s, unsigned int i, unsigned int j) {
	unsigned char temp = s[i];
	s[i] = s[j];
	s[j] = temp;
}

/* Key Scheduling algorithm */
void rc4_init(const unsigned char *key, unsigned int key_length) {
	for (i = 0; i < 256; i++)
		S[i] = i;

	for (i = j = 0; i < 256; i++) {
		j = (j + key[i % key_length] + S[i]) & 255;
		swap(S, i, j);
	}

	i = j = 0;
}

/* pseudo-random generation algorithm */
static unsigned char rc4_output() {
	i = (i + 1) & 255;
	j = (j + S[i]) & 255;

	swap(S, i, j);

	return S[(S[i] + S[j]) & 255];
}

// Overwrite 'data' with the xor of the keystring
void rc4_xor(unsigned char *data, int len) {
	for (int i=0; i<len; i++) {
		data[i] ^= rc4_output();
	}
}

#include <stdio.h>
#include <string.h>

void rc4_test() {
	// Test of encryption, according to http://en.wikipedia.org/wiki/RC4#Test_vectors
	unsigned char buff[100];
	strcpy((char *)buff, "Plaintext");
	int l1 = strlen((char *)buff);
	rc4_init((const unsigned char*)"Key", 3);
	rc4_xor(buff, l1);
	printf("rc4_test: ");
	for (int i=0; i<l1; i++)
		printf("%d ", buff[i]);
	printf("\n");
}

/* Copyright (c) 2011 SEOmoz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bloom.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

int init_pyrebloom(pyrebloomctxt * ctxt, unsigned char * key, uint32_t capacity, double error, char* host, uint32_t port) {
	// Counter
	uint32_t i;
	
	ctxt->capacity = capacity;
	ctxt->bits     = - (uint32_t)((log(error) * capacity) / (log(2) * log(2)));
	ctxt->hashes   = (uint32_t)(ceil(log(2) * ctxt->bits / capacity));
	ctxt->error    = error;
	ctxt->key      = key;//(unsigned char *)(malloc(strlen(key)));
	//strcpy(ctxt->key, key);
	ctxt->seeds    = (uint32_t *)(malloc(ctxt->hashes * sizeof(uint32_t)));
	// Generate all the seeds
	srand(1);
	for (i = 0; i < ctxt->hashes; ++i) {
		ctxt->seeds[i] = (uint32_t)(rand());
	}
	
	// Now for the redis context
	struct timeval timeout = { 1, 500000 };
	ctxt->ctxt     = redisConnectWithTimeout(host, port, timeout);
	if (ctxt->ctxt->err) {
		// Some kind of error
		return 0;
	}
	return 1;
}

int free_pyrebloom(pyrebloomctxt * ctxt) {
	if (ctxt->seeds) {
		free(ctxt->seeds);
	}
	redisFree(ctxt->ctxt);
}

int add(pyrebloomctxt * ctxt, const char * data, uint32_t len) {
	uint32_t i;
	for (i = 0; i < ctxt->hashes; ++i) {
    	uint32_t d = hash(data, len, ctxt->seeds[i], ctxt->bits);
		redisAppendCommand(ctxt->ctxt, "SETBIT %s %d 1", ctxt->key, d);
	}
	return 1;
}

int add_complete(pyrebloomctxt * ctxt, uint32_t count) {
	uint32_t i;
	redisReply * reply;
	
	for (i = 0; i < ctxt->hashes * count; ++i) {
		redisGetReply(ctxt->ctxt, (void**)(&reply));
		freeReplyObject(reply);
	}
	return 1;
}

int check(pyrebloomctxt * ctxt, const char * data, uint32_t len) {
	uint32_t i;
	for (i = 0; i < ctxt->hashes; ++i) {
		redisAppendCommand(ctxt->ctxt, "GETBIT %s %d", ctxt->key, hash(data, len, ctxt->seeds[i], ctxt->bits));
	}
	return 1;
}

int check_next(pyrebloomctxt * ctxt) {
	uint32_t i;
	int result = 1;
	redisReply * reply;
	for (i = 0; i < ctxt->hashes; ++i) {
		redisGetReply(ctxt->ctxt, (void**)(&reply));
		result = result && reply->integer;
		freeReplyObject(reply);
	}
	return result;
}

int delete(pyrebloomctxt * ctxt) {
	redisReply * reply;
	reply = redisCommand(ctxt->ctxt, "DEL %s", ctxt->key);
	freeReplyObject(reply);
	return 1;
}

// Taken from the discussion at http://www.azillionmonkeys.com/qed/hash.html
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t hash(const char * data, uint32_t len, uint32_t hash, uint32_t bits) {
	uint32_t tmp;
	int rem;

	if (len <= 0 || data == NULL) return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3: hash += get16bits (data);
			hash ^= hash << 16;
			hash ^= data[sizeof (uint16_t)] << 18;
			hash += hash >> 11;
			break;
		case 2: hash += get16bits (data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		case 1: hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash % bits;
}

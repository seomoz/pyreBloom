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

const uint32_t max_bits_per_key = 0xffffffff;

int init_pyrebloom(pyrebloomctxt * ctxt, char * key, uint32_t capacity, double error, char* host, uint32_t port, char* password) {
	// Counter
	uint32_t i;
	
	ctxt->capacity = capacity;
	ctxt->bits     = (uint64_t)(-(log(error) * capacity) / (log(2) * log(2)));
	ctxt->hashes   = (uint32_t)(ceil(log(2) * ctxt->bits / capacity));
	ctxt->error    = error;
	ctxt->key      = (char *)(malloc(strlen(key)));
	strncpy(ctxt->key, key, strlen(key));
	ctxt->seeds    = (uint32_t *)(malloc(ctxt->hashes * sizeof(uint32_t)));
	ctxt->password = (char *)(malloc(strlen(password)));
	strncpy(ctxt->password, password, strlen(password));

	/* We'll need a certain number of strings here */
	ctxt->num_keys = (uint32_t)(
		ceil((float)(ctxt->bits) / max_bits_per_key));
	ctxt->keys = (char**)(malloc(ctxt->num_keys * sizeof(char*)));
	for (i = 0; i < ctxt->num_keys; ++i) {
		ctxt->keys[i] = (char*)(malloc(strlen(key) + 10));
		sprintf(ctxt->keys[i], "%s.%i", key, i);
	}

	/* The implementation here used to rely on srand(1) and then repeated
	 * calls to rand(), but I no longer trust that to provide correct behavior
	 * when working between different platforms. As such, We'll be using a LCG
	 *
	 *     http://en.wikipedia.org/wiki/Linear_congruential_generator
	 *
	 * Hopefully this will be the last of interoperability issues. Note that
	 * updating to this version will unfortunately require rebuilding old
	 * bloom filters.
	 *
	 * Our m is implicitly going to be 2^32 by storing the result into a
	 * uint32_t */
	uint32_t a = 1664525;
	uint32_t c = 1013904223;
	uint32_t x = 314159265;
	for (i = 0; i < ctxt->hashes; ++i) {
		ctxt->seeds[i] = x;
		x = a * x + c;
	}
	
	// Now for the redis context
	struct timeval timeout = { 1, 500000 };
	ctxt->ctxt = redisConnectWithTimeout(host, port, timeout);
	if (ctxt->ctxt->err != 0) {
		return ctxt->ctxt->err;
	}

	/* And now if a password was provided, then */
	if (strlen(password) != 0) {
		redisReply * reply;
		reply = redisCommand(ctxt->ctxt, "AUTH %s", password);
		if (reply->type == REDIS_REPLY_ERROR) {
			/* We'll copy the error into the context's error string */
			strncpy(ctxt->ctxt->errstr, reply->str, 128);
			freeReplyObject(reply);
			return 1;
		} else {
			freeReplyObject(reply);
			return 0;
		}
	} else {
		/* Make sure that we can ping the host */
		redisReply * reply;
		reply = redisCommand(ctxt->ctxt, "PING");
		if (reply->type == REDIS_REPLY_ERROR) {
			strncpy(ctxt->ctxt->errstr, reply->str, 128);
			freeReplyObject(reply);
			return 1;
		}
		/* And now, finally free this reply object */
		freeReplyObject(reply);
	}
	return 0;
}

int free_pyrebloom(pyrebloomctxt * ctxt) {
	if (ctxt->seeds) {
		free(ctxt->seeds);
	}
	redisFree(ctxt->ctxt);
	return 0;
}

int add(pyrebloomctxt * ctxt, const char * data, uint32_t len) {
	uint32_t i;
	for (i = 0; i < ctxt->hashes; ++i) {
    	uint64_t d = hash(data, len, ctxt->seeds[i], ctxt->bits);
		redisAppendCommand(ctxt->ctxt, "SETBIT %s %lu 1",
			ctxt->keys[d / max_bits_per_key], d % max_bits_per_key);
	}
	return 1;
}

int add_complete(pyrebloomctxt * ctxt, uint32_t count) {
	uint32_t i, j, ct=0, total=0;
	redisReply * reply;

	ctxt->ctxt->err = 0;
	for (i = 0; i < count; ++i) {
		for (j = 0, ct = 0; j < ctxt->hashes; ++j) {
			redisGetReply(ctxt->ctxt, (void**)(&reply));
			if (reply->type == REDIS_REPLY_ERROR) {
				ctxt->ctxt->err = 1;
				strncpy(ctxt->ctxt->errstr, reply->str, 128);
			} else {
				ct += reply->integer;
			}
			freeReplyObject(reply);
		}
		if (ct == ctxt->hashes) {
			total += 1;
		}
	}

	if (ctxt->ctxt->err) {
		return -1;
	} else {
		return count - total;
	}
}

int check(pyrebloomctxt * ctxt, const char * data, uint32_t len) {
	uint32_t i;
	for (i = 0; i < ctxt->hashes; ++i) {
		uint64_t d = hash(data, len, ctxt->seeds[i], ctxt->bits);
		redisAppendCommand(ctxt->ctxt, "GETBIT %s %lu",
			ctxt->keys[d / max_bits_per_key], d % max_bits_per_key);
	}
	return 1;
}

int check_next(pyrebloomctxt * ctxt) {
	uint32_t i;
	int result = 1;
	redisReply * reply;
	ctxt->ctxt->err = 0;
	for (i = 0; i < ctxt->hashes; ++i) {
		redisGetReply(ctxt->ctxt, (void**)(&reply));
		if (reply->type == REDIS_REPLY_ERROR) {
			ctxt->ctxt->err = 1;
			strncpy(ctxt->ctxt->errstr, reply->str, 128);
		}
		result = result && reply->integer;
		freeReplyObject(reply);
	}
	if (ctxt->ctxt->err) {
		return -1;
	}
	return result;
}

int delete(pyrebloomctxt * ctxt) {
	uint32_t num_keys = (uint32_t)(
		ceil((float)(ctxt->bits) / max_bits_per_key));
	uint32_t i = 0;
	for (; i < num_keys; ++i) {
		redisReply * reply;
		reply = redisCommand(ctxt->ctxt, "DEL %s", ctxt->keys[i]);
		freeReplyObject(reply);
	}
	
	return 1;
}

/* MurmurHash64A implementation taken from:
 *
 *    https://sites.google.com/site/murmurhash/
 *
 * on November 20, 2012 under the MIT license */

#include <stdint.h>

uint64_t MurmurHash64A ( const void * key, uint32_t len, uint64_t seed )
{
    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len/8);

    while(data != end)
    {
        uint64_t k = *data++;

        k *= m; 
        k ^= k >> r; 
        k *= m; 
        
        h ^= k;
        h *= m; 
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch(len & 7)
    {
    case 7: h ^= (uint64_t)(data2[6]) << 48;
    case 6: h ^= (uint64_t)(data2[5]) << 40;
    case 5: h ^= (uint64_t)(data2[4]) << 32;
    case 4: h ^= (uint64_t)(data2[3]) << 24;
    case 3: h ^= (uint64_t)(data2[2]) << 16;
    case 2: h ^= (uint64_t)(data2[1]) << 8;
    case 1: h ^= (uint64_t)(data2[0]);
            h *= m;
    };
 
    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
} 

uint64_t hash(const char* data, uint32_t len, uint64_t seed, uint64_t bits) {
    return MurmurHash64A(data, len, seed) % bits;
}
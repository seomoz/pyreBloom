#ifndef PYRE_BLOOM_H
#define PYRE_BLOOM_H

#include <stdint.h>
#include <stdlib.h>
#include <hiredis/hiredis.h>

// And now for some redis stuff
typedef struct {
	uint32_t        capacity;
	uint32_t        bits;
	uint32_t        hashes;
	float           error;
	uint32_t      * seeds;
	unsigned char * key;
	redisContext  * ctxt;
} pyrebloomctxt;

int init_pyrebloom(pyrebloomctxt * ctxt, unsigned char * key, uint32_t capacity, double error);
int free_pyrebloom(pyrebloomctxt * ctxt);

int add(pyrebloomctxt * ctxt, const char * data, uint32_t len);
int add_complete(pyrebloomctxt * ctxt, uint32_t count);

int check(pyrebloomctxt * ctxt, const char * data, uint32_t len);
int check_next(pyrebloomctxt * ctxt);

int delete(pyrebloomctxt * ctxt);

uint32_t hash(const char * data, uint32_t len, uint32_t hash, uint32_t bits);

#endif

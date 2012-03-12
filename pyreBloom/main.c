#include "bloom.h"
#include <time.h>

/*
int init_pyrebloom(pyrebloomctxt * ctxt, const char * key, uint32_t capacity, double error);

int add(pyrebloomctxt * ctxt, const char * data, uint32_t len);
int add_complete(pyrebloomctxt * ctxt, uint32_t count);

int check(pyrebloomctxt * ctxt, const char * data, uint32_t len);
int check_next(pyrebloomctxt * ctxt);

uint32_t hash(const char * data, uint32_t len, uint32_t hash, uint32_t bits);
*/

int main(int argc, char ** argv) {
	pyrebloomctxt ctxt;
	uint32_t i;
	time_t start, end;
	uint32_t count = 100000;
	
	init_pyrebloom(&ctxt, "testing", count, 0.1);
	
	time(&start);
	for (i = 0; i < count; ++i) {
		add(&ctxt, "hello", 5);
	}
	add_complete(&ctxt, count);
	time(&end);
	printf("pyreBloom : %f\n", difftime(end, start));
	
	time(&start);
	for (i = 0; i < count; ++i) {
		redisAppendCommand(ctxt.ctxt, "SADD testing2 hello");
	}
	add_complete(&ctxt, count / 4);
	time(&end);
	printf("sadd      : %f\n", difftime(end, start));
	
	return 0;
}
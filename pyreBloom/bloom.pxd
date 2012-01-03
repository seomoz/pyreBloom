cdef extern from "bloom.h":
	ctypedef struct redisContext
	ctypedef unsigned int uint32_t
	
	ctypedef struct pyrebloomctxt:
		uint32_t capacity
		uint32_t bits
		uint32_t hashes
		float    error
		uint32_t * seeds
		unsigned char * key
		redisContext * ctxt

	bint init_pyrebloom(pyrebloomctxt * ctxt, char * key, uint32_t capacity, float error)
	bint add(pyrebloomctxt * ctxt, char * data, uint32_t len)
	bint add_complete(pyrebloomctxt * ctxt, uint32_t count)
	bint check(pyrebloomctxt * ctxt, char * data, uint32_t len)
	bint check_next(pyrebloomctxt * ctxt)
	uint32_t hash(unsigned char * data, uint32_t len, uint32_t hash, uint32_t bits)

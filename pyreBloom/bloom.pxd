cdef extern from "bloom.h":
	ctypedef unsigned int uint32_t
	uint32_t hash(unsigned char * data, uint32_t len, uint32_t hash, uint32_t bits)

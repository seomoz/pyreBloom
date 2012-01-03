import math
import redis
import random

cimport bloom

cdef class pyreBloom(object):
	cdef bloom.pyrebloomctxt context
	cdef object              r
	
	property bits:
		def __get__(self):
			return self.context.bits
	
	property hashes:
		def __get__(self):
			return self.context.hashes
	
	def __init__(self, key, capacity, error, *args, **kwargs):
		bloom.init_pyrebloom(&self.context, key, capacity, error)
		self.r = redis.Redis()
	
	def delete(self):
		self.r.delete(self.context.key)
	
	def put(self, value):
		if getattr(value, '__iter__', False):
			r = [bloom.add(&self.context, v, len(v)) for v in value]
			bloom.add_complete(&self.context, len(value))
		else:
			bloom.add(&self.context, value, len(value))
			bloom.add_complete(&self.context, 1)
	
	def add(self, value):
		self.put(value)
	
	def extend(self, values):
		self.put(values)
	
	def contains(self, value):
		# If the object is 'iterable'...
		if getattr(value, '__iter__', False):
			r = [bloom.check(&self.context, v, len(v)) for v in value]
			return [v for v in value if bloom.check_next(&self.context)]
		else:
			bloom.check(&self.context, value, len(value))
			return bool(bloom.check_next(&self.context))
	
	def __contains__(self, value):
		return self.contains(value)

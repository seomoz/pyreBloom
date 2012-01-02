import math
import redis
import random

cimport bloom

class pyreBloom(object):
	def __init__(self, key, capacity, error, *args, **kwargs):
		self.capacity = capacity
		self.error    = error
		self.bits     = - int(self.capacity * math.log(self.error) / (math.log(2) ** 2))
		self.hashes   = int(math.ceil(math.log(2) * self.bits / self.capacity))
		self.key      = key
		# This will need to be changed, obviously
		self.seeds    = [random.getrandbits(32) for i in range(self.hashes)]
		# Make a redis object
		self.redis    = redis.Redis(*args, **kwargs)
	
	def _hash(self, value):
		cdef bloom.uint32_t length
		if isinstance(value, str):
			length = len(value)
			return [bloom.hash(value, length, seed, self.bits) for seed in self.seeds]
		else:
			return [bloom.hash(<unsigned char*>(value), 4, seed, self.bits) for seed in self.seeds]
	
	def put(self, value):
		with self.redis.pipeline() as p:
			for hash in self._hash(value):
				o = p.setbit(self.key, hash, 1)
			o = p.execute()
		return True
	
	def add(self, value):
		self.put(value)
	
	def extend(self, values):
		with self.redis.pipeline() as p:
			for value in values:
				for hash in self._hash(value):
					o = p.setbit(self.key, hash, 1)
			o = p.execute()
			return True
	
	def contains(self, value):
		cdef bloom.uint32_t counter
		cdef bloom.uint32_t s
		
		# Set the sum and counters to 0
		s       = 0
		counter = 0
		
		# If the object is 'iterable'...
		if getattr(value, '__iter__', False):
			results = []
			with self.redis.pipeline() as p:
				values = [v for v in value]
				for value in values:
					for hash in self._hash(value):
						o = p.getbit(self.key, hash)
				results  = p.execute()
				return [values[i] for i in range(len(values)) if (sum(results[i * self.hashes:(i+1) * self.hashes]) == self.hashes)]
		else:
			hashes = self._hash(value)
			with self.redis.pipeline() as p:
				for hash in hashes:
					o = p.getbit(self.key, hash)
				return sum(p.execute()) == self.hashes
	
	def __contains__(self, value):
		return self.contains(value)

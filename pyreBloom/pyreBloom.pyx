# Copyright (c) 2011 SEOmoz
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import math
import random

cimport bloom


class pyreBloomException(Exception):
	'''Some sort of exception has happened internally'''
	pass


cdef class pyreBloom(object):
	cdef bloom.pyrebloomctxt context
	cdef bytes               key
	
	property bits:
		def __get__(self):
			return self.context.bits
	
	property hashes:
		def __get__(self):
			return self.context.hashes
	
	def __cinit__(self, key, capacity, error, host='127.0.0.1', port=6379):
		self.key = key
		if bloom.init_pyrebloom(&self.context, self.key, capacity,
			error, host, port):
			raise pyreBloomException(self.context.ctxt.errstr)
	
	def __dealloc__(self):
		bloom.free_pyrebloom(&self.context) 
	
	def delete(self):
		bloom.delete(&self.context)
	
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

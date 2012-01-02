#! /usr/bin/env python

import time
import random
import string
import unittest
import pyreBloom

count    = 5000
capacity = count * 2
error    = 0.1

print 'Generating %i random test words' % (count * 2)
start = -time.time()
included  = [''.join(random.sample(string.lowercase, 20)) for i in range(count)]
outcluded = [''.join(random.sample(string.lowercase, 20)) for i in range(count)]
start += time.time()
print 'Generated random test words in %fs' % start

p = pyreBloom.pyreBloom('pyreBloomTesting', capacity, error)
p.redis.delete('pyreBloomTesting')

print 'Filter using %i hash functions and %i bits' % (p.hashes, p.bits)

start = -time.time()
p.extend(included)
start += time.time()
print 'Batch insert : %fs (%f words / second)' % (start, (count / start))

p.redis.delete('pyreBloomTesting')
start = -time.time()
r = [p.add(word) for word in included]
start += time.time()
print 'Serial insert: %fs (%f words / second)' % (start, (count / start))

start = -time.time()
r = p.contains(included)
start += time.time()
print 'Batch test   : %fs (%f words / second)' % (start, count / start)

start = -time.time()
r = [word in p for word in included]
start += time.time()
print 'Serial test  : %fs (%f words / second)' % (start, count / start)

falsePositives = p.contains(outcluded)
falseRate      = float(len(falsePositives)) / len(outcluded)
print 'False positive rate: %f (%f expected)' % (falseRate, error)

p.redis.delete('pyreBloomTesting')
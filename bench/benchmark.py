#! /usr/bin/env python

import time
import redis
import random
import string
import unittest
import pyreBloom

count    = 10000
capacity = count * 2
error    = 0.1

print('Generating %i random test words' % (count * 2))
start = -time.time()
included  = [''.join(random.sample(string.lowercase, 20)) for i in range(count)]
outcluded = [''.join(random.sample(string.lowercase, 20)) for i in range(count)]
start += time.time()
print('Generated random test words in %fs' % start)

p = pyreBloom.pyreBloom('pyreBloomTesting', capacity, error)
p.delete()

print('Filter using %i hash functions and %i bits' % (p.hashes, p.bits))

start = -time.time()
p.extend(included)
start += time.time()
print('Batch insert : %fs (%f words / second)' % (start, (count / start)))

p.delete()
start = -time.time()
r = [p.add(word) for word in included]
start += time.time()
print('Serial insert: %fs (%f words / second)' % (start, (count / start)))

start = -time.time()
r = p.contains(included)
start += time.time()
print('Batch test   : %fs (%f words / second)' % (start, count / start))

start = -time.time()
r = [(word in p) for word in included]
start += time.time()
print('Serial test  : %fs (%f words / second)' % (start, count / start))

falsePositives = p.contains(outcluded)
falseRate      = float(len(falsePositives)) / len(outcluded)
print('False positive rate: %f (%f expected)' % (falseRate, error))

# Now, let's compare this to adding items to a set in redis
p.delete()
start = -time.time()
r = redis.Redis()
o = r.sadd('pyreBloomTesting', *included)
start += time.time()
print('Redis set add  : %fs (%f words / second)' % (start, count / start))

start = -time.time()
with r.pipeline() as pipe:
    o = [pipe.sismember('pyreBloomTesting', word) for word in included]
    results = pipe.execute()
start += time.time()
if sum(int(i) for i in results) != len(included):
    print('REDIS PIPE FAILED!')
print('Redis pipe chk : %fs (%f words / second)' % (start, count / start))

p.delete()
start = -time.time()
with r.pipeline() as pipe:
    o = [pipe.sadd('pyreBloomTesting', word) for word in included]
    results = pipe.execute()
start += time.time()
print('Redis pipe sadd: %fs (%f words / second)' % (start, count / start))

start = -time.time()
with r.pipeline() as pipe:
    o = [pipe.sismember('pyreBloomTesting', word) for word in included]
    results = pipe.execute()
start += time.time()
if sum(int(i) for i in results) != len(included):
    print('REDIS PIPE FAILED!')
print('Redis pipe chk : %fs (%f words / second)' % (start, count / start))

p.delete()
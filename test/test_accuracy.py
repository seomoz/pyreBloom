#! /usr/bin/env python

'''All the tests'''

import random
import string
import unittest
import pyreBloom
from pyreBloom import pyreBloomException


def sample_strings(length, count):
    '''Return a set of sample strings'''
    return [''.join(
        random.sample(string.lowercase, length)) for i in range(count)]


class FunctionalityTest(unittest.TestCase):
    '''Check some basic functionality requirements'''
    def setUp(self):
        self.bloom = pyreBloom.pyreBloom('pyreBloomTesting', 200000000, 0.00001)
        self.bloom.delete()

    def tearDown(self):
        self.setUp()

    def test_connection_refused(self):
        '''In this test I want to make sure that we can catch errors when
        connecting to a redis instance'''
        self.assertRaises(pyreBloom.pyreBloomException, pyreBloom.pyreBloom,
            'pyreBloomTesting', 100, 0.01, port=1234)

    def test_size_allocation(self):
        '''Make sure we can allocate a bloom filter that would take more than
        512MB (the string size limit in Redis)'''
        included = sample_strings(20, 5000)
        excluded = sample_strings(20, 5000)

        # Add only the included strings
        self.bloom.extend(included)
        self.assertEqual(len(included), len(self.bloom.contains(included)))

        false_positives = self.bloom.contains(excluded)
        false_rate = float(len(false_positives)) / len(excluded)
        self.assertLess(
            false_rate, 0.00001, 'False positive error rate exceeded!')

        # We also need to know that we can access all the keys we need
        self.assertEqual(self.bloom.keys(),
            ['pyreBloomTesting.0', 'pyreBloomTesting.1'])

    def test_delete(self):
        '''Make sure that when we delete the bloom filter, we really do'''
        samples = sample_strings(20, 5000)
        self.bloom.extend(samples)
        self.bloom.delete()
        self.assertEqual(len(self.bloom.contains(samples)), 0,
            'Failed to actually delete filter')

    def test_error(self):
        '''If we encounter a redis error, we should raise exceptions'''
        self.bloom.delete()
        from redis import Redis
        redis = Redis()
        redis.hmset('pyreBloomTesting.0', {'hello': 5})
        self.assertRaises(pyreBloomException, self.bloom.add, 'hello')
        self.assertRaises(pyreBloomException, self.bloom.extend, ['a', 'b'])
        self.assertRaises(pyreBloomException, self.bloom.contains, 'a')
        self.assertRaises(pyreBloomException, self.bloom.contains, ['a', 'b'])


class Accuracytest(unittest.TestCase):
    '''Make sure we meet our accuracy expectations for the bloom filter'''
    def setUp(self):
        self.bloom = pyreBloom.pyreBloom('pyreBloomTesting', 10000, 0.1)
        self.bloom.delete()

    def tearDown(self):
        self.setUp()

    def test_add(self):
        '''Make sure we can add, check existing in a basic way'''
        tests = ['hello', 'how', 'are', 'you', 'today']
        for test in tests:
            self.bloom.add(test)
        for test in tests:
            self.assertTrue(test in self.bloom)

    def test_extend(self):
        '''Make sure we can use the extend method to the same effect'''
        tests = ['hello', 'how', 'are', 'you', 'today']
        self.bloom.extend(tests)
        for test in tests:
            self.assertTrue(test in self.bloom)

    def test_contains(self):
        '''Make sure contains returns a list when given a list'''
        tests = ['hello', 'how', 'are', 'you', 'today']
        self.bloom.extend(tests)
        self.assertEqual(tests, self.bloom.contains(tests))

    def test_random(self):
        '''Insert some random strings, make sure we don't see another set of
        random strings as in the bloom filter'''
        included = sample_strings(20, 5000)
        excluded = sample_strings(20, 5000)

        # Add only the included strings
        self.bloom.extend(included)
        self.assertTrue(len(included) == len(self.bloom.contains(included)))

        false_positives = self.bloom.contains(excluded)
        false_rate = float(len(false_positives)) / len(excluded)
        self.assertLess(
            false_rate, 0.1, 'False positive error rate exceeded!')

    def test_two_instances(self):
        '''Make sure two bloom filters pointing to the same key work'''
        bloom2 = pyreBloom.pyreBloom('pyreBloomTesting', 10000, 0.1)
        tests = ['hello', 'how', 'are', 'you', 'today']

        # Add them through the first instance
        self.bloom.extend(tests)
        self.assertEqual(tests, self.bloom.contains(tests))

        # Make sure they're accessible through the second instance
        self.assertEqual(tests, bloom2.contains(tests))

if __name__ == '__main__':
    unittest.main()

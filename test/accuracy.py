#! /usr/bin/env python

'''All the tests'''

import random
import string
import unittest
import pyreBloom


class FunctionalityTest(unittest.TestCase):
    '''Check some basic functionality requirements'''
    def test_connection_refused(self):
        '''In this test I want to make sure that we can catch errors when
        connecting to a redis instance'''
        self.assertRaises(pyreBloom.pyreBloomException, pyreBloom.pyreBloom,
            'pyreBloomTesting', 100, 0.01, port=1234)


class Accuracytest(unittest.TestCase):
    '''Make sure we meet our accuracy expectations for the bloom filter'''
    def setUp(self):
        self.bloom = pyreBloom.pyreBloom('pyreBloomTesting', 10000, 0.1)
        self.bloom.delete()

    def tearDown(self):
        self.bloom = pyreBloom.pyreBloom('pyreBloomTesting', 10000, 0.1)
        self.bloom.delete()

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
        included = [''.join(
            random.sample(string.lowercase, 20)) for i in range(5000)]
        excluded = [''.join(
            random.sample(string.lowercase, 20)) for i in range(5000)]

        # Add only the included strings
        self.bloom.extend(included)
        self.assertTrue(len(included) == len(self.bloom.contains(included)))

        false_positives = self.bloom.contains(excluded)
        false_rate = float(len(false_positives)) / len(excluded)
        self.assertTrue(
            false_rate < 0.1, 'False positive error rate exceeded!')

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

#! /usr/bin/env python

import random
import string
import unittest
import pyreBloom

class Accuracytest(unittest.TestCase):
    def setUp(self):
        self.p = pyreBloom.pyreBloom('pyreBloomTesting', 10000, 0.1)
        self.p.redis.delete('pyreBloomTesting')
    
    def tearDown(self):
        self.p = pyreBloom.pyreBloom('pyreBloomTesting', 10000, 0.1)
        self.p.redis.delete('pyreBloomTesting')
    
    def test_add(self):
        tests = ['hello', 'how', 'are', 'you', 'today']
        
        for t in tests:
            self.p.add(t)
        
        for t in tests:
            self.assertTrue(t in self.p)

    def test_extend(self):
        tests = ['hello', 'how', 'are', 'you', 'today']
        self.p.extend(tests)
        for t in tests:
            self.assertTrue(t in self.p)
    
    def test_contains(self):
        tests = ['hello', 'how', 'are', 'you', 'today']
        self.p.extend(tests)
        self.assertEqual(tests, self.p.contains(tests))
    
    def test_random(self):
        included  = [''.join(random.sample(string.lowercase, 20)) for i in range(5000)]
        outcluded = [''.join(random.sample(string.lowercase, 20)) for i in range(5000)]
        
        # Add only the included strings
        self.p.extend(included)
        self.assertTrue(len(included) == len(self.p.contains(included)))
        
        falsePositives = self.p.contains(outcluded)
        falseRate      = float(len(falsePositives)) / len(outcluded)
        print 'False positive rate: %f' % falseRate
        self.assertTrue(falseRate < 0.1, 'False positive error rate exceeded!')

if __name__ == '__main__':
    unittest.main()

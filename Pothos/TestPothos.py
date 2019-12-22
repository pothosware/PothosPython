# Copyright (c) 2014-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

import Pothos
import unittest
import warnings
import numpy as np

# Pothos can't do this from its Proxy infrastructure because it can't
# get "UserWarning".
def CallWarning(msg):
    warnings.warn(msg, UserWarning, stacklevel=1)

class TestPothosModule(unittest.TestCase):

    def setUp(self):
        self.env = Pothos.ProxyEnvironment("managed")
        self.assertEqual(self.env.getName(), "managed")

    def test_basic(self):
        #create null/empty proxy
        p0 = Pothos.Proxy()
        self.assertFalse(p0)

        #the proxy for empty object (is non null)
        p1 = self.env.convertObjectToProxy(None)
        self.assertTrue(p1)

        #make a proxy that wraps a python obj
        p2 = Pothos.Proxy(True)
        self.assertTrue(p2)
        self.assertTrue(p2.convert())

        #check that the same env can be retrieved
        self.assertTrue(p1.getEnvironment() == self.env)

    def test_compare_to(self):
        #create integer proxies
        oneProxy = self.env.convertObjectToProxy(1)
        self.assertTrue(oneProxy)
        twoProxy = self.env.convertObjectToProxy(2)
        self.assertTrue(twoProxy)

        #exercise rich-comparisons
        self.assertLess(oneProxy, twoProxy)
        self.assertLessEqual(oneProxy, twoProxy)
        self.assertLessEqual(oneProxy, oneProxy)
        self.assertGreater(twoProxy, oneProxy)
        self.assertGreaterEqual(twoProxy, oneProxy)
        self.assertGreaterEqual(twoProxy, twoProxy)
        self.assertNotEqual(oneProxy, twoProxy)
        self.assertEqual(twoProxy, twoProxy)

        #mix python objects with non-proxies
        self.assertLess(oneProxy, 2)
        self.assertLess(1, twoProxy)

    def test_block(self):

        #testing it through the proxy
        reg = self.env.findProxy("Pothos/BlockRegistry")
        self.assertTrue(reg)
        print(reg.callProxy("/blocks/feeder_source", "int"))
        print(hash(reg))

        #using the wrapper
        print(Pothos.BlockRegistry("/blocks/feeder_source", "int"))

    def test_int_buffer(self):
        npArr0 = np.array([1, 2, 3], np.int32)
        localBuffer0 = self.env.convertObjectToProxy(npArr0)
        print('localBuffer0 = %s'%localBuffer0)

        self.assertEqual(localBuffer0.dtype.size(), 4)
        self.assertEqual(localBuffer0.address, npArr0.__array_interface__['data'][0])

        npArr1 = self.env.convertProxyToObject(localBuffer0)
        print('npArr1 = %s'%npArr1)
        self.assertEqual(npArr0.dtype, npArr1.dtype)
        np.testing.assert_array_equal(npArr0, npArr1)

    def test_complex_float_buffer(self):
        npArr0 = np.array([1, 2-1j, 3j], np.complex64)
        localBuffer0 = self.env.convertObjectToProxy(npArr0)
        print('localBuffer0 = %s'%localBuffer0)

        self.assertEqual(localBuffer0.dtype.size(), 8)
        self.assertEqual(localBuffer0.address, npArr0.__array_interface__['data'][0])

        npArr1 = self.env.convertProxyToObject(localBuffer0)
        print('npArr1 = %s'%npArr1)
        self.assertEqual(npArr0.dtype, npArr1.dtype)
        np.testing.assert_array_equal(npArr0, npArr1)

    def test_packet_type(self):
        pkt0 = Pothos.Packet()
        pkt0.payload = np.array([1, 2, 3], np.int32)
        #TODO test labels

try: from StringIO import StringIO
except ImportError: from io import StringIO

def main():
    suite = unittest.TestLoader().loadTestsFromTestCase(TestPothosModule)
    sio = StringIO()
    result = unittest.TextTestRunner(stream=sio, verbosity=2).run(suite)
    print(sio.getvalue())
    if not result.wasSuccessful(): raise Exception("unittest FAIL")

if __name__ == '__main__':
    main()

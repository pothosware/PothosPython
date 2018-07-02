# Copyright (c) 2016-2018 Josh Blum
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *

class Topology(object):
    def __init__(self, *args):
        env = ProxyEnvironment("managed")
        self._topology = env.findProxy('Pothos/Topology').make(*args)

    def connect(self, src, srcPort, dst, dstPort):
        return self._topology.connect(src, str(srcPort), dst, str(dstPort))

    def disconnect(self, src, srcPort, dst, dstPort):
        return self._topology.disconnect(src, str(srcPort), dst, str(dstPort))

    def __getattr__(self, name):
        return lambda *args: self._topology.call(name, *args)

    def getInternalBlock(self):
        """
        Get access to the underlying Pothos::Block handle.
        Topology uses this call to speak directly to the block.
        """
        return self.self._topology

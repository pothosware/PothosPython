# Copyright (c) 2016-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *
#from . Label import Label
#from . Buffer import numpy_to_chunk

class Packet(object):
    def __init__(self):
        self.payload = None
        self.metadata = {}
        self.labels = []

    def toProxy(self, env):
        """
        Convert to a Pothos::Packet Proxy object.
        """
        pkt = env.findProxy("Pothos/Packet")()
        if self.payload is not None:
            pkt.payload = numpy_to_chunk(env, self.payload)
        if self.metadata:
            pkt.metadata = self.metadata
        #if self.labels:
        #    pkt.labels = [label.toProxy(env) for label in self.labels]
        return pkt

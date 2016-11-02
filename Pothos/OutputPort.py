# Copyright (c) 2014-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *
from . Buffer import pointer_to_ndarray
from . Buffer import dtype_to_numpy
from . Buffer import numpy_to_chunk
from . Label import Label
from . Packet import Packet
import numpy

class OutputPort(object):
    def __init__(self, port):
        self._port = port
        self._env = self._port.getEnvironment()

    def __getattr__(self, name):
        return lambda *args: self._port.call(name, *args)

    def dtype(self):
        return dtype_to_numpy(self._port.dtype())

    def buffer(self):
        addr = self._port.buffer().address
        nitems = self._port.elements()
        dtype = self.dtype()
        return pointer_to_ndarray(addr, nitems, dtype, readonly=False)

    def postBuffer(self, buffer):
        self._port.postBuffer(numpy_to_chunk(self._env, buffer))

    def postLabel(self, label):
        if isinstance(label, Proxy):# and label.getClassName() == "Pothos::Label":
            self._port.postLabel(label)
        elif isinstance(label, Label):
            self._port.postLabel(label.toProxy(self._env))
        else:
            raise Exception('OutputPort.postLabel - unknown type %s'%type(label))

    def postMessage(self, message):
        #special handling for known wrapped types
        if isinstance(message, Packet):
            self._port.postMessage(message.toProxy(self._env))
        else:
            self._port.postMessage(message)

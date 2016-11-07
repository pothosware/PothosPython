# Copyright (c) 2014-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *
from . Buffer import pointer_to_ndarray
from . Buffer import dtype_to_numpy
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

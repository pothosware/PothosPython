# Copyright (c) 2014-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *
from . InputPort import InputPort
from . OutputPort import OutputPort
from . Label import Label, LabelIteratorRange
from . BlockRegistry import BlockRegistry
import weakref

class Block(object):
    def __init__(self):
        self._block = BlockRegistry("/blocks/python_block")
        self._block._setPyBlock(weakref.proxy(self))

    def __getattr__(self, name):
        return lambda *args: self._block.call(name, *args)

    def getInternalBlock(self):
        """
        Get access to the underlying Pothos::Block handle.
        Topology uses this call to speak directly to the block.
        """
        return self._block

    def inputs(self):
        ports = self._block.inputs()
        return [InputPort(ports.at(i)) for i in range(ports.size())]

    def allInputs(self):
        ports = self._block.allInputs()
        return dict([(key, InputPort(ports.at(key))) for key in ports.keys()])

    def input(self, name):
        return InputPort(self._block.input(name))

    def outputs(self):
        ports = self._block.outputs()
        return [OutputPort(ports.at(i)) for i in range(ports.size())]

    def allOutputs(self):
        ports = self._block.allOutputs()
        return dict([(key, OutputPort(ports.at(key))) for key in ports.keys()])

    def output(self, name):
        return OutputPort(self._block.output(name))

    def activate(self): pass

    def deactivate(self): pass

    def work(self): pass

    def propagateLabels(self, input): return NotImplemented

    def _propagateLabels(self, name):
        return self.propagateLabels(self.input(name)) is NotImplemented

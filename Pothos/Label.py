# Copyright (c) 2014-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *

Label = ProxyEnvironment("managed").findProxy('Pothos/Label')

class LabelIteratorRange(object):
    def __init__(self, labelIter):
        self._labelIter = labelIter

    def __getattr__(self, name):
        return lambda *args: self._labelIter.call(name, *args)

    def __iter__(self):
        index = 0
        while True:
            i = self._labelIter.at(index)
            if i == self._labelIter.end(): break
            yield i.deref()
            index += 1

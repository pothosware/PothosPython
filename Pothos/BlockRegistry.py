# Copyright (c) 2016-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *

def BlockRegistry(path, *args):
    env = ProxyEnvironment("managed")
    reg = env.findProxy("Pothos/BlockRegistry")
    return reg.callProxy(path, *args)

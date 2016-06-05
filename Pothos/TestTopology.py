# Copyright (c) 2016-2016 Josh Blum
# SPDX-License-Identifier: BSL-1.0

import Pothos

class MessageForwarder(Pothos.Block):
    def __init__(self):
        Pothos.Block.__init__(self)
        self.setupInput("0")
        self.setupOutput("0")

    def work(self):

        #forward message
        if self.input(0).hasMessage():
            m = self.input(0).popMessage()
            print('msg %s'%m)
            self.output(0).postMessage(m)

if __name__ == '__main__':

    #access to the C++ Pothos classes
    env = Pothos.ProxyEnvironment("managed")

    #make a topology to connect and process
    topology = Pothos.Topology()

    #create feeder block and give it a message
    feeder = Pothos.BlockRegistry("/blocks/feeder_source", "int")
    feeder.feedMessage("hello")
    feeder.feedMessage("world")
    feeder.feedMessage(123)

    #create a collector block to house the result
    collector = Pothos.BlockRegistry("/blocks/collector_sink", "int")

    #use an all in-python block thats not even in the block registry
    fwd = MessageForwarder()

    #connect and run it
    topology.connect(feeder, 0, fwd, 0)
    topology.connect(fwd, 0, collector, 0)
    topology.commit()
    topology.waitInactive()

    #print the collector result
    msgs = collector.getMessages()
    print(msgs)

    topology.disconnectAll()
    topology.commit()

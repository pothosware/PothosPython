// Copyright (c) 2014-2017 Josh Blum
//               2020-2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Managed.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Proxy.hpp>

#include <json.hpp>

#include <Poco/Path.h>
#include <Poco/TemporaryFile.h>
#include <Poco/Thread.h>

#include <complex>
#include <cmath>
#include <iostream>

using json = nlohmann::json;

POTHOS_TEST_BLOCK("/proxy/python/tests", python_module_import)
{
    auto env = Pothos::ProxyEnvironment::make("python");
    env->findProxy("Pothos");
}

POTHOS_TEST_BLOCK("/proxy/python/tests", test_python_module)
{
    auto env = Pothos::ProxyEnvironment::make("python");
    env->findProxy("Pothos.TestPothos").call("main");
}

POTHOS_TEST_BLOCK("/proxy/python/tests", test_python_block)
{
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", "int");
    auto collector = Pothos::BlockRegistry::make("/blocks/collector_sink", "int");
    auto forwarder = Pothos::BlockRegistry::make("/python/forwarder", Pothos::DType("int"));

    //create a test plan
    json testPlan;
    testPlan["enableBuffers"] = true;
    testPlan["enableLabels"] = true;
    testPlan["enableMessages"] = true;
    auto expected = feeder.call("feedTestPlan", testPlan.dump());

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, forwarder, 0);
        topology.connect(forwarder, 0, collector, 0);
        std::cout << "topology commit\n";
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive(0.5, 5.0));
    }

    collector.call("verifyTestPlan", expected);
    std::cout << "run done\n";
}

POTHOS_TEST_BLOCK("/proxy/python/tests", test_signals_and_slots)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto reg = env->findProxy("Pothos/BlockRegistry");
    auto emitter = Pothos::BlockRegistry::make("/python/simple_signal_emitter");
    auto acceptor = Pothos::BlockRegistry::make("/python/simple_slot_acceptor");

    //run the topology
    {
        Pothos::Topology topology;
        topology.connect(emitter, "activateCalled", acceptor, "activateHandler");
        std::cout << "topology commit\n";
        topology.commit();
        POTHOS_TEST_TRUE(topology.waitInactive());
    }

    std::string lastWord = acceptor.call("getLastWord");
    POTHOS_TEST_EQUAL(lastWord, "hello");
}

//
// Test utility blocks
//

static Pothos::Object performEval(const Pothos::Proxy& evaluator, const std::string& expr)
{
    auto periodicTrigger = Pothos::BlockRegistry::make("/blocks/periodic_trigger");
    periodicTrigger.call("setRate", 5); // Triggers per second
    periodicTrigger.call("setArgs", std::vector<std::string>{expr});

    auto slotToMessage = Pothos::BlockRegistry::make(
                             "/blocks/slot_to_message",
                             "handleIt");
    auto collectorSink = Pothos::BlockRegistry::make(
                            "/blocks/collector_sink",
                            ""); // DType irrelevant

    {
        Pothos::Topology topology;

        topology.connect(periodicTrigger, "triggered", evaluator, "setExpression");
        topology.connect(evaluator, "triggered", slotToMessage, "handleIt");
        topology.connect(slotToMessage, 0, collectorSink, 0);

        // Since periodic_trigger will trigger 5 times/second, half a second
        // should be enough to get at least one.
        topology.commit();
        Poco::Thread::sleep(500); // ms
    }

    auto collectorSinkObjs = collectorSink.call<Pothos::ObjectVector>("getMessages");
    POTHOS_TEST_FALSE(collectorSinkObjs.empty());

    return collectorSinkObjs[0];
}

POTHOS_TEST_BLOCK("/proxy/python/tests", test_evaluator)
{
    constexpr auto jsonStr = "{\"outer\": [{\"inner\": [400,300,200,100]}, {\"inner\": [0.1,0.2,0.3,0.4]}]}";
    constexpr auto inner0Index = 3;
    constexpr auto inner1Index = 2;
    const auto localDoubles = std::vector<double>{12.3456789, 0.987654321};
    const std::complex<double> complexArg{1.351, 4.18};
    constexpr double doubleArg0 = 1234.0;
    constexpr double doubleArg1 = 5678.0;

    auto cppJSON = json::parse(jsonStr);

    const auto expectedResult = (cppJSON["outer"][0]["inner"][inner0Index].get<double>()
                              * cppJSON["outer"][1]["inner"][inner1Index].get<double>())
                              + std::pow((localDoubles[0] - std::pow(localDoubles[1] + std::abs(complexArg), 2)), 3)
                              - doubleArg0
                              + doubleArg1;

    auto evaluator = Pothos::BlockRegistry::make(
                         "/python/evaluator",
                         std::vector<std::string>{"inner0Index", "inner1Index", "complexArg", "doubleArg"});
    evaluator.call("setInner0Index", inner0Index);
    evaluator.call("setInner1Index", inner1Index);
    evaluator.call("setComplexArg", complexArg);
    evaluator.call("setDoubleArg", doubleArg0, doubleArg1);

    auto imports = std::vector<std::string>{"json","math","numpy"};
    evaluator.call("setImports", imports);

    auto env = Pothos::ProxyEnvironment::make("python");
    auto localVars = Pothos::ProxyMap
    {
        {env->makeProxy("testJSON"), env->makeProxy(jsonStr)},
        {env->makeProxy("localDoubles"), env->makeProxy(localDoubles)}
    };
    evaluator.call("setLocalVars", localVars);

    const std::string jsonExpr0 = "json.loads(testJSON)['outer'][0]['inner'][inner0Index]";
    const std::string jsonExpr1 = "json.loads(testJSON)['outer'][1]['inner'][inner1Index]";
    const std::string powExpr = "numpy.power([localDoubles[0] - math.pow(localDoubles[1] + abs(complexArg), 2)], 3)[0]";
    const auto expr = jsonExpr0 + " * " + jsonExpr1 + " + " + powExpr + "- doubleArg0 + doubleArg1";

    auto result = performEval(evaluator, expr);
    POTHOS_TEST_CLOSE(
        expectedResult,
        result.convert<double>(),
        1e-3);
}

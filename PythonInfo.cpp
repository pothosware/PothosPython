// Copyright (c) 2020 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>

#include <Poco/String.h>

#include <json.hpp>

#include <sstream>

static std::string __getPythonInfoJSON()
{
    auto env = Pothos::ProxyEnvironment::make("python");
    auto sys = env->findProxy("sys");

    nlohmann::json topObj;
    auto& pythonInfo = topObj["Python Info"];
    pythonInfo["Exec Prefix"] = sys.get<std::string>("exec_prefix");
    pythonInfo["Implementation"] = sys.get("implementation").get<std::string>("name");
    pythonInfo["Cache Tag"] = sys.get("implementation").get<std::string>("cache_tag");

    auto& versionInfo = pythonInfo["Version Info"];

    auto sysVersionInfo = sys.get("version_info");
    versionInfo["Major"] = sysVersionInfo.call<int, int>("__getitem__", 0);
    versionInfo["Minor"] = sysVersionInfo.call<int, int>("__getitem__", 1);
    versionInfo["Patch"] = sysVersionInfo.call<int, int>("__getitem__", 2);
    versionInfo["Release Level"] = sysVersionInfo.call<std::string>("__getitem__", 3);
    versionInfo["Serial"] = sysVersionInfo.call<int, int>("__getitem__", 4);

    auto fullVersionString = sys.get<std::string>("version");
    versionInfo["Version String"] = fullVersionString.substr(0, fullVersionString.find(" "));

    std::string hexVersion;
    std::stringstream hexVersionStream;
    hexVersionStream << "0x";
    hexVersionStream << std::hex << sys.get<size_t>("hexversion");
    hexVersionStream >> hexVersion;
    versionInfo["Version Hex"] = hexVersion;

    return topObj.dump();
}

static std::string getPythonInfoJSON()
{
    // Only do this once.
    static const std::string pythonInfoJSON = __getPythonInfoJSON();

    return pythonInfoJSON;
}

pothos_static_block(registerPythonInfo)
{
    Pothos::PluginRegistry::addCall(
        "/devices/python/info",
        Pothos::Callable(&getPythonInfoJSON));
}

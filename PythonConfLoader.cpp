// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Util/BlockDescription.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/System.hpp>
#include <Pothos/Proxy.hpp>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <map>

/***********************************************************************
 * The loader factory opens a python environment,
 * locates the specified module and class (or function),
 * and invokes it with the specified arguments.
 **********************************************************************/
static Pothos::Object opaquePythonLoaderFactory(
    const Poco::Path &rootPath,
    const std::string &targetName,
    const std::string &moduleName,
    const std::string &functionName,
    const Pothos::Object *args,
    const size_t numArgs)
{
    //create python environment
    auto env = Pothos::ProxyEnvironment::make("python");

    //add to the system path
    //TODO only add if not already added...
    auto sys = env->findProxy("sys");
    auto sysPath = sys.callProxy("get:path");
    sysPath.callProxy("append", rootPath.toString());

    //convert arguments into proxy environment
    std::vector<Pothos::Proxy> proxyArgs(numArgs);
    for (size_t i = 0; i < numArgs; i++)
    {
        proxyArgs[i] = env->makeProxy(args[i]);
    }

    //locate the module
    auto mod = env->findProxy(moduleName);

    //call into the factory
    auto block = mod.getHandle()->call(functionName, proxyArgs.data(), proxyArgs.size());
    return Pothos::Object(block);
}

/***********************************************************************
 * register the loader factory for every block factory
 **********************************************************************/
static std::vector<Pothos::PluginPath> PythonLoader(const std::map<std::string, std::string> &config)
{
    std::vector<Pothos::PluginPath> entries;
    std::vector<Pothos::PluginPath> factories;
    const auto tokOptions = Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_TRIM;

    //config file path set by caller
    const auto confFilePathIt = config.find("confFilePath");
    if (confFilePathIt == config.end() or confFilePathIt->second.empty())
        throw Pothos::Exception("missing confFilePath");
    const auto rootDir = Poco::Path(confFilePathIt->second).makeParent();

    //config section set by caller
    const auto confFileSectionIt = config.find("confFileSection");
    if (confFileSectionIt == config.end() or confFileSectionIt->second.empty())
        throw Pothos::Exception("missing confFileSection");
    const auto &target = confFileSectionIt->second;

    //extract module name
    const auto moduleIt = config.find("module");
    if (moduleIt == config.end() or moduleIt->second.empty())
        throw Pothos::Exception("missing module");
    const auto &module = moduleIt->second;

    //extract function name (class path or function name)
    const auto functionIt = config.find("function");
    if (functionIt == config.end() or functionIt->second.empty())
        throw Pothos::Exception("missing function");
    const auto &function = functionIt->second;

    //doc sources: scan sources unless doc sources are specified
    std::vector<std::string> docSources;
    const auto docSourcesIt = config.find("doc_sources");
    if (docSourcesIt != config.end()) for (const auto &docSource :
        Poco::StringTokenizer(docSourcesIt->second, ",", tokOptions))
    {
        const auto absPath = Poco::Path(docSource).makeAbsolute(rootDir);
        docSources.push_back(absPath.toString());
    }
    else {} //TODO scan for *.py?

    //load the factories: use this when providing no block description
    const auto factoriesIt = config.find("factories");
    if (factoriesIt != config.end()) for (const auto &factory :
        Poco::StringTokenizer(factoriesIt->second, ",", tokOptions))
    {
        factories.push_back(Pothos::PluginPath("/blocks", factory));
    }

    //generate JSON block descriptions
    Pothos::Util::BlockDescriptionParser parser;
    for (const auto &source : docSources) parser.feedFilePath(source);

    //store block paths in handle, and store doc paths
    for (const auto &factory : parser.listFactories())
    {
        const auto pluginPath = Pothos::PluginPath("/blocks/docs", factory);
        Pothos::PluginRegistry::add(pluginPath, parser.getJSONObject(factory));
        entries.push_back(pluginPath);
        factories.push_back(Pothos::PluginPath("/blocks", factory));
    }

    //register for all factory paths
    for (const auto &pluginPath : factories)
    {
        const auto factory = Pothos::Callable(&opaquePythonLoaderFactory)
            .bind(rootDir, 0)
            .bind(target, 1)
            .bind(module, 2)
            .bind(function, 3);
        Pothos::PluginRegistry::addCall(pluginPath, factory);
        entries.push_back(pluginPath);
    }

    return entries;
}

/***********************************************************************
 * loader registration
 **********************************************************************/
pothos_static_block(pothosFrameworkRegisterPythonLoader)
{
    Pothos::PluginRegistry::addCall("/framework/conf_loader/python", &PythonLoader);
}

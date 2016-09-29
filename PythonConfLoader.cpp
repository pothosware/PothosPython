// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

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
    const std::string &moduleName,
    const std::string &className,
    const Pothos::Object *args,
    const size_t numArgs)
{
    //create python environment
    auto env = Pothos::ProxyEnvironment::make("python");

    //convert arguments into proxy environment
    std::vector<Pothos::Proxy> proxyArgs(numArgs);
    for (size_t i = 0; i < numArgs; i++)
    {
        proxyArgs[i] = env->makeProxy(args[i]);
    }

    //locate the module
    auto mod = env->findProxy(moduleName);

    //call into the factory
    auto block = mod.getHandle()->call(className, proxyArgs.data(), proxyArgs.size());
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
        factories.push_back(Pothos::PluginPath("/blocks").join(factory.substr(1)));
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

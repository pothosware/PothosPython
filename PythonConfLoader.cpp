// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/System/Version.hpp>
#if POTHOS_API_VERSION >= 0x00050000
#include <Pothos/Util/BlockDescription.hpp>
#include <Pothos/Plugin.hpp>
#include <Pothos/System.hpp>
#include <Pothos/Proxy.hpp>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/StringTokenizer.h>
#include <tuple>
#include <map>

/***********************************************************************
 * Recursive traverse for python files
 **********************************************************************/
static std::vector<Poco::Path> getPythonFiles(const Poco::Path &path)
{
    std::vector<Poco::Path> paths;

    const Poco::File file(path);
    if (not file.exists()) return paths;
    else if (file.isFile() and (path.getExtension() == "py"))
    {
        paths.push_back(path);
    }
    else if (file.isDirectory())
    {
        std::vector<std::string> files; file.list(files);
        for (size_t i = 0; i < files.size(); i++)
        {
            auto subpaths = getPythonFiles(Poco::Path(path, files[i]).absolute());
            paths.insert(paths.end(), subpaths.begin(), subpaths.end());
        }
    }

    return paths;
}

/***********************************************************************
 * The loader factory opens a python environment,
 * locates the specified module and class (or function),
 * and invokes it with the specified arguments.
 **********************************************************************/
static Pothos::Object opaquePythonLoaderFactory(
    const Poco::Path &rootPath,
    const std::string &moduleName,
    const std::string &functionName,
    const Pothos::Object *args,
    const size_t numArgs)
{
    //create python environment
    auto env = Pothos::ProxyEnvironment::make("python");

    //add to the system path
    auto sys = env->findProxy("sys");
    auto sysPath = sys.callProxy("get:path");
    try
    {
        //throws if the path is not found already
        sysPath.callProxy("index", rootPath.toString());
    }
    catch (...)
    {
        //and so we add our path here
        sysPath.callProxy("append", rootPath.toString());
    }

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
    std::vector<std::tuple<Pothos::PluginPath, std::string, std::string>> factories;
    const auto tokOptions = Poco::StringTokenizer::TOK_IGNORE_EMPTY | Poco::StringTokenizer::TOK_TRIM;
    const std::string tokSep(" \t");

    //config file path set by caller
    const auto confFilePathIt = config.find("confFilePath");
    if (confFilePathIt == config.end() or confFilePathIt->second.empty())
        throw Pothos::Exception("missing confFilePath");
    const auto rootDir = Poco::Path(confFilePathIt->second).makeParent();

    //doc sources: scan sources unless doc sources are specified
    std::vector<std::string> docSources;
    const auto docSourcesIt = config.find("doc_sources");
    if (docSourcesIt != config.end()) for (const auto &docSource :
        Poco::StringTokenizer(docSourcesIt->second, tokSep, tokOptions))
    {
        const auto absPath = Poco::Path(docSource).makeAbsolute(rootDir);
        docSources.push_back(absPath.toString());
    }

    //otherwise scan all .py files in this directory
    else for (const auto &path : getPythonFiles(rootDir))
    {
        docSources.push_back(path.toString());
    }

    //load the factories: use this when providing no block description
    const auto factoriesIt = config.find("factories");
    if (factoriesIt == config.end()) throw Pothos::Exception("missing factories");
    for (const auto &factoryMarkup :
        Poco::StringTokenizer(factoriesIt->second, tokSep, tokOptions))
    {
        const auto colonPos = factoryMarkup.find(':');
        const auto lastDotPos = factoryMarkup.find_last_of('.');
        if (colonPos == std::string::npos or lastDotPos == std::string::npos or colonPos > lastDotPos)
        {
            throw Pothos::Exception("factory entry not in format /block/path:Module.Function");
        }
        const auto path = Pothos::PluginPath("/blocks", factoryMarkup.substr(0, colonPos));
        const auto module = factoryMarkup.substr(colonPos+1, (lastDotPos-colonPos)-1);
        const auto function = factoryMarkup.substr(lastDotPos+1);
        factories.emplace_back(path, module, function);
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
    }

    //register for all factory paths
    for (const auto &factoryTuple : factories)
    {
        const auto &pluginPath = std::get<0>(factoryTuple);
        const auto factory = Pothos::Callable(&opaquePythonLoaderFactory)
            .bind(rootDir, 0)
            .bind(std::get<1>(factoryTuple), 1)
            .bind(std::get<2>(factoryTuple), 2);
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

#endif //POTHOS_API_VERSION >= 0x00050000

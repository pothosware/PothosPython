// Copyright (c) 2013-2020 Josh Blum
//                    2021 Nicholas Corgan
// SPDX-License-Identifier: BSL-1.0

#include "PythonSupport.hpp"
#include "PythonProxy.hpp"
#include <Pothos/Plugin.hpp>
#include <Pothos/Callable.hpp>
#include <Poco/SingletonHolder.h>
#include <Pothos/System/Paths.hpp>
#include <Poco/Path.h>

/***********************************************************************
 * Per process Python interp init and cleanup
 **********************************************************************/
struct PythonInterpWrapper
{
    PythonInterpWrapper(void):
        _s(nullptr)
    {
        Py_Initialize();
#if PY_VERSION_HEX < 0x03070000
        // Python 3.7: automatically called by Py_Initialize()
        // Python 3.9: deprecated, does nothing
        // Python 3.11: removed
        PyEval_InitThreads();
#endif
        _s = PyEval_SaveThread();
    }

    ~PythonInterpWrapper(void)
    {
        PyEval_RestoreThread(_s);
        Py_Finalize();
    }
    PyThreadState *_s;
};

static PythonInterpWrapper &getPythonInterpWrapper(void)
{
    static Poco::SingletonHolder<PythonInterpWrapper> sh;
    return *sh.get();
}

/***********************************************************************
 * PythonProxyEnvironment methods
 **********************************************************************/
PythonProxyEnvironment::PythonProxyEnvironment(const Pothos::ProxyEnvironmentArgs &)
{
    return;
}

Pothos::Proxy PythonProxyEnvironment::makeHandle(PyObject *obj, const bool borrowed)
{
    auto env = std::dynamic_pointer_cast<PythonProxyEnvironment>(this->shared_from_this());
    return Pothos::Proxy(new PythonProxyHandle(env, obj, borrowed));
}

Pothos::Proxy PythonProxyEnvironment::makeHandle(const PyObjectRef &ref)
{
    return this->makeHandle(ref.obj, REF_BORROWED);
}

std::shared_ptr<PythonProxyHandle> PythonProxyEnvironment::getHandle(const Pothos::Proxy &proxy)
{
    PyGilStateLock lock;
    Pothos::Proxy myProxy = proxy;
    if (proxy.getEnvironment() != this->shared_from_this())
    {
        myProxy = this->convertObjectToProxy(proxy.toObject());
    }
    auto handle = std::dynamic_pointer_cast<PythonProxyHandle>(myProxy.getHandle());
    assert(handle);
    return handle;
}

Pothos::Proxy PythonProxyEnvironment::findProxy(const std::string &name)
{
    PyGilStateLock lock;
    PyObjectRef module(PyImport_ImportModule(name.c_str()), REF_NEW);
    if (module.obj == nullptr) throw Pothos::ProxyEnvironmentFindError(
        "PythonProxyEnvironment::findProxy("+name+")", getErrorString());
    return this->makeHandle(module);
}

Pothos::Proxy PythonProxyEnvironment::convertObjectToProxy(const Pothos::Object &local)
{
    PyGilStateLock lock;
    try
    {
        return Pothos::ProxyEnvironment::convertObjectToProxy(local);
    }
    catch (const Pothos::ProxyEnvironmentConvertError &)
    {
        auto env = Pothos::ProxyEnvironment::make("managed");
        auto proxy = env->convertObjectToProxy(local);
        return this->makeProxy(proxy);
    }
}

Pothos::Object PythonProxyEnvironment::convertProxyToObject(const Pothos::Proxy &proxy)
{
    PyGilStateLock lock;
    auto r = Pothos::ProxyEnvironment::convertProxyToObject(proxy);
    if (r.type() == typeid(Pothos::Object)) return r.extract<Pothos::Object>();
    return r;
}

void PythonProxyEnvironment::serialize(const Pothos::Proxy &proxy, std::ostream &os)
{
    try
    {
        auto marshal = this->findProxy("marshal");
        const std::vector<char> data = marshal.call("dumps", proxy);
        os.write(data.data(), data.size());
    }
    catch (const Pothos::Exception &ex)
    {
        throw Pothos::ProxySerializeError("PythonProxyEnvironment::serialize()", ex);
    }
}

Pothos::Proxy PythonProxyEnvironment::deserialize(std::istream &is)
{
    is.seekg (0, std::ios_base::end);
    const auto length = is.tellg();
    is.seekg (0, std::ios_base::beg);
    std::vector<char> bytes(length);
    is.read(bytes.data(), bytes.size());

    try
    {
        auto marshal = this->findProxy("marshal");
        return marshal.call("loads", bytes);
    }
    catch (const Pothos::Exception &ex)
    {
        throw Pothos::ProxySerializeError("PythonProxyEnvironment::deserialize()", ex);
    }
}

/***********************************************************************
 * factory registration
 **********************************************************************/
Pothos::ProxyEnvironment::Sptr makePythonProxyEnvironment(const Pothos::ProxyEnvironmentArgs &args)
{
    auto env = Pothos::ProxyEnvironment::Sptr(new PythonProxyEnvironment(args));

    //The interpreter might already be initialized if python is the caller
    if (Py_IsInitialized()) return env;

    getPythonInterpWrapper();

    auto sys = env->findProxy("sys");
    sys.call("set:dont_write_bytecode", true);

    Poco::Path pythonPath(Pothos::System::getRootPath());
    pythonPath.append(POTHOS_PYTHON_DIR);

    auto sysPath = sys.call("get:path");
    sysPath.call("append", pythonPath.toString());

    env->findProxy("Pothos"); //registers important converters

    return env;
}

pothos_static_block(pothosRegisterPythonProxy)
{
    Pothos::PluginRegistry::addCall(
        "/proxy/environment/python",
        &makePythonProxyEnvironment);
}

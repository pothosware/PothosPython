// Copyright (c) 2014-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PythonSupport.hpp"
#include "PythonProxy.hpp"
#include <Pothos/Plugin.hpp>
#include <cassert>

/***********************************************************************
 * PyObject helpers - used in python bindings
 **********************************************************************/
static Pothos::Proxy convertPyObjectToProxy(Pothos::ProxyEnvironment::Sptr env, PyObject *obj)
{
    return std::dynamic_pointer_cast<PythonProxyEnvironment>(env)->makeHandle(obj, REF_BORROWED);
}

static PyObject *convertProxyToPyObject(const Pothos::Proxy &proxy)
{
    assert(proxy);
    assert(std::dynamic_pointer_cast<PythonProxyHandle>(proxy.getHandle()));
    return std::dynamic_pointer_cast<PythonProxyHandle>(proxy.getHandle())->ref.newRef();
}

pothos_static_block(pothosRegisterPyObjectHelpers)
{
    Pothos::PluginRegistry::add("/proxy_helpers/" POTHOS_PYNAME "/pyobject_to_proxy",
        PyObjectToProxyFcn(&convertPyObjectToProxy));
    Pothos::PluginRegistry::add("/proxy_helpers/" POTHOS_PYNAME "/proxy_to_pyobject",
        ProxyToPyObjectFcn(&convertProxyToPyObject));
}

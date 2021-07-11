// Copyright (c) 2014-2021 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Managed.hpp>
#include <Pothos/Proxy.hpp>

class PythonBlock : Pothos::Block
{
public:
    PythonBlock(void)
    {
        this->registerCall(this, POTHOS_FCN_TUPLE(PythonBlock, _setPyBlock));
    }

    static Block *make(void)
    {
        return new PythonBlock();
    }

    void _setPyBlock(const Pothos::Proxy &block)
    {
        _block = block;
    }

    void work(void)
    {
        _block.call("work");
    }

    void activate(void)
    {
        _block.call("activate");
    }

    void deactivate(void)
    {
        _block.call("deactivate");
    }

    void propagateLabels(const Pothos::InputPort *input)
    {
        //forward to wrapper that takes input port name
        auto not_implemeneted = _block.call<bool>("_propagateLabels", input->name());

        //if the overload was not implemented, call base function
        if (not_implemeneted) Pothos::Block::propagateLabels(input);
    }

    Pothos::Object opaqueCallHandler(const std::string &name, const Pothos::Object *inputArgs, const size_t numArgs)
    {
        if (name == "_setPyBlock") return Pothos::Block::opaqueCallHandler(name, inputArgs, numArgs);
        if (not _block) throw name;
        auto env = _block.getEnvironment();
        Pothos::ProxyVector args(numArgs);
        for (size_t i = 0; i < numArgs; i++)
        {
            args[i] = env->convertObjectToProxy(inputArgs[i]);
        }
        auto result = _block.getHandle()->call(name, args.data(), args.size());
        return env->convertProxyToObject(result);
    }

    Pothos::Proxy _block;
};

static Pothos::BlockRegistry registerPythonBlock(
    "/blocks/python_block", &PythonBlock::make);

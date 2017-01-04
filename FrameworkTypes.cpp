// Copyright (c) 2016-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Framework/BufferChunk.hpp>

/***********************************************************************
 * buffer chunk to/from numpy
 **********************************************************************/
static Pothos::Proxy convertBufferChunkToNumpyArray(Pothos::ProxyEnvironment::Sptr env, const Pothos::BufferChunk &buffer)
{
    auto module = env->findProxy("Pothos.Buffer");
    auto dtype = module.get("dtype_to_numpy")(buffer.dtype);
    return module.get("pointer_to_ndarray")(buffer.address, buffer.elements(), dtype);
}

static Pothos::BufferChunk convertNumpyArrayToBufferChunk(const Pothos::Proxy &npArray)
{
    //extract shape and data type information
    const auto shape = npArray.get<Pothos::ProxyVector>("shape");
    const size_t numBytes = npArray.get<size_t>("nbytes");
    const size_t dimension = (shape.size() > 1)? shape.at(1).convert<size_t>() : 1;
    const auto dtypeName = npArray.get("dtype").get<std::string>("name");
    const Pothos::DType dtype(dtypeName, dimension);
    const auto address = npArray.get("__array_interface__").callProxy("get", "data").call<size_t>("__getitem__", 0);

    //create a shared buffer that holds the numpy array
    auto sharedBuff = Pothos::SharedBuffer(address, numBytes, npArray.getHandle());

    //now create a buffer chunk of that shared buffer with matching dtype
    auto chunk = Pothos::BufferChunk(sharedBuff);
    chunk.dtype = dtype;
    return chunk;
}

pothos_static_block(pothosRegisterNumpyBufferConversions)
{
    Pothos::PluginRegistry::addCall("/proxy/converters/python/buffer_chunk_to_numpy_array",
        &convertBufferChunkToNumpyArray);
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_array_to_buffer_chunk",
        Pothos::ProxyConvertPair("numpy.ndarray", &convertNumpyArrayToBufferChunk));
}

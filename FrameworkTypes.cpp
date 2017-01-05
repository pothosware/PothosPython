// Copyright (c) 2016-2017 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Plugin.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Framework/BufferChunk.hpp>
#include <complex>
#include <cstdint>

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

template <typename T>
static T convertNumpyIntegerToNative(const Pothos::Proxy &num)
{
    return num.call<T>("__int__");
}

template <typename T>
static T convertNumpyFloatToNative(const Pothos::Proxy &num)
{
    return num.call<T>("__float__");
}

template <typename T>
static std::complex<T> convertNumpyComplexToNative(const Pothos::Proxy &num)
{
    return std::complex<T>(num.get<T>("real"), num.get<T>("imag"));
}

pothos_static_block(pothosRegisterNumpyBufferConversions)
{
    Pothos::PluginRegistry::addCall("/proxy/converters/python/buffer_chunk_to_numpy_array",
        &convertBufferChunkToNumpyArray);
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_array_to_buffer_chunk",
        Pothos::ProxyConvertPair("numpy.ndarray", &convertNumpyArrayToBufferChunk));

    //integer types
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_int8_to_int8",
        Pothos::ProxyConvertPair("numpy.int8", &convertNumpyIntegerToNative<int8_t>));
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_uint8_to_uint8",
        Pothos::ProxyConvertPair("numpy.uint8", &convertNumpyIntegerToNative<uint8_t>));

    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_int16_to_int16",
        Pothos::ProxyConvertPair("numpy.int16", &convertNumpyIntegerToNative<int16_t>));
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_uint16_to_uint16",
        Pothos::ProxyConvertPair("numpy.uint16", &convertNumpyIntegerToNative<uint16_t>));

    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_int32_to_int32",
        Pothos::ProxyConvertPair("numpy.int32", &convertNumpyIntegerToNative<int32_t>));
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_uint32_to_uint32",
        Pothos::ProxyConvertPair("numpy.uint32", &convertNumpyIntegerToNative<uint32_t>));

    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_int64_to_int64",
        Pothos::ProxyConvertPair("numpy.int64", &convertNumpyIntegerToNative<int64_t>));
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_uint64_to_uint64",
        Pothos::ProxyConvertPair("numpy.uint64", &convertNumpyIntegerToNative<uint64_t>));

    //float types
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_float16_to_float",
        Pothos::ProxyConvertPair("numpy.float16", &convertNumpyFloatToNative<float>));

    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_float32_to_float",
        Pothos::ProxyConvertPair("numpy.float32", &convertNumpyFloatToNative<float>));

    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_float64_to_double",
        Pothos::ProxyConvertPair("numpy.float64", &convertNumpyFloatToNative<double>));

    //complex types
    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_complex64_to_complex_float",
        Pothos::ProxyConvertPair("numpy.complex64", &convertNumpyComplexToNative<float>));

    Pothos::PluginRegistry::add("/proxy/converters/python/numpy_complex128_to_complex_double",
        Pothos::ProxyConvertPair("numpy.complex128", &convertNumpyComplexToNative<double>));
}

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosPythonTestBlocks)
enable_language(CXX)

find_package(Pothos CONFIG REQUIRED)
include(PothosPythonUtil)

########################################################################
## Build and install tester blocks for python bindings
########################################################################
POTHOS_PYTHON_UTIL(
    TARGET PythonTestBlocks
    SOURCES
        __init__.py
        Forwarder.py
        SimpleSigSlots.py
    FACTORIES
        "/python${POTHOS_PYVER}/forwarder:Forwarder"
        "/python${POTHOS_PYVER}/simple_signal_emitter:SimpleSignalEmitter"
        "/python${POTHOS_PYVER}/simple_slot_acceptor:SimpleSlotAcceptor"
    DESTINATION PothosTestBlocks
    ENABLE_DOCS
)

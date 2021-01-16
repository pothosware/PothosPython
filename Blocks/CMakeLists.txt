########################################################################
## Build and install
########################################################################
POTHOS_PYTHON_UTIL(
    TARGET PythonBlocks
    SOURCES
        __init__.py
        Evaluator.py
    FACTORIES
        "/python/evaluator:Evaluator"
    DESTINATION PothosBlocks
    ENABLE_DOCS
)

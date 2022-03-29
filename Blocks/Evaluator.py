# Copyright (c) 2020-2021 Nicholas Corgan
# SPDX-License-Identifier: BSL-1.0

from functools import partial
import importlib

import Pothos

"""
/***********************************************************************
 * |PothosDoc Python Evaluator
 *
 * The Python evaluator block performs a user-specified expression evaluation
 * on input slot(s) and produces the evaluation result on an output signal.
 * The input slots are user-defined. The output signal is named "triggered".
 * The arguments from the input slots must be primitive types.
 *
 * |category /Event
 * |keywords signal slot eval expression
 *
 * |param imports[Imports] A list of Python modules to import before executing the expression.
 * Example: ["math", "numpy"] will import the math and numpy modules.
 * |default ["math"]
 *
 * |param args[Arguments] A list of named variables to use in the expression.
 * Each variable corresponds to settings slot on the transform block.
 * Example: ["foo", "bar"] will create the slots "setFoo" and "setBar".
 * |default ["val"]
 *
 * |param expr[Expression] The expression to re-evaluate for each slot event.
 * An expression is valid Python, comprised of combinations of variables, constants, and math functions.
 * Example: math.log2(foo)/bar
 *
 * <p><b>Multi-argument input:</b> Upstream blocks may pass multiple arguments to a slot.
 * Each argument will be available to the expression suffixed by its argument index.
 * For example, suppose that the slot "setBaz" has two arguments,
 * then the following expression would use both arguments: "baz0 + baz1"</p>
 *
 * |default "math.log2(val)"
 * |widget StringEntry()
 *
 * |param localVars[LocalVars] A map of variable names to values.
 * This allows you to use global variables from the topology in the expression.
 *
 * For example this mapping lets us use foo, bar, and baz in the expression
 * to represent several different globals and combinations of expressions:
 * {"foo": myGlobal, "bar": "test123", "baz": myNum+12345}
 * |default {}
 * |preview valid
 *
 * |factory /python/evaluator(args)
 * |setter setExpression(expr)
 * |setter setImports(imports)
 * |setter setLocalVars(localVars)
 **********************************************************************/
"""
class Evaluator(Pothos.Block):
    def __init__(self, varNames):
        Pothos.Block.__init__(self)
        self.setName("/python/evaluator")

        self.__checkIsStringList(varNames)

        self.__expr = ""
        self.__localVars = dict()
        self.__varNames = varNames
        self.__varValues = dict()
        self.__imports = []
        self.__varsReady = set()

        # Add setters for user variables
        for name in self.__varNames:
            if not name:
                continue

            setterName = "set"+name[0].upper()+name[1:]
            setattr(Evaluator, setterName, partial(self.__setter, name))
            self.registerSlot(setterName)

        self.registerSlot("setExpression")
        self.registerSlot("setImports")
        self.registerSlot("setLocalVars")
        self.registerSignal("triggered")

    def getExpression(self):
        return self.__expr

    def setExpression(self,expr):
        self.__checkIsStr(expr)
        self.__expr = expr

        notReadyVars = [var for var in self.__varNames if var not in self.__varsReady]
        if notReadyVars:
            return

        args = self.__performEval()
        self.triggered(args)

    def setImports(self,imports):
        self.__checkIsStringOrStringList(imports)
        self.__imports = imports if (type(imports) == list) else [imports]

    def getImports(self):
        return self.__imports

    def setLocalVars(self,userLocalVars):
        self.__checkIsDict(userLocalVars)
        self.__localVars = userLocalVars

    #
    # Private utility functions
    #

    def __performEval(self):
        for key,val in self.__varValues.items():
            locals()[key] = val

        for mod in self.__imports:
            exec("import "+mod)

        for key,val in self.__localVars.items():
            locals()[key] = val

        return eval(self.__expr)

    def __setter(self,field,*args):
        if len(args) > 1:
            for i in range(len(args)):
                self.__varValues[field+str(i)] = args[i]
        else:
            self.__varValues[field] = args[0]

        self.__varsReady.add(field)

        notReadyVars = [var for var in self.__varNames if var not in self.__varsReady]
        if (not notReadyVars) and self.__expr:
            args = self.__performEval()
            self.triggered(args)

        return None

    def __checkIsStr(self,var):
        if type(var) != str:
            raise ValueError("The given value must be a str. Found {0}".format(type(var)))

    def __checkIsDict(self,var):
        if type(var) != dict:
            raise ValueError("The given value must be a dict. Found {0}".format(type(var)))

    def __checkIsList(self,var):
        if type(var) != list:
            raise ValueError("The given value must be a list. Found {0}".format(type(var)))

    def __checkIsStringList(self,var):
        self.__checkIsList(var)

        nonStringVals = [x for x in var if type(x) != str]
        if nonStringVals:
            raise ValueError("All list values must be strings. Found {0}".format(type(nonStringVals[0])))

    def __checkIsStringOrStringList(self,var):
        if type(var) is str:
            return
        elif type(var) is list:
            self.__checkIsStringList(var)
        else:
            raise ValueError("The given value must be a string or list. Found {0}".format(type(var)))

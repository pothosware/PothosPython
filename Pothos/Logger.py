# Copyright (c) 2016-2016 Josh Blum
#                    2019 Nicholas Corgan
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *
import logging

class LogHandler(logging.Handler):
    def __init__(self, name):
        logging.Handler.__init__(self)
        env = ProxyEnvironment("managed")
        self._logger = env.findProxy("Pothos/Python/Logger")(name)

    def emit(self, record):
        level = {
            logging.FATAL:'FATAL',
            logging.CRITICAL:'CRITICAL',
            logging.ERROR:'ERROR',
            logging.WARNING:'WARNING',
            logging.INFO:'INFO',
            logging.DEBUG:'DEBUG',
            logging.NOTSET:'NOTSET',
        }[record.levelno]
        self._logger.log(record.name, record.getMessage(), level)

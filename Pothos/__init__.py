# Copyright (c) 2014-2016 Josh Blum
#                    2019 Nicholas Corgan
# SPDX-License-Identifier: BSL-1.0

from . PothosModule import *
from . Block import Block
from . Label import Label, LabelIteratorRange
from . InputPort import InputPort
from . OutputPort import OutputPort
from . Topology import Topology
from . BlockRegistry import BlockRegistry
from . Logger import LogHandler
from . Packet import Packet

import logging

# logging.captureWarnings() redirects all outputs from the "warnings" module
# to a Python logger named "py.warnings". Adding our log handler to this logger
# results in all Python warnings being consumed by our infrastructure.
logging.basicConfig(level=logging.INFO)
logging.getLogger("py.warnings").addHandler(LogHandler("py.warnings"))
logging.captureWarnings(True)

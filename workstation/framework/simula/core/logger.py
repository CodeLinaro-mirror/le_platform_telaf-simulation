#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import logging
import os

_logger_name = "simula"

L = logging.getLogger(_logger_name)
L.setLevel(logging.DEBUG)

_file_handler = logging.FileHandler(os.path.join("/tmp", _logger_name + ".log"), encoding='utf-8')
_file_handler.setLevel(logging.DEBUG)

_console_handler = logging.StreamHandler()
_console_handler.setLevel(logging.INFO)

_formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
_file_handler.setFormatter(_formatter)
_console_handler.setFormatter(_formatter)

L.addHandler(_file_handler)
L.addHandler(_console_handler)

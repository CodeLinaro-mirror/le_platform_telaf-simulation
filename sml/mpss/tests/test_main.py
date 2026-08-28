# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from unittest.mock import MagicMock

from sml.mpss.__main__ import _register_default_net
from sml.mpss.net import NetSubsystem


def test_default_net_registered_without_scenario():
    client = MagicMock()

    subsystem = _register_default_net(client)

    assert isinstance(subsystem, NetSubsystem)
    assert subsystem._slot_id == 1
    client.register_subsystem.assert_called_once_with(subsystem)

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from unittest.mock import MagicMock



from sml.mpss.__main__ import _register_default_net_modem
from sml.mpss.net_modem import NetModemSubsystem



def test_default_net_modem_registered_without_scenario():
    """MPSS keeps only the modem-owned slice of the net domain.

    The other five families moved to APSS (see sml/apss/net/__init__.py), so
    this is now the only net registration mpss performs.
    """
    client = MagicMock()


    subsystem = _register_default_net_modem(client)


    assert isinstance(subsystem, NetModemSubsystem)
    assert subsystem._slot_id == 1
    client.register_subsystem.assert_called_once_with(subsystem)


def test_mpss_no_longer_exposes_the_net_subsystem():
    """Guards against the old MPSS-side net package being reintroduced.

    ``sml.mpss.net`` was moved wholesale to ``sml.apss.net``; if it ever comes
    back, two processes would own the same topics.
    """
    import importlib

    try:
        importlib.import_module("sml.mpss.net")
    except ImportError:
        return
    raise AssertionError(
        "sml.mpss.net still importable -- the net domain now lives in "
        "sml.apss.net; a resurrected MPSS copy would double-own the net topics"
    )

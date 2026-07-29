# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Runtime persisted-state primitives: atomic JSON write/read.

Used by collection stores (currently only `sml.mpss.data.profile.
ProfileStore`) that are marked `persistent` in a `devices/*.yaml`'s
`DevicesDoc.persistent` list, to survive an mpss process restart. Sync,
blocking, whole-file writes only -- no incremental/WAL, no file locking:
current callers all run on the single `MqttClient` AO thread, so there is
no real concurrent writer to guard against.
"""
from __future__ import annotations

import json
import logging
import os
import tempfile
from pathlib import Path
from typing import Optional

_log = logging.getLogger("sml.runtime.persist")


def atomic_write_json(path: Path, data) -> None:
    """Write `data` as JSON to `path`, atomically (temp file + os.replace).

    Creates `path`'s parent directories if they don't exist yet.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(dir=path.parent, prefix=f".{path.name}.", suffix=".tmp")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as fh:
            json.dump(data, fh)
        os.replace(tmp_name, path)
    except Exception:
        try:
            os.unlink(tmp_name)
        except OSError:
            pass
        raise


def read_json(path: Path) -> Optional[dict]:
    """Read and parse `path` as JSON.

    Returns `None` if the file doesn't exist, or if it exists but fails to
    parse (also logged at ERROR -- a corrupt persist file has already lost
    data, worse than "never written"). Never raises: callers degrade to
    seed defaults on `None` rather than fail process startup.
    """
    try:
        with open(path, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except FileNotFoundError:
        return None
    except (OSError, json.JSONDecodeError) as exc:
        _log.error("persist file %s exists but failed to parse: %s", path, exc)
        return None


__all__ = ["atomic_write_json", "read_json"]

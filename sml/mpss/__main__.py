# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS Director — stub entry point. Replace with real implementation."""
import logging
import signal
import time

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(name)s %(levelname)s %(message)s",
)
log = logging.getLogger("sml.mpss")

_running = True


def _on_signal(signum, _frame):
    global _running
    log.info("sml.mpss: received signal %d, shutting down", signum)
    _running = False


def main() -> None:
    signal.signal(signal.SIGTERM, _on_signal)
    signal.signal(signal.SIGINT, _on_signal)
    log.info("sml.mpss: started (stub — awaiting implementation)")
    while _running:
        time.sleep(1)
    log.info("sml.mpss: stopped")


if __name__ == "__main__":
    main()

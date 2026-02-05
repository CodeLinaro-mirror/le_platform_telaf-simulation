# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import struct
import json
import os

fifo_path = "/tmp/radio_event"

data = {
    "ims_status": 2,
}

payload = json.dumps(data).encode('utf-8')

header = struct.pack(
    ">HH",
    len(payload) + 2,
    0x0001
)

packet = header + payload

with open(fifo_path, "wb") as fifo:
    fifo.write(packet)

print("Sent:", packet)

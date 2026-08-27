# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

if [ "`id -u`" -eq 0 ]; then PS1="[Build Your Simulation] \u:\w # "; else PS1="[Build Your Simulation] \u:\w $ "; fi

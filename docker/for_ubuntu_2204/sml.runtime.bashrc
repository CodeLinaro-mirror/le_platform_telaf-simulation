# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

_sml_cc() {
    sml cc
    READLINE_LINE=""
    READLINE_POINT=0
}

bind -x '"\C-g":"_sml_cc"'

TELAF_IN_CONTAINER=yes
if [ "`id -u`" -eq 0 ]; then PS1="[TelAF Simulation] \u:\w # "; else PS1="[TelAF Simulation] \u:\w $ "; fi
if [ -f $HOME/simulation/up_simulation.sh ]; then . $HOME/simulation/up_simulation.sh; else echo "Not found $HOME/simulation/up_simulation.sh --> NO simulation"; exit 0; fi



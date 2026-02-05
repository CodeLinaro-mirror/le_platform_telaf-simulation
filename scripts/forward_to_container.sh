#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# TELAF_DEV_IN_CONTAINER is from container environement args

# current_dir=$(dirname "$0")
project_top="$TELAF_DEV_IN_CONTAINER"
telaf_root=$project_top/telaf

if [ "${M_CHANGETO}" == "TRUE" ];then
    cd $telaf_root
fi

if [ "${V_VERBOSE}" == "TRUE" ];then

    echo -e "\n-- container information record begin --"

    cat /etc/os-release

    echo -e "\nTelAF-Simulation Project : ${project_top}\n"

    echo "Environment Variables [BEG]"
    env
    echo "Environment Variables [END]"

    echo -e "-- container information record done --\n"
fi

if [ -n "${SHELL_ACTIONS}" ]; then
    if [ -e "${SHELL_ACTIONS}" ]; then
        bash ${SHELL_ACTIONS}
    else
        echo "SHELL_ACTIONS: [${SHELL_ACTIONS}], but not found."
    fi
else
    if [ -n "${I_SHELL}" ] && [ "${I_SHELL}" == "TRUE" ] ; then
        /bin/bash
    else
        echo "Nothing to do"
    fi
fi

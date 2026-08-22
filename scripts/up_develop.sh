#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

current_dir=$(dirname "$0")
simulation_base=$(realpath ${SIMULATION_HOME})
simulation_workstation=$(realpath ${SIMULATION_WORKDIR})
project_root=$(realpath ${TELAF_ROOT}/..)

i_shell=FALSE
v_verbose=FALSE
m_changeto=FALSE

while getopts ":ivm" opt; do
    case $opt in
        i) # shell for interaction
            i_shell=TRUE
            ;;
        v) # verbose for show system information
            v_verbose=TRUE
            ;;
        m) # change workspace to project/telaf
            m_changeto=TRUE
            ;;
        \?)
            echo "Invalid option: -$OPTARG"
            exit 1
            ;;
    esac
done

shift $((OPTIND - 1))

WHICH=$1

IMG_NAME=${IMG_NAME:="telaf_simulation_develop_$WHICH"}
IMG_VERSION=${IMG_VERSION:="1.0.0"}
CONTAINER_OPTIONS=${CONTAINER_OPTIONS:=""}

# Allocate a TTY only when stdin is a real terminal (not a CI pipe).
TTY_OPT=""
[ -t 0 ] && TTY_OPT="-t"

if [ "$i_shell" == "TRUE" ]; then
    CONTAINER_NAME=${CONTAINER_NAME:="telaf_simulation_develop_$WHICH"}

    # Caution: when we use '-d' to run container, you should check the log for aync jobs.
    BUILTIN_CONTAINER_OPTIONS=${BUILTIN_CONTAINER_OPTIONS:="-d -i ${TTY_OPT}"} # --privileged=true --net=bridge

    attach_container="docker exec -i ${TTY_OPT} ${CONTAINER_NAME} /bin/bash"

    cd ${simulation_workstation}

    container_status=$(docker inspect -f '{{.State.Status}}' "${CONTAINER_NAME}" 2>/dev/null)
    if [[ "${container_status}" == "running" ]]; then
        eval ${attach_container}
    else
        # Stale container (exited/created) blocks 'docker run --name', remove it first.
        if [ -n "${container_status}" ]; then
            docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1 || true
        fi
        docker run --name ${CONTAINER_NAME} \
            ${BUILTIN_CONTAINER_OPTIONS} \
            ${CONTAINER_OPTIONS} -u $(id -u):$(id -g) \
            -e TELAF_DEV_IN_CONTAINER=${project_root} \
            -e CPLUS_INCLUDE_PATH='/usr/include/python2.7/' \
            -e M_CHANGETO=${m_changeto} -e V_VERBOSE=${v_verbose} -e I_SHELL=${i_shell} \
            -v ${simulation_base}:/home/developer/simulation_ro:ro \
            -v ${project_root}:${project_root}:rw \
            -v ${current_dir}/example.gitconfig:/home/developer/.gitconfig \
            ${IMG_NAME}:${IMG_VERSION} && eval ${attach_container}
    fi

    exit 0

else

    # For now, fix the name of the script, not customize
    SHELL_ACTIONS=${simulation_workstation}/.simula.dev.action.sh

    if [ ! -s "${SHELL_ACTIONS}" ]; then
        echo "[up_develop] SHELL_ACTIONS missing or empty: ${SHELL_ACTIONS}"
        echo "  Non-interactive develop container needs a command file to execute."
        echo "  Provide one via: make simula-in-dev CMD='...'"
        echo "  Or write ${SHELL_ACTIONS} directly, then rerun."
        exit 1
    fi

    BUILTIN_CONTAINER_OPTIONS=${BUILTIN_CONTAINER_OPTIONS:="--rm -i ${TTY_OPT}"}
    # Caution: random name for this once command-container.
    # For 'within', it's passed from environment, like export.
    # Example: make simula within="'hostname && make simula-clean && make simulac'"
    docker run ${BUILTIN_CONTAINER_OPTIONS} -u $(id -u):$(id -g) \
        -e TELAF_DEV_IN_CONTAINER=${project_root} \
        -e CPLUS_INCLUDE_PATH='/usr/include/python2.7/' \
        -e M_CHANGETO=${m_changeto} -e V_VERBOSE=${v_verbose} \
        -e SHELL_ACTIONS=${SHELL_ACTIONS} \
        -v ${simulation_base}:/home/developer/simulation_ro:ro \
        -v ${project_root}:${project_root}:rw \
        -v ${current_dir}/example.gitconfig:/home/developer/.gitconfig \
        ${IMG_NAME}:${IMG_VERSION}
fi

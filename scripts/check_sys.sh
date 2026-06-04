#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

OS_VERSION=$(grep -oP 'VERSION_ID=\K"(.+)"' /etc/os-release | tr -d '"')

function check_system_version ()
{
    RESULT=0

    if [ "$OS_VERSION" = "22.04" ]
    then
        printf "%-30s ... %-20s ... [OK]\n" "Checking system version" "$OS_VERSION"
    else
        printf "%-30s ... %-20s ... [NOK] Unsupported Ubuntu Version. Ubuntu 22.04 is required for the build\n" "Checking system version" "$OS_VERSION"
        RESULT=1
    fi
    return $RESULT
}

function check_package_install ()
{
    RESULT=0
    for pkg in "$@"
    do
        if dpkg -s $pkg > /dev/null 2>&1; then
            printf "%-30s ... %-20s ... [OK]\n" "Checking package installed" "$pkg"
        else
            printf "%-30s ... %-20s ... [NOK] <--\n" "Checking package installed" "$pkg"
            RESULT=1
        fi
    done
    return $RESULT
}

function check_user_umask ()
{
    RESULT=0
    umask_val=`umask`
    if [ "$umask_val" = "0022" -o "$umask_val" = "0002" ]
    then
        printf "%-30s ... %-20s ... [OK]\n" "Checking user umask" "$umask_val"
    else
        printf "%-30s ... %-20s ... [NOK] <-- Please change your umask to '0022' or '0002' manually.\n" "Checking user umask" "$umask_val"
        RESULT=1
    fi
    return $RESULT
}

function check_gcc_gxx_version ()
{
    RESULT=0
    required_version="7.5.0"

    gcc_version=$(gcc --version | awk 'NR==1{print $NF}')
    gcc_version=$(echo "$gcc_version" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    gxx_version=$(g++ --version | awk 'NR==1{print $NF}')
    gxx_version=$(echo "$gxx_version" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

    if [[ "$(printf '%s\n' "$required_version" "$gcc_version" | sort -V | head -n1)" == "$required_version" ]]; then
        printf "%-30s ... %-20s ... [OK] >= $required_version\n" "Checking gcc version" "$gcc_version"
    else
        printf "%-30s ... %-20s ... [NOK] <-- version < $required_version\n" "Checking gcc version" "$gcc_version"
        RESULT=1
    fi

    if [[ "$(printf '%s\n' "$required_version" "$gxx_version" | sort -V | head -n1)" == "$required_version" ]]; then
        printf "%-30s ... %-20s ... [OK] >= $required_version\n" "Checking g++ version" "$gxx_version"
    else
        printf "%-30s ... %-20s ... [NOK] <-- version < $required_version\n" "Checking g++ version" "$gxx_version"
        RESULT=1
    fi

    return $RESULT
}

function check_docker_version ()
{
    RESULT=0
    required_version="20.10.0"

    # In dev-container, don't need to check the docker tool
    if [ -n "${TELAF_DEV_IN_CONTAINER}" ]; then
        return 0
    fi

    if ! command -V docker &>/dev/null; then
        printf "Please install the docker tool first ( >= $required_version )\n"
        return 1
    fi

    docker_version=$(docker --version | awk '{print $3}' | cut -d ',' -f1)

    if [[ "$(printf '%s\n' "$required_version" "$docker_version" | sort -V | head -n 1)" == "$required_version" ]]; then
        printf "%-30s ... %-20s ... [OK] >= $required_version\n" "Docker version" "$required_version"
    else
        printf "%-30s ... %-20s ... [NOK] version < $required_version\n" "Docker version" "$required_version"
        RESULT=1
    fi
    return $RESULT
}

ERR_EXIT='exit 1'

total_packages_checking="
    build-essential
    ca-certificates
    libssl-dev
    curl
    python3
    ninja-build
    cmake
    python3-setuptools
    git
    fakeroot
    file
    libcap-dev
    python2
"





if ! check_system_version ; then
    eval $ERR_EXIT
elif ! check_package_install $total_packages_checking ; then
    eval $ERR_EXIT
elif ! check_docker_version ; then
    eval $ERR_EXIT
elif ! check_gcc_gxx_version ; then
    eval $ERR_EXIT
elif ! check_user_umask ; then
    eval $ERR_EXIT
else
    exit 0
fi

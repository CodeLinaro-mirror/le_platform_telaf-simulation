#!/usr/bin/env bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

function append_msg_to_syslogd()
{
    logger -- "-------------------------- [ $@ ] --------------------------"
}

function list_between_marks()
{
    logread | sed -n "/${1}/,/${2}/p"
}

alias mark='append_msg_to_syslogd'
alias listm='list_between_marks'

# Need to know which CONF file we are using for 'supervisorctl' CLI
alias supervisorctl='supervisorctl -c /root/sml/supervisord.sml.conf'

# To use a proxy server for your container,remove the comment from the following lines and replace <your_proxy> with your
# proxy address and replace <your_port> with the appropriate port number for the proxy.
# Once you have set the proxy,docker will route network requests through the specified proxy server.

# export HTTP_PROXY=http://<your_proxy>:<your_port>
# export HTTPS_PROXY=http://<your_proxy>:<your_port>
# export NO_PROXY=localhost,127.0.0.1

# Try to export all environment variables for Common API
if [ -e /legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini ]; then
    export COMMONAPI_CONFIG=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini
    export VSOMEIP_CONFIGURATION=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/vsomeip-local.json
    export LD_LIBRARY_PATH=/legato/taf_rootfs/lib:$LD_LIBRARY_PATH
fi

# sml simulation module (sub-packages live under /root/sml)
export PYTHONPATH=/root:${PYTHONPATH}

# `sml <tool> [args...]` CLI wrapper (e.g. `sml mqttcli sub`) -- see
# sml/tools/bin/sml and sml/tools/__main__.py.
export PATH=/root/sml/tools/bin:${PATH}

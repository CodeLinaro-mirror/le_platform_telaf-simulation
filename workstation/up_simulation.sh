#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

if [[ "$0" = "$BASH_SOURCE" ]]; then
    SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
else # when source me
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi

if [ -n "${TELAF_IN_CONTAINER}" ]; then # [Docker-Container-Env]

    # Once again to ensure the 'ssh-server' to be accessed normally
    export TELAF_IN_CONTAINER=yes

    # Modify global environment variables for all users
    TOP_ENV=/etc/environment
    echo 'PATH="/legato/systems/current/bin:/legato/taf_rootfs/bin:/venv/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"' >> $TOP_ENV
    echo 'TELAF_IN_CONTAINER=yes' >> $TOP_ENV
    source "$TOP_ENV"

    source $HOME/simulation/framework/environ.sh

    export TELAF_OK=0
    export TELAF_ERR=1
    export TELAF_TRUE=1
    export TELAF_FALSE=0

    function change_PS1()
    {
        if [ -e "$HOME/.whoami" ]; then
            # prompt on master: [TelAF Simulation] :/path/to/here #
            # prompt on slavex: [TelAF Simulation] (slave@ip-address):/path/to/here #
            # Every time you login the container, the prompt should change accordingly
            export CONTAINER_WHO_AM_I=$(awk '{print $1}' $HOME/.whoami)
            if [ "$CONTAINER_WHO_AM_I" == "master" ];then
                export PS1='[\[\e[0;33m\]TelAF Simulation\[\e[0m\]] :\w \$ '
            else
                SLAVE_IP=$(awk '{print $2}' $HOME/.whoami)
                export PS1='[\[\e[0;33m\]TelAF Simulation\[\e[0m\]] (\[\e[1;31m\]\[\e[1m\]$CONTAINER_WHO_AM_I@$SLAVE_IP\[\e[0m\]):\w \$ '
            fi
        fi
    }

    change_PS1

    if [ "$CONTAINER_WHO_AM_I" == "master" ];then
        echo
        echo "Welcome to TelAF Simulation Environment, enter 'telaf start' to launch!"
        echo "# Caution: If you want to keep the DATA persistently, please put them into '/root/simulation' directory or volumes!"
        echo
    fi

    # Add a hook script for doing some thing every time you login
    if [ -f $HOME/simulation/.simula.always.sh ]; then
        source $HOME/simulation/.simula.always.sh
    fi

    # Touch the new loader configuration
    sync && ldconfig

    if [ -f /tmp/telaf_simulation_up ]; then
        return # multi-user access with ssh-tool ? keep ONLY once init-action.
    fi

    # Prepare some ssh client/server options in advance
    sed -i 's/^#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config
    sed -i 's/^#PrintLastLog.*/PrintLastLog no/' /etc/ssh/sshd_config
    # sed -i 's/^#LogLevel.*/LogLevel quiet/' /etc/ssh/sshd_config
    sed -i 's/^#   StrictHostKeyChecking.*/    StrictHostKeyChecking no/' /etc/ssh/ssh_config
    sed -i '/motd.dynamic/ s/^/#/' /etc/pam.d/sshd
    service ssh start >/dev/null 2>&1

    # Stop displaying "warranties" notice when logging in by ssh-client
    mkdir -p $HOME/.cache
    touch $HOME/.cache/motd.legal-displayed

    cnode_ipv4=$(hostname -i | grep -oE "\b([0-9]{1,3}\.){3}[0-9]{1,3}\b")
    if [ "$CONTAINER_WHO_AM_I" == "master" ]; then
        echo "$CONTAINER_NAME $cnode_ipv4" > $HOME/simulation/.simula.master
        # $HOME/.whoami to indicate the container node identity: master or slave
        echo "$CONTAINER_WHO_AM_I $cnode_ipv4 $CONTAINER_NAME" > $HOME/.whoami
    elif [ "$CONTAINER_WHO_AM_I" == "slave" ]; then
        echo "$CONTAINER_WHO_AM_I $cnode_ipv4 $CONTAINER_NAME" > $HOME/.whoami
    fi

    # Create SSH key for access between different containers [master -> slavex]
    M_SSH_HOME=$HOME/simulation/.ssh/${CONTAINER_WHO_AM_I}

    # The key is updated every time, we delete and then rebuild.
    if [ -e "$M_SSH_HOME" ];then
        rm -rf $M_SSH_HOME
    fi

    # Create a new ssh key to authenticate
    # Ensure that the authentication file has the correct permissions.
    mkdir -m 700 -p $M_SSH_HOME
    ssh-keygen -t rsa -b 2048 -N "" -f "$M_SSH_HOME/id_rsa" > /dev/null 2>&1
    touch $M_SSH_HOME/authorized_keys
    chmod 600 $M_SSH_HOME/authorized_keys

    # Register 'master' pub key to 'slave-x'
    if [ "$CONTAINER_WHO_AM_I" == "master" ]; then
        if [ -f "$HOME/simulation/.slavex" ]; then
            cat $HOME/simulation/.ssh/master/id_rsa.pub >> $HOME/simulation/.ssh/slave/authorized_keys
        fi
    fi

    # Redirect ssh key location, without 'umount' at the end, just 'exit' is fine.
    mkdir -m 700 -p $HOME/.ssh
    mount --bind $M_SSH_HOME $HOME/.ssh

    # Create some default groups
    groupadd system
    groupadd diag
    groupadd radio
    groupadd inet
    groupadd locclient
    groupadd ubi
    groupadd gps
    groupadd sensors

    # Create some default users
    useradd -m --shell /bin/bash tafcore
    useradd -M --no-log-init --shell /bin/bash telaf
    useradd -M --no-log-init --shell /bin/bash appdefault
    useradd -M --no-log-init --gid root --shell /bin/bash securityunpack

    usermod -aG root tafcore
    usermod -aG tafcore root

    # Add 'tafcore' to sudoer list
    echo 'tafcore:simula' | chpasswd
    echo "tafcore ALL=(ALL:ALL) ALL" >> /etc/sudoers
    echo 'tafcore ALL=(ALL:ALL) NOPASSWD: /usr/sbin/setcap' >> /etc/sudoers

    echo "/mnt/legato/system/lib" > /tmp/ld.so.conf

    # Remap 'reboot' to a special script, don't restart the container
    unlink /sbin/reboot
    echo "/usr/bin/killall serviceDirectory" > /tmp/simulation_reboot

    # When the framework starts, if the supervisor encounters a FATAL error,
    # we need to make sure that the docker container does not exit
    echo "/usr/bin/killall startSystem"     >> /tmp/simulation_reboot

    chmod a+x /tmp/simulation_reboot
    ln -s /tmp/simulation_reboot /sbin/reboot

    # Enable read-only mode in the container environment
    cp /etc/passwd /tmp/passwd
    cp /etc/group /tmp/group
    mount --bind -o ro /tmp/passwd /etc/passwd
    mount --bind -o ro /tmp/group  /etc/group

    # Change the hostname to a specific label: simulation
    # Also be used for syslog tag
    hostname simulation

    # Maping 'simulation' to localhost
    echo '127.0.1.1   simulation' >> /etc/hosts

    # Initialize the 'locale' for system
    echo 'LANG="en_US.UTF-8"' >> /etc/default/locale

    # Busybox syslogd on Ubuntu
    /sbin/syslogd -C20000

    if [ -f /usr/bin/logread ]; then
        ln -s /usr/bin/logread /sbin/logread
    else
        ln -s /bin/logread /sbin/logread
    fi

    mkdir -p /legato
    mkdir -p /tmp/legato_logs
    mkdir -p /tmp/legato

    mkdir -p /data/le_fs
    mkdir -p /data/persist
    mkdir -p /data/ManagedServices

    chmod 0777 /data/le_fs

    MOUNTPOINT_TELAF="/mnt/legato"
    mount -o bind $MOUNTPOINT_TELAF /legato

    # Update the runtime library pathes for telaf.
    echo "/legato/taf_rootfs/lib"   >> /etc/ld.so.conf.d/00-taf.conf
    echo "/legato/taf_rootfs/lib64" >> /etc/ld.so.conf.d/00-taf.conf

    sync && ldconfig

    # Use SIMULATION_TARBALL_NAME to ensure which image we could use
    SML_RO_TARBALL=$HOME/simulation/$SIMULATION_TARBALL_NAME

    if [ -f $SML_RO_TARBALL ]; then

        # extract the tarball to /mnt/legato without the 'install/' directory
        tar zxf $SML_RO_TARBALL --no-same-owner --overwrite -C $MOUNTPOINT_TELAF --exclude up_simulation.sh

        CONTAINER_DISTRO_VERSION=$(grep -oP 'VERSION_ID="\K[^"]+' /etc/os-release)

        if [ -e $MOUNTPOINT_TELAF/.check_done ] ; then

            FROM_VERSION_STR=`cat $MOUNTPOINT_TELAF/.check_done`

            if [ "$FROM_VERSION_STR" != "from 22.04" ]; then
                echo "Exit .check_done, but [$FROM_VERSION_STR], not 22.04, please rebuild your tarball."
                exit 1
            fi

            FROM_VERSION=`echo ${FROM_VERSION_STR} | cut -d ' ' -f 2`
            if [ "$FROM_VERSION" != "$CONTAINER_DISTRO_VERSION" ];then
                echo "Mismatch [${SIMULATION_TARBALL_NAME}] tarball !"
                echo "Build from: [ubuntu-${FROM_VERSION}]"
                echo "Deploy to : [ubuntu-${CONTAINER_DISTRO_VERSION}]"
                echo -e "\nPlease re-build your project in corresponding ubuntu system.\n"
                exit 1
            fi

            # Script that is only used for initialization once
            if [ -f $HOME/simulation/.simula.once.sh ]; then
                source $HOME/simulation/.simula.once.sh
            fi

            # For CGROUP V2, change-root-cgroup is needed
            if [ -e "${MOUNTPOINT_TELAF}/cg.version" ];then

                CG_VERSION=`cat ${MOUNTPOINT_TELAF}/cg.version`

                echo -e "cg.version is [${CG_VERSION}]\n"

                if [ "${CG_VERSION}" == "V2" ]; then
                    bash $HOME/simulation/relocate_cgroup_v2_root.sh
                    if [ $? -ne 0 ]; then
                        exit 1 # Exception ?
                    fi
                fi

            else
                echo -e "No cg.version to be recorded, [legacy]\n"
            fi

            # here we go, happy to simulate <..

        else
            echo "Tarball NOT contains .check_done, this is a bad ball, please rebuild right one."
            exit 1
        fi

    else
        echo "Not found $SML_RO_TARBALL, please get it first !"
        exit 0
    fi

    # Mark done
    touch /tmp/telaf_simulation_up
    change_PS1

    ldconfig

# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
else # [Non-Docker-Container-Env]
# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------

    trap : INT TERM

    if [ $# -ge 1 ]; then
        export CONTAINER_WHO_AM_I="$1"
    else
        export CONTAINER_WHO_AM_I="master"
    fi

    # INSTANCE tags a runtime container/volume set for multi-instance debugging.
    # Empty INSTANCE preserves legacy names. Allowed chars follow docker's rules.
    if [ -n "$INSTANCE" ]; then
        if ! echo "$INSTANCE" | grep -qE '^[a-zA-Z0-9][a-zA-Z0-9_.-]*$'; then
            echo "Invalid INSTANCE=$INSTANCE (allowed: [a-zA-Z0-9][a-zA-Z0-9_.-]*)"
            exit 1
        fi
    fi

    function try_to_create_volume () {
        if docker volume inspect $1 > /dev/null 2>&1; then
            echo "[Volume Reuse] --> $1"
        else
            echo "[Volume Create] --> $1"
            docker volume create $1 > /dev/null
        fi
    }

    SML_WORKSPACE=$SCRIPT_DIR

    IMG_NAME=${IMG_NAME:="telaf_simulation_runtime"}
    IMAGE_EXISTS=$(docker images --filter "reference=$IMG_NAME" | grep $IMG_NAME | wc -l)

    if [ ! $IMAGE_EXISTS -gt 0 ]; then
        echo "> Docker image '$IMG_NAME' NOT exists, please build or pull this image firstly !"
        exit 1
    fi

    if [ ! -f $SML_WORKSPACE/telaf_simulation.tar.gz ]; then
        echo "> Put 'up_simulation.sh' and 'telaf_simulation.tar.gz' to the SAME directory !"
        exit 1
    fi

    # The priority of variables is as follows:
    #  1. simulation.cfg (if exists)
    #  2. Environmental parameters, such as: 'CONTAINER_NAME=telaf_sml_alias ./up_simulation.sh'
    #  3. Default values in this script

    if [ -f $SML_WORKSPACE/simulation.cfg ]; then
        source $SML_WORKSPACE/simulation.cfg
    fi

    CONTAINER_NAME=${CONTAINER_NAME:="telaf_simulation_runtime"}
    IMG_NAME=${IMG_NAME:="telaf_simulation_runtime"}
    IMG_VERSION=${IMG_VERSION:="1.0.0"}
    # Network is shared across all instances of the same which-one — strip the INSTANCE
    # suffix (leading '_<tag>') from CONTAINER_NAME before deriving the network name so
    # that '..._m' and '..._m_test' both resolve to the same '..._ipv6net'.
    if [ -n "$INSTANCE" ]; then
        NETNAME_BASE="${CONTAINER_NAME%_$INSTANCE}"
    else
        NETNAME_BASE="$CONTAINER_NAME"
    fi
    IPV6_NETWORK_NAME=${IPV6_NETWORK_NAME:="${NETNAME_BASE%??}_ipv6net"}
    IPV6_DEFAULT_SUBNET=${IPV6_DEFAULT_SUBNET:="2001:0DB8::/112"}

    # SIMULA_MODE selects how the container is started:
    #   interactive : human shell, -t, --rm, /bin/bash as PID 1  (default, legacy behavior)
    #   headless    : long-lived container for AI/CI, detached, no TTY, no --rm, tail -f /dev/null
    # Users should not set TTY_OPT/RM_OPT/CONTAINER_CMD directly.
    SIMULA_MODE=${SIMULA_MODE:-interactive}
    case "$SIMULA_MODE" in
        interactive)
            SML_TTY_OPT="-t"
            SML_RM_OPT="--rm"
            SML_DETACH_OPT=""
            SML_CONTAINER_CMD="/bin/bash"
            ;;
        headless)
            SML_TTY_OPT=""
            SML_RM_OPT=""
            SML_DETACH_OPT="-d"
            SML_CONTAINER_CMD="tail -f /dev/null"
            ;;
        *)
            echo "Unknown SIMULA_MODE=$SIMULA_MODE (want: interactive|headless)"
            exit 1
            ;;
    esac

    BUILTIN_CONTAINER_OPTIONS=${BUILTIN_CONTAINER_OPTIONS:="-i $SML_TTY_OPT $SML_DETACH_OPT --privileged=true --net=$IPV6_NETWORK_NAME"}
    SSH_PORT=${SSH_PORT:-9022}
    CONTAINER_OPTIONS=${CONTAINER_OPTIONS:="-p ${SSH_PORT}:22 $SML_RM_OPT $EX_DOCKER_OPTS"}
    SIMULATION_TARBALL_NAME=${SIMULATION_TARBALL_NAME:="telaf_simulation.tar.gz"}

    # Get recorded CGROUP version
    CG_VERSION=`tar -xOzf $SML_WORKSPACE/telaf_simulation.tar.gz cg.version`
    if [ $? -ne 0 ]; then
        echo "cg.version does NOT exist in telaf_simulation.tar.gz, mark as CGROUP VERSION as V1"
        CG_VERSION=V1
    else
        echo "cg.version is [${CG_VERSION}]"
    fi

    # Get current target machine CGROUP version
    if [ -d "/sys/fs/cgroup/freezer" ]; then
        MACHINE_CG_VERSION=V1
    elif [ -e "/sys/fs/cgroup/cgroup.controllers"  ]; then
        MACHINE_CG_VERSION=V2
    else
        echo "CGROUP on target machine is not identified"
        exit 1
    fi

    # Check CGROUP version
    if [ "${CG_VERSION}" != "${MACHINE_CG_VERSION}" ]; then
        echo
        echo "cg.version in tar.ball  : [${CG_VERSION}]"
        echo "current machine version : [${MACHINE_CG_VERSION}]"
        echo "[Mismatched], please rebuild simulation within proper 'ENABLE_CGROUP_V2' option"
        exit 1
    fi

    if [ "${MACHINE_CG_VERSION}" == "V1" ]; then
        CGROUP_OPTIONS="--cgroupns=private"

    else # MACHINE_CG_VERSION == V2
        # For CGROUP V2, use 'user.slice' to be root cgroup,
        # then systemd will manage the different groups according to
        # different scopes.
        # Check with: ' systemd-cgls -u user.slice '
        # Example:
        # Unit user.slice (/user.slice):
        # ├─docker-3cab3d4597b48c8ffda9f36ba2a91f95c8d784808a1f3028ed3e7989f3c7831c.scope
        # │ └─_cgroot__
        # │   ├─787361 /bin/bash
        # │   ├─787549 sshd: /usr/sbin/sshd [listener] 0 of 10-100 startups
        # │   └─787669 /sbin/syslogd -C20000
        CGROUP_OPTIONS="--cgroupns=private --cgroup-parent=user.slice"
    fi

    SML_APP_VOLUME=${CONTAINER_NAME}_sml_app
    SML_DATA_VOLUME=${CONTAINER_NAME}_sml_data
    SML_PERSIST_VOLUME=${CONTAINER_NAME}_sml_persist
    SML_MNT_LEGATO_VOLUME=${CONTAINER_NAME}_sml_mnt_legato
    SML_SSH_INFO_VOLUME=telaf_simulation_common_sml_ssh_info

    echo "[Prepare] Create or Reuse docker volumes for persistently data"
    try_to_create_volume $SML_APP_VOLUME
    try_to_create_volume $SML_DATA_VOLUME
    try_to_create_volume $SML_PERSIST_VOLUME
    try_to_create_volume $SML_MNT_LEGATO_VOLUME
    try_to_create_volume $SML_SSH_INFO_VOLUME

    networks=$(docker network ls --format "{{.Name}}")

    if echo "$networks" | grep -w "$IPV6_NETWORK_NAME" &>/dev/null; then
        echo "[Network Reuse] --> $IPV6_NETWORK_NAME"
    else
        echo "[Network Create] --> $IPV6_NETWORK_NAME"
        docker network create --driver bridge --ipv6 --subnet "$IPV6_DEFAULT_SUBNET" "$IPV6_NETWORK_NAME" > /dev/null
        if [ $? -ne 0 ]; then
            echo "There is a conflicting network [address: $IPV6_DEFAULT_SUBNET is reused], check docker network and delete that for continuing"
            exit 1
        fi
    fi

    CMD="docker run --name $CONTAINER_NAME \
        $BUILTIN_CONTAINER_OPTIONS \
        $CONTAINER_OPTIONS \
        $CGROUP_OPTIONS \
        $EX_DOCKER_OPTS \
        -e CONTAINER_WHO_AM_I=$CONTAINER_WHO_AM_I \
        -e CONTAINER_NAME=$CONTAINER_NAME \
        -e SIMULATION_TARBALL_NAME=$SIMULATION_TARBALL_NAME \
        -v $SML_WORKSPACE:/root/simulation:rw \
        -v $SML_WORKSPACE/../sml:/root/sml:rw \
        -v $SML_APP_VOLUME:/app:rw \
        -v $SML_DATA_VOLUME:/data:rw \
        -v $SML_PERSIST_VOLUME:/persist:rw \
        -v $SML_MNT_LEGATO_VOLUME:/mnt/legato:rw \
        -v $SML_SSH_INFO_VOLUME:/root/simulation/.ssh:rw \
        $IMG_NAME:$IMG_VERSION $SML_CONTAINER_CMD"
    echo
    echo "> "$CMD
    eval $CMD

    if [ $? -ne 0 ]; then
        exit $?
    fi

    # mark slave-x to be started, also store the IP address related.
    if [ "$CONTAINER_WHO_AM_I" == "slave" ]; then
        CONTAINER_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$CONTAINER_NAME")
        echo -e "\n> Slave daemon [${CONTAINER_NAME}] is started @ [$CONTAINER_IP].\n"
        echo "$CONTAINER_NAME $CONTAINER_IP" >> $SML_WORKSPACE/.slavex
        echo "$CONTAINER_NAME $CONTAINER_IP" > $SML_WORKSPACE/.simula.slave
    fi

    # master record the container names & IP addresses from slave-x.
    if [ "$CONTAINER_WHO_AM_I" == "master" ]; then
        if [ -f $SML_WORKSPACE/.slavex ]; then

            # Ensure the .slavex was deleted after stop-actions
            cp -af $SML_WORKSPACE/.slavex /tmp/.slavex
            rm -f $SML_WORKSPACE/.slavex

            while IFS=' ' read -r cname ipaddr;
            do
                echo "> Stop [$cname] partner @ [$ipaddr] ..."
                docker stop $cname > /dev/null 2>&1
                echo "> Stop [$cname] partner @ [$ipaddr] done."
            done < /tmp/.slavex

            rm -f /tmp/.slavex $SML_WORKSPACE/.simula.slave $SML_WORKSPACE/.simula.master
            exit $?

        else
            # no partner ? ok, exit directly.
            exit 0
        fi
    elif [ "$CONTAINER_WHO_AM_I" == "slave" ]; then
        exit $?
    else # standalone ?
        exit 0
    fi
fi

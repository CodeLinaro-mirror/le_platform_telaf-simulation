# --------------------------------------------------------------------------------------------------
# Makefile for TelAF Simulation Target
# --------------------------------------------------------------------------------------------------

TARGETS += simulation menuconfig_simulation

# For outputing the simulation related details, along with 'Q=' in make command
# BTW, for all details for TelAF & Legato, along with 'Q= V=1' in make command
Q ?=@

-include $(SIMULATION_ROOT)/.simulation.build

# Sometimes, due to docker's caching, it can lead docker image rebuilding failure.
# So we need to add some options for building images, such as "--no-cache"
docker_build_opts ?=

# Some work needs to be done earlier or later, so we prevent the real simulation goal.
ifneq ($(filter simulation,$(MAKECMDGOALS)),)
  $(error Please pass 'make simula-help' for TelAF Simulation Target [simulation])
endif

# Override the LEGATO_VERSION, to ensure the version is same as other targets.
export LEGATO_VERSION=$(shell cat $(TELAF_ROOT)/VERSION 2> /dev/null)

# For embedded target, the cross-compilation tool will change the 'sysroot'
# to search header & libraries that have beed relocated.
# when we get the path by '--print-sysroot', that value will be returned.
# But for simulation target, the default prefix (empty) is used,
# because the host default environment for the gcc compiler is that.
# So for consistency, we add the required path without any affect for mktools.
export SYSROOT=/

export SIMULATION_HOME := $(SIMULATION_ROOT)
export SIMULATION_SCRIPTS := $(SIMULATION_HOME)/scripts
export SIMULATION_WORKDIR := $(SIMULATION_HOME)/workstation
SIMULATION_TARBALL := $(SIMULATION_HOME)/workstation/telaf_simulation.tar

# Add PA default and stub searching path for CDEF files
export TELAF_DEFAULT_PA_LIB_DIR=${SIMULATION_HOME}/pa/telaf-pa-default/staging

# Re-export the variable for subsequent scripts in legato-af project
export SELINUX_FILE_CONTEXTS := /no/selinux/feature/for/simulation

# Get the distro version of current system, refer to: /etc/os-release
OS_VERSION=$(shell grep -oP 'VERSION_ID=\K"(.+)"' /etc/os-release | tr -d '"')

# Sub-Makefile to handle all target dependencies and extended host tools
include $(SIMULATION_HOME)/deps/dependence.mk

SIMULATION_DEPS += _openssl _curl _boost
SIMULATION_DEPS_ALL = _pre_deps $(SIMULATION_DEPS) _post_deps

# [1] Another way: (not in container)
#   1. mkdir $(SIMULATION_WORKDIR)/sdk_rootfs
#   2. sudo mount --bind /path/to/sdk_rootfs  $(SIMULATION_WORKDIR)/sdk_rootfs
# [2] In container:
#   1. Before you start the container, please put the 'sdk' project into the same level as the 'telaf' project:
#   - top_level_dir
#    |- sdk/
#    |- telaf/
#   2. specify the sdk_root=/path/to/the/sdk/rootfs
export sdk_rootfs ?= $(SIMULATION_WORKDIR)/sdk_rootfs
CHECK_SDK_ROOTFS := $(shell if [ -d "$(sdk_rootfs)" ]; then echo "y"; else echo "n"; fi)

export IMPORT_SDK_SIMULATION ?= n

ifneq ($(IMPORT_SDK_SIMULATION),n)

ifeq ($(CHECK_SDK_ROOTFS),n)
  $(error sdk rootfs path is invalid [$(sdk_rootfs)], please check it)
else
  # $(warning sdk rootfs path [$(sdk_rootfs)])
endif

MKTOOLS_FLAGS_SIMULATION_EX += --cxxflags=-I$(sdk_rootfs)/include \
                               --ldflags=-L$(sdk_rootfs)/lib

export TELAF_SIMULATION_ENABLE_SMS ?= n
export TELAF_SIMULATION_ENABLE_DCS ?= n
export TELAF_SIMULATION_ENABLE_SIM ?= n
export TELAF_SIMULATION_ENABLE_LOC ?= n
export TELAF_SIMULATION_ENABLE_RADIO ?= n

export TELAF_SIMULATION_ENABLE_MNGD_CONN ?= n
ifneq ($(TELAF_SIMULATION_ENABLE_MNGD_CONN),n)
  TELAF_SIMULATION_ENABLE_DCS := y
  TELAF_SIMULATION_ENABLE_SIM := y
  TELAF_SIMULATION_ENABLE_RADIO := y
endif

export TELAF_SIMULATION_ENABLE_ECALL ?= n
ifneq ($(TELAF_SIMULATION_ENABLE_ECALL),n)
  TELAF_SIMULATION_ENABLE_RADIO := y
  TELAF_SIMULATION_ENABLE_SIM := y
  TELAF_SIMULATION_ENABLE_LOC := y
endif

endif # (IMPORT_SDK_SIMULATION)

export TELAF_SIMULATION_ENABLE_SOMEIP_GW ?= n
export TELAF_SIMULATION_ENABLE_DIAG ?= n
export TELAF_SIMULATION_ENABLE_CAPI ?= n
export TELAF_SIMULATION_ENABLE_MNGD_STRG ?= n

export TELAF_SIMULATION_ENABLE_RPC ?= n
ifneq ($(TELAF_SIMULATION_ENABLE_RPC),n)
  TELAF_SIMULATION_ENABLE_SOMEIP_GW := y
endif


SIMULATION_SOMEIP_GW_DEPS_y := _vsomeip
SIMULATION_COMMON_API_DEPS_y := _capi_core_rt _capi_someip_rt _capi_tools
SIMULATION_DEPS += \
  $(SIMULATION_SOMEIP_GW_DEPS_$(TELAF_SIMULATION_ENABLE_SOMEIP_GW)) \
  $(SIMULATION_COMMON_API_DEPS_$(TELAF_SIMULATION_ENABLE_CAPI))

# Common options for C++
MKTOOLS_FLAGS_SIMULATION_EX += -X -std=c++17 -X -lstdc++

# Suppress the boost warning output
MKTOOLS_FLAGS_SIMULATION_EX += -X -DBOOST_BIND_GLOBAL_PLACEHOLDERS

# Exported 3rd party dependencies
MKTOOLS_FLAGS_SIMULATION_EX += \
  --cxxflags=-I$(SIMULATION_DEPS_ROOTFS)/include \
  --ldflags=-L$(SIMULATION_DEPS_ROOTFS)/lib

export MKTOOLS_FLAGS_SIMULATION_EX

# Command line can override the option of CGROUP feature
export ENABLE_CGROUP_V2 ?= y

# Check and record to tar ball within file cg.version
ifeq ($(ENABLE_CGROUP_V2),y)
  $(shell echo -n "V2" > $(SIMULATION_HOME)/workstation/cg.version)
else
  $(shell echo -n "V1" > $(SIMULATION_HOME)/workstation/cg.version)
endif

.PHONY: simulation

ifeq ($(XDEBUG),on)
  export DEBUG=1
  export STRIP_STAGING_TREE=0
  XDEBUG_ACTION:=simula-clean-config
endif

simula simulac simula-c: $(XDEBUG_ACTION) check-sys pre-simulation-build simulation post-simulation-build
simulax simula-x simulacx simula-cx: clean_check_done_flag simula-up-develop-for-c

clean_check_done_flag:
	$Q echo "In container for compilation, we are always check the system dependencies."
	$Q rm -f $(SIMULATION_HOME)/workstation/.check_done

check-sys:
	$Q echo "TelAF Simulation pre-checking your system ..."
	$Q if [ -e $(SIMULATION_WORKDIR)/.check_done ] && [ "`umask`" = "0022" ]; then \
		echo "Current system is already checked [OK]"; \
	else \
		if /bin/bash $(SIMULATION_SCRIPTS)/check_sys.sh ; then \
			echo "Current system is [OK]"; \
			echo -n "from $(OS_VERSION)" > $(SIMULATION_WORKDIR)/.check_done; \
		else \
			if [ -e $(SIMULATION_WORKDIR)/.check_done ]; then rm $(SIMULATION_WORKDIR)/.check_done; fi ; \
			echo "Current system is [NOK], please check above log for details."; exit 1 ; \
		fi \
	fi


pre-simulation-build: $(SIMULATION_HOME)/workstation/up_simulation.sh $(SIMULATION_DEPS_ALL)
	$Q echo "[Simulation]: Building PA layers ..."
	$Q bash $(SIMULATION_ROOT)/pa/simulation_build_pa.sh
	$Q echo "[Simulation]: PA layers finished"
post-simulation-build: CURRENT_SYSTEM_OUTPUT=$(TELAF_BUILD)/simulation/_staging_system.simulation.update_ro/systems/current
post-simulation-build:
	$Q echo "[Simulation]: Installing PA libraries ..."
	$Q bash $(SIMULATION_ROOT)/pa/simulation_install_pa.sh
	$Q echo "[Simulation]: Creating Tarball ..."
	$Q tar cf $(SIMULATION_TARBALL) -C $(TELAF_BUILD)/simulation/staging_combined . # After introduced PA
	$Q tar rf $(SIMULATION_TARBALL) -C $(SIMULATION_HOME)/workstation/ up_simulation.sh
	$Q tar rf $(SIMULATION_TARBALL) -C $(SIMULATION_HOME)/workstation/ .check_done
	$Q tar rf $(SIMULATION_TARBALL) -C $(SIMULATION_HOME)/workstation/ cg.version
ifneq ($(TELAF_SIMULATION_ENABLE_MNGD_CONN),n)
	$Q cp $(TELAF_ROOT)/apps/tafMngdServices/Connectivity/Components/tafMngdConnSvc/Config/mngdConnectivity.json $(SIMULATION_HOME)/deps/taf_rootfs
endif
	$Q tar rf $(SIMULATION_TARBALL) --exclude=taf_rootfs/include \
	                                --exclude=taf_rootfs/lib/cmake \
	                                --exclude=taf_rootfs/lib/pkgconfig \
	                                -C $(SIMULATION_HOME)/deps taf_rootfs
ifneq ($(CHECK_SDK_ROOTFS),n)
	$Q tar rf $(SIMULATION_TARBALL) --transform 's/rootfs/sdk_rootfs/' -C $(sdk_rootfs)/../ rootfs
endif
	$Q gzip -f $(SIMULATION_TARBALL)
	$Q echo "[Simulation]: Tarball $(SIMULATION_TARBALL).gz done."


simula-menuconfig: menuconfig_simulation

which_one_default := $(SIMULATION_HOME)/which_one_default
which_one := $(SIMULATION_WORKDIR)/.which_one
get_which_one := `if [ -e $(which_one) ]; then cat $(which_one) ; else cat $(which_one_default) ; fi`
which_one_point_version :=  $(shell echo $(get_which_one) | sed 's/\([0-9][0-9]\)/\1./')

# If you want to specify a private hub address to get ubuntu base images, override 'from' in commands
export from ?=

ifneq ("$(origin from)","command line")
  export from := ubuntu:$(which_one_point_version)
else
  # Check if the current version matches the version passed in, if not, please 'make simula-distro-<version>'
  FROM_TYPICAL_VERSION=$(shell echo $(from) | sed 's/.*:\(.*\)/\1/')
  ifneq ("$(FROM_TYPICAL_VERSION)","$(which_one_point_version)")
    $(error "Mismatched version, current wanted [$(which_one_point_version)], but got [$(FROM_TYPICAL_VERSION)].\
	         Please check 'make simula-list' to ensure the version to be supported.")
  endif

  # NOTE: in addition to the officially supported versions, you can create your own, but we will not give technical support
  # Step 1. Create directory & Dockerfiles: <version> -> 2204, <dot-version> -> 22.04
  #      1.1 simulation/docker/for_ubuntu_<version>
  #      1.2 simulation/docker/for_ubuntu_<version>/docker-compose.yml
  #      1.2.2 Add the docker-compose.yml with correct Dockerfile names
  #      1.3 simulation/docker/for_ubuntu_<version>/Dockerfile.<version>.runtime
  #      1.4 simulation/docker/for_ubuntu_<version>/Dockerfile.<version>.develop
  # Step 2. Change the 'simulation/workstation/.which_one' to which <version> you want
  # Step 3. Rebuild all images: make simula-buildall from=ubuntu:<dot-version>

endif

define up_simulation_container
	$Q echo "Up Simulation with [$(1:up_%.sh=%)]"
	$Q /bin/bash $(SIMULATION_SCRIPTS)/$(1) $(get_which_one) $(2)
	$Q echo "Down Simulation with [$(1:up_%.sh=%)], see you ~"
endef

# Configure the UID & GID during docker build image times along with environment variables
# Rebuild all docker images for special UID:GID. please check: DEVELOPER_UID & DEVELOPER_GID
define build_simulation_docker_image
	$Q echo "[$@] build docker image..."
	$Q export UBUNTU_DISTRO_ORIGIN=$(from) \
	    && export DEVELOPER_UID=$(shell id -u) \
	    && export DEVELOPER_GID=$(shell id -g) \
	    && docker compose -f "$(SIMULATION_HOME)/docker/for_ubuntu_$(get_which_one)/docker-compose.yml" \
	    build $(docker_build_opts) telaf_simulation_$(1)_$(get_which_one)
	$Q echo "[$@] image build done."
endef

simula-st simula-subsys-tests: $(LEGATO_ROOT)/build/simulation
	$Q $(MAKE) -C $(LEGATO_ROOT) subsys_tests TARGET=simulation TELAF_ROOT_SET=$(TELAF_ROOT)

simula-tests_c:
	$Q $(MAKE) -C $(LEGATO_ROOT) tests_c TARGET=simulation TELAF_ROOT_SET=$(TELAF_ROOT)
	$Q ln -sf $(LEGATO_RELATIVE_PATH)/build ./build

simula-help:
	@echo "Help Page for TelAF Simulation CLI"
	@echo
	@echo "> make simula-action [simula-action ...] [simula-action-args]"
	@echo
	@echo "  Examples:"
	@echo "    > make simula-list"
	@echo "    > make simula-build-runtime from=ubuntu:20.04"
	@echo
	@echo "  >> simula-action-args"
	@echo "    - from='hub-address'"
	@echo "    - sdk_rootfs='/absolute/path/to/sdk/rootfs'"
	@echo
	@echo "  >> simula-action supported list as follows"
	@echo "    - List & Switch simulation container distro system versions (default Ubuntu18.04)"
	@echo "      + simula-list                    -- List all system distro versions simulation supported."
	@echo
	@echo "    - Compile your simulation project on your HOST or CONTAINER"
	@echo "      + simula | simulac  | simula-c   -- Incrementally compile simulation open source code on HOST"
	@echo "                                       -- (workstation/.simulation.build) if needed, create and add Makefile variables"
	@echo "      + simulax| simulacx | simula-cx  -- Incrementally compile simulation open source code in CONTAINER"
	@echo "                                       -- (workstation/.simula.dev.action.sh) if needed, create and add shell commands"
	@echo "      + simula-clean                   -- Just clean your simulation project & tools"
	@echo "      + simula-distclean               -- Deep clean all telaf project stuff"
	@echo
	@echo "    - Build your simulation docker containers cli, depends which system version you selected (see 'simula-list')"
	@echo "      + simula-build-runtime           -- Build a runtime docker image for running TelAF Simulation"
	@echo "      + simula-build-develop           -- Build a develop docker image for developing Simulation in it"
	@echo "      + simula-build-all               -- Build all docker images including [runtime, develop]"
	@echo "      + simula-build-runtime from='hub-address'"
	@echo "                                       -- Specify a hub address you want to get the ubuntu base image and build it"
	@echo
	@echo "    - Boot up your simulation docker container that was built, depends which system version you selected (see 'simula-list')"
	@echo "      + simula-up | simula-up-runtime  -- Boot up the runtime container to simulate"
	@echo "      + simula-upx| simula-upx-runtime -- Boot up multi-runtime-containers to simulate"
	@echo "      + simula-up-develop              -- Boot up the develop container for developers"
	@echo
	@echo "    - Docker operation helper commands"
	@echo "      + simula-listimg                 -- List all docker images on your host"
	@echo "      + simula-listv                   -- List all volumes named along with 'telaf'"
	@echo "      + simula-rmv                     -- Delete all volumes named along with 'telaf'"
	@echo "      + simula-rm-app                  -- Delete the app volume"


simula-buildall simula-build-all simula-build-all-docker-images: simula-build-runtime simula-build-develop

simula-build simula-build-runtime:
	$(call build_simulation_docker_image,runtime)

simula-build-develop:
	$(call build_simulation_docker_image,develop)

simula-up simula-up-runtime:
	$(call up_simulation_container,up_runtime_master.sh)

# slave-x containers are daemons, start first.
simula-upx simula-upx-runtime:
	$(call up_simulation_container,up_runtime_slavex.sh)
	$(call up_simulation_container,up_runtime_master.sh)

simula-up-develop:
	$Q /bin/bash $(SIMULATION_SCRIPTS)/up_develop.sh -i $(get_which_one)

simula-up-develop-for-c:
	$Q /bin/bash $(SIMULATION_SCRIPTS)/up_develop.sh -v -m $(get_which_one)
	$Q echo "Compiled from --> [$@] [dversion: $(get_which_one)]"

simula-list simula-list-distro:
	$Q echo "TelAF Simulation support list:"
	$Q echo -n "  [1] ubuntu22.04"; if [ "$(get_which_one)" = "2204" ]; then echo " <--" ; else echo ; fi
	$Q if [ "$(get_which_one)" != "2204" ]; then \
	      echo -n "  [x] ubuntu$(which_one_point_version)"; echo " <-- (unsupported version)" ; fi

simula-distro-2204:
	$Q echo -n "2204" > $(which_one)
	$Q echo "Switch container OS version to --> 22.04"
	$Q echo
	$Q $(MAKE) --no-print-directory simula-list

simula-listimg simula-list-all-docker-images:
	$Q echo "[$@] list all simulation images"
	$Q docker images

simula-rm-dangling simula-remove-docker-dangling-images:
	$Q echo "[$@] Removing dangling images..."
	$Q docker images -f "dangling=true" -q | xargs -r docker rmi
	$Q echo "[$@] remove dangling images done."

simula-listv simula-list-simulation-volumes:
	$Q echo "[$@] docker just for simulation volumes as the list"
	$Q docker volume ls -qf "name=telaf"

simula-listallv simula-list-all-volumes:
	$Q echo "[$@] docker all volumes as the list"
	$Q docker volume ls

simula-rmv simula-remove-simulation-volumes:
	$Q echo "[$@] detele all telaf simulation volumes ..."
	$Q docker volume ls -qf "name=telaf" | xargs -r docker volume rm
	$Q echo "[$@] detele all telaf simulation volumes done."

simula-rm-app simula-remove-simulation-app-volume:
	$Q docker volume ls --format '{{.Name}}' | grep -E '^telaf.*app$$' | xargs -r docker volume rm
	$Q echo "[$@] detele app volume done."

simula-remove-all-volumes:
	$Q echo "[$@] detele all volumes ..."
	$Q volumes=$$(docker volume ls -q) && { for volume in $$volumes ; do docker volume rm $$volume ; done }
	$Q echo "[$@] detele all volumes done."

simula-clean: simula-clean-config
	$Q rm -rf $(TELAF_ROOT)/build/simulation
	$Q rm -rf $(TELAF_ROOT)/build/tools
	$Q rm -rf $(SIMULATION_WORKDIR)/telaf_simulation.tar.gz

simula-distclean: distclean
	$Q rm -rf $(SIMULATION_WORKDIR)/telaf_simulation.tar.gz

simula-show-deps-path:
	$Q realpath $(SIMULATION_HOME)/deps/

simula-reuse-deps:
	$Q if [ -n "$(SIMULATION_REUSE_DEPS)" ]; then \
	     if [ -e "$(SIMULATION_REUSE_DEPS)" ]; then \
	       echo "Copy deps from $(SIMULATION_REUSE_DEPS) to $(SIMULATION_HOME)/deps/ ..." ; \
	       cp -ar $(SIMULATION_REUSE_DEPS)/.deps.origin $(SIMULATION_HOME)/deps/ ; \
	       cp -ar $(SIMULATION_REUSE_DEPS)/host_xtools $(SIMULATION_HOME)/deps/ ; \
	       cp -ar $(SIMULATION_REUSE_DEPS)/taf_rootfs $(SIMULATION_HOME)/deps/ ; \
	       echo "Ready to reuse deps in your current project."; \
	     else \
	       echo "Invalid [SIMULATION_REUSE_DEPS=$(SIMULATION_REUSE_DEPS)] that you set, please check." ; \
	     fi \
	   else \
	     echo "Please set [SIMULATION_REUSE_DEPS=/path/to/another/telaf/simulatoin/deps] to reuse simulation deps."; \
	   fi

simula-clean-deps:
	$Q rm -f $(SIMULATION_HOME)/deps/.deps.origin
	$Q echo "Remove $(SIMULATION_HOME)/deps/.deps.origin [Done]"

simula-clean-config:
	$Q rm -f $(LEGATO_ROOT)/.config.simulation

simula-clean-system:
	$Q rm -rf $(TELAF_ROOT)/build/simulation/{_staging_system.simulation.update,system}

simula-rm-network:
	$Q docker network ls -q --filter="name=telaf_simulation_runtime" | xargs -r docker network rm

simula-build-config simula-bc:
	$Q cat $(TELAF_ROOT)/simulation/.simulation.build

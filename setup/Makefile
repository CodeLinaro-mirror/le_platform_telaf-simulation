# ---------------------------------
# --- Simulaiton Setup Makefile ---
# ---------------------------------

# --- Usage ---
help usage:
	@echo "Usage: "
	@echo "    list | list-info    : List all git repo information"
	@echo "    update-<repo-name>  : Update the special git repo based on patch_me.json"
	@echo "    patch-<repo-name>   : Patch to the special git repo based on patch_me.json"
	@echo "    update-all          : Update all git repos"
	@echo "    patch-all           : Patch all git repos"
	@echo "    build-docker-2204   : Build ubuntu 22.04 docker image"
	@echo "    to-2204             : Change current distro pointer to ubunutu 22.04"
	@echo "    setup-sdk           : Setup the basic dev-env for SDK in container"
	@echo "    build-sdk           : Build SDK project in container"
	@echo "    clean-sdk           : Clean SDK project output"
	@echo "    setup-telaf         : Check the precondtion for building telaf"
	@echo "    build-telaf         : Build telaf project in container"
	@echo "    clean-telaf         : Clean telaf project output"

# --- Show Information ---

list list-info:
	@echo "All projects information:"
	@cat ./FROM | xargs echo "FROM:"
	@echo "telaf:" && cd telaf && git log --oneline -3
	@echo "legato:" && cd legato/legato-af && git log --oneline -3
	@echo "sdk:" && cd sdk && git log --oneline -3
	@echo "jansson:" && cd legato/3rdParty/jansson && git log --oneline -1
	@echo "Kconfiglib:" && cd legato/3rdParty/Kconfiglib && git log --oneline -1
	@echo "telaf-pa:" && cd telaf-pa && git log --oneline -1
	@echo "telaf-pa-default:" && cd telaf-pa-default && git log --oneline -1

# --- Updating ---

update-all: update-telaf update-legato update-sdk update-jansson update-kconfiglib update-telaf-pa update-telaf-pa-default

PY = python3
PYTHON_OK := $(shell $(PY) -c 'import sys; print(1) if sys.version_info >= (3, 6) else print(0)')

check-python:
	@ if [ $(PYTHON_OK) -ne 1 ]; then echo "Python version should be (>= 3.6)"; exit 1; fi

update-telaf: check-python telaf/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --update telaf

update-legato: legato/legato-af/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --update legato/legato-af

update-sdk: sdk/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --update sdk

update-jansson: legato/3rdParty/jansson/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --update legato/3rdParty/jansson

update-kconfiglib: legato/3rdParty/Kconfiglib/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --update legato/3rdParty/Kconfiglib

update-telaf-pa: telaf-pa/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --update telaf-pa

update-telaf-pa-default: telaf-pa-default/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --update telaf-pa-default

# --- Patching ---

patch-all: patch-telaf patch-legato patch-sdk patch-jansson patch-kconfiglib patch-telaf-pa patch-telaf-pa-default

patch-telaf: telaf/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --patch telaf

patch-legato: legato/legato-af/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --patch legato/legato-af

patch-sdk: sdk/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --patch sdk

patch-jansson: legato/3rdParty/jansson/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --patch legato/3rdParty/jansson

patch-kconfiglib: legato/3rdParty/Kconfiglib/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --patch legato/3rdParty/Kconfiglib

patch-telaf-pa: telaf-pa/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --patch telaf-pa

patch-telaf-pa-default: telaf-pa-default/.git
	@echo "[$@] <--"
	@python3 helpme_setup.py --patch telaf-pa-default


DOCKER_HUB ?=

# --- Docker images ---

precheck-docker:
	@if ! command -v docker &> /dev/null; then echo "Please install docker dev-env first"; exit 1; fi
	@if ! docker info &> /dev/null; then echo "[docker info] failed, please check"; exit 1; fi

build-docker-2204: precheck-docker telaf/simulation/docker/for_ubuntu_2204/docker-compose.yml
	@make simula-distro-2204 -C telaf --no-print-directory
	make simula-buildall from=$(DOCKER_HUB)ubuntu:22.04 docker_build_opts="--no-cache" -C telaf


to-2204: telaf/.git
	@make simula-distro-2204 -C telaf --no-print-directory


# --- SDK ---

setup-sdk: sdk/.git telaf/.git
	@echo "cd ../sdk && ./build_sim.sh setup" > telaf/simulation/workstation/.simula.dev.action.sh
	@echo "" > telaf/simulation/.simulation.build
	make simula-x -C telaf

build-sdk: sdk/.git telaf/.git
	@echo "cd ../sdk && ./build_sim.sh rootfs" > telaf/simulation/workstation/.simula.dev.action.sh
	@echo "" > telaf/simulation/.simulation.build
	make simula-x -C telaf

# Perform the actions in HOST is better
clean-sdk: sdk/.git
	cd sdk && rm -rf rootfs && ./build_sim.sh clean

# --- TelAF ---

check-telaf-precondition setup-telaf: precheck-docker sdk/rootfs
	@:

build-telaf: precheck-docker telaf/.git
	@echo "make simula-c" > telaf/simulation/workstation/.simula.dev.action.sh
	@touch telaf/simulation/.simulation.build
	make simula-x -C telaf

clean-telaf:
	@make simula-clean -C telaf --no-print-directory

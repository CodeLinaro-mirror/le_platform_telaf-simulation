
# Here we handle all the host tool dependencies, as well as the dependencies inside the container

# Stores the source code and zip for all dependent projects
export SIMULATION_DEPS_SOURCE := $(SIMULATION_HOME)/deps/source

# Dependencies to install in docker containers
export SIMULATION_DEPS_ROOTFS := $(SIMULATION_HOME)/deps/taf_rootfs

# Required extend tools to be installed in dev HOST
export SIMULATION_HOST_XTOOLS := $(SIMULATION_HOME)/deps/host_xtools

# This file with a valid format of xx.xx or empty are used to check compatibility
deps_origin=$(SIMULATION_HOME)/deps/.deps.origin

precheck_deps := \
  if [ -e "$(deps_origin)" ]; then \
    ORIG_VERSION=$$(cat $(deps_origin)) ; \
    if echo "$$ORIG_VERSION" | grep -qE '^[0-9][0-9]\.[0-9][0-9]$$'; then \
      if [ "$(OS_VERSION)" = "$$ORIG_VERSION" ]; then \
        echo "Prechecking dependencies is consistent [OK]" ; \
      else \
        echo ; \
        echo "Prechecking dependencies is [NOK]" ; \
        echo "Origin from: $$ORIG_VERSION" ; \
        echo "Current ver: $(OS_VERSION)" ; \
        echo ; \
        echo "Ignore this warning ..." ; \
		echo "If you want to be consistent, delete [$(deps_origin)]" and rebuild all; \
        echo ; \
      fi ; \
    else \
      echo "Invalid context in [$(deps_origin)], continue ... " ; \
    fi ; \
  else \
    echo "Not found file [$(deps_origin)]" ; \
    echo "Touch it and clean all 3rd party deps, rebuilding all ... " ; \
    touch $(deps_origin) && rm -rf $(SIMULATION_DEPS_ROOTFS)/* $(SIMULATION_HOST_XTOOLS)/* ; \
  fi

mark_deps_origin := \
  if ! [ -s "$(deps_origin)" ]; then \
    echo -n "$(OS_VERSION)" > $(deps_origin) ; \
    echo "Mark 3rd party dependencies from [$(OS_VERSION)]" ; \
  fi

# All pre actions for handling dependencies.
# Check the existed 3rd party dependencies if fulfill current system
_pre_deps:
	$Q $(precheck_deps)

# All post actions for handling dependencies.
# Mark the 3rd party dependencies that are from which ubuntu distro version
# to be used by the precondition checking steps
_post_deps:
	$Q $(mark_deps_origin)

# -- The steps for all dependencies --
#-> [clean]    1. Clean original compression files and directories (every time)
#-> [download] 2. Download & extract the compression files
#-> [compile]  3. Compile the dependent project
#-> [install]  4. Install all generated stuff to SIMULATION_DEPS_ROOTFS or SIMULATION_HOST_XTOOLS

.PHONY: _boost simula-boost boost_

# Deprecated link: https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz
BOOST_URL?=https://archives.boost.io/release/1.78.0/source/boost_1_78_0.tar.gz
_BOOST_VERSION=$(notdir $(lastword $(subst /, ,$(BOOST_URL))))
BOOST_VERSION=$(_BOOST_VERSION:%.tar.gz=%)

# $(error $(shell ls -l $(SIMULATION_DEPS_ROOTFS)/include/boost/version.hpp))

_boost: $(SIMULATION_DEPS_ROOTFS)/include/boost/version.hpp
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/boost/version.hpp:
	@$(MAKE) --no-print-directory simula-boost

simula-boost: boost_
boost_:
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$(_BOOST_VERSION) $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_$(_BOOST_VERSION).log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(BOOST_URL)]" \
	  && wget -O $(SIMULATION_DEPS_SOURCE)/$(_BOOST_VERSION) $(BOOST_URL) > $(SIMULATION_DEPS_SOURCE)/__download_$(_BOOST_VERSION).log 2>&1
	$Q echo "[$@] extract to [$(SIMULATION_DEPS_SOURCE)/$@]" \
	  && cd $(SIMULATION_DEPS_SOURCE) \
	  && tar xfz $(_BOOST_VERSION) && mv $(BOOST_VERSION) $@
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	  && ./bootstrap.sh --prefix=$(SIMULATION_DEPS_ROOTFS) > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..."  \
	  && ./b2 > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] installing ..." \
	  && ./b2 install > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONY: _cmake simula-cmake cmake_

# NOTE: cmake >= 3.23.xx is required for finding OpenSSL3 in /lib64 suffix
CMAKE_URL?=https://cmake.org/files/v3.23/cmake-3.23.0-linux-x86_64.sh
_CMAKE_VERSION=$(notdir $(lastword $(subst /, ,$(CMAKE_URL))))
CMAKE_VERSION=$(_CMAKE_VERSION:%-Linux-x86_64.sh=%)

# $(error $(shell ls -l $(SIMULATION_HOST_XTOOLS)/bin/cmake))

_cmake: $(SIMULATION_HOST_XTOOLS)/bin/cmake
	$Q echo "[$@] Already preparation"

$(SIMULATION_HOST_XTOOLS)/bin/cmake:
	@$(MAKE) --no-print-directory simula-cmake

simula-cmake: cmake_
cmake_:
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$(_CMAKE_VERSION) $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_$(_CMAKE_VERSION).log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(CMAKE_URL)] to [$@]" \
	  && wget -O $(SIMULATION_DEPS_SOURCE)/$(_CMAKE_VERSION) $(CMAKE_URL) > $(SIMULATION_DEPS_SOURCE)/__download_$(_CMAKE_VERSION).log 2>&1
	$Q echo "[$@] setting up [$(SIMULATION_DEPS_SOURCE)/$@]" \
	  && cd $(SIMULATION_DEPS_SOURCE) && mkdir -p $(SIMULATION_DEPS_SOURCE)/$@ && mv $(_CMAKE_VERSION) $(SIMULATION_DEPS_SOURCE)/$@ \
	  && chmod a+x $(SIMULATION_DEPS_SOURCE)/$@/$(_CMAKE_VERSION)
#-> 3. [compile]
	$Q echo "[$@] configuration & compilation are not needed"
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] installing ..." \
	  && ./$(_CMAKE_VERSION) --prefix=$(SIMULATION_HOST_XTOOLS) --skip-license > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONY: _vsomeip simula-vsomeip vsomeip_

# NOTE: cmake >= 3.13.xx is required for vsomeip 3.4.9-r1
VSOMEIP_URL?=https://github.com/COVESA/vsomeip/archive/refs/tags/3.4.10.tar.gz
_VSOMEIP_VERSION=$(notdir $(lastword $(subst /, ,$(VSOMEIP_URL))))
VSOMEIP_VERSION=$(_VSOMEIP_VERSION:%.tar.gz=vsomeip-%)

_vsomeip: $(SIMULATION_DEPS_ROOTFS)/include/vsomeip/vsomeip.hpp
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/vsomeip/vsomeip.hpp:
	$Q $(MAKE) --no-print-directory simula-vsomeip

simula-vsomeip: vsomeip_
vsomeip_: _cmake _boost
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$(_VSOMEIP_VERSION) $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_$(_VSOMEIP_VERSION).log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(VSOMEIP_URL)]" \
	  && wget -O $(SIMULATION_DEPS_SOURCE)/$(_VSOMEIP_VERSION) $(VSOMEIP_URL) > $(SIMULATION_DEPS_SOURCE)/__download_$(_VSOMEIP_VERSION).log 2>&1
	$Q echo "[$@] extract to [$(SIMULATION_DEPS_SOURCE)/$@]" \
	  && cd $(SIMULATION_DEPS_SOURCE) \
	  && tar xfz $(_VSOMEIP_VERSION) && mv $(VSOMEIP_VERSION) $@
#-> 3. [compile]
	$Q echo "[$@] patch to support multiple routing managers in single process"
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	&& patch -p1 < $(SIMULATION_HOME)/deps/patches/0001-Fix-bug-to-support-multiple-routing-managers.patch
	$Q echo "[$@] patch to fix vsomeip client not returning to initial wait phase"
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	&& patch -p1 < $(SIMULATION_HOME)/deps/patches/0002-Fix-vsomeip-client-not-returning-to-initial-wait-phase.patch
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && mkdir -p build && cd build \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake \
	      -DBoost_INCLUDE_DIR=$(SIMULATION_DEPS_ROOTFS)/include \
	      -DBoost_LIBRARY_DIR=$(SIMULATION_DEPS_ROOTFS)/lib \
	      -DENABLE_SIGNAL_HANDLING=1 \
	      -DENABLE_MULTIPLE_ROUTING_MANAGERS=1 \
	      -DCMAKE_INSTALL_PREFIX=$(SIMULATION_DEPS_ROOTFS) -DCMAKE_PREFIX_PATH=$(SIMULATION_DEPS_ROOTFS) .. > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . -- -j $(shell nproc) > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@/build \
	  && echo "[$@] installing ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . --target install > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONY: _openssl simula-openssl openssl_

OPENSSL_URL?=https://github.com/openssl/openssl.git
OPENSSL_VERSION=openssl-3.0.9

_openssl: $(SIMULATION_DEPS_ROOTFS)/include/openssl/opensslconf.h
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/openssl/opensslconf.h:
	$Q $(MAKE) --no-print-directory simula-openssl

simula-openssl: openssl_
openssl_:
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_$(OPENSSL_VERSION).log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(OPENSSL_URL)]" \
	  && git clone -q --depth 1 --branch ${OPENSSL_VERSION} --single-branch \
	         ${OPENSSL_URL} $(SIMULATION_DEPS_SOURCE)/$@ > $(SIMULATION_DEPS_SOURCE)/__download_$(OPENSSL_VERSION).log 2>&1
	$Q echo "[$@] just from git repo, no need to extract"
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && ./Configure --prefix=${SIMULATION_DEPS_ROOTFS} > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && make > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] installing ..." \
	    && make -j $(shell nproc) install_sw > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONY: _curl simula-curl curl_

CURL_URL?=https://github.com/curl/curl.git
CURL_VERSION=curl-7_69_1

_curl: $(SIMULATION_DEPS_ROOTFS)/include/curl/curl.h
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/curl/curl.h:
	$Q $(MAKE) --no-print-directory simula-curl

simula-curl: curl_
curl_: _cmake _openssl
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_$(CURL_VERSION).log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(CURL_URL)]" \
	  && git clone -q --depth 1 --branch ${CURL_VERSION} --single-branch \
	         ${CURL_URL} $(SIMULATION_DEPS_SOURCE)/$@ > $(SIMULATION_DEPS_SOURCE)/__download_$(CURL_VERSION).log 2>&1
	$Q echo "[$@] just from git repo, no need to extract"
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && mkdir -p build && cd build \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake -DCMAKE_INSTALL_PREFIX=${SIMULATION_DEPS_ROOTFS} -DCMAKE_PREFIX_PATH=$(SIMULATION_DEPS_ROOTFS) .. > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . -- -j $(shell nproc) > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@/build \
	  && echo "[$@] installing ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . --target install > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONY: _capi_core_rt simula-capi-core-rt capi_core_rt_

CAPI_CORE_RT_URL?=https://git.codelinaro.org/clo/la/platform/external/capicxx-core-runtime.git
CAPI_CORE_RT_VERSION=capi_core_rt_3.2.0

_capi_core_rt: $(SIMULATION_DEPS_ROOTFS)/lib/libCommonAPI.so
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/lib/libCommonAPI.so:
	$Q $(MAKE) --no-print-directory simula-capi-core-rt

simula-capi-core-rt: capi_core_rt_
capi_core_rt_: _cmake
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_$(CAPI_CORE_RT_VERSION).log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(CAPI_CORE_RT_URL)]" \
	  && git clone -q --branch github/master \
	         ${CAPI_CORE_RT_URL} $(SIMULATION_DEPS_SOURCE)/$@ > $(SIMULATION_DEPS_SOURCE)/__download_$(CAPI_CORE_RT_VERSION).log 2>&1 \
	  && cd $(SIMULATION_DEPS_SOURCE)/$@ \
	    && git checkout 89720d3c63bbd22cbccc80cdc92c2f2dd20193ba > /dev/null 2>&1
	$Q echo "[$@] just from git repo, no need to extract"
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && sed -i 's/-Werror=extra-semi//g' CMakeLists.txt \
	    && mkdir -p build && cd build \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake -DCMAKE_INSTALL_PREFIX=${SIMULATION_DEPS_ROOTFS} \
	             -DCMAKE_PREFIX_PATH=${SIMULATION_DEPS_ROOTFS} .. > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . -- -j $(shell nproc) > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@/build \
	  && echo "[$@] installing ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . --target install > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONY: _capi_someip_rt simula-capi-someip-rt capi_someip_rt_

CAPI_SOMEIP_RT_URL?=https://git.codelinaro.org/clo/la/platform/external/capicxx-someip-runtime.git
CAPI_SOMEIP_RT_VERSION=capi_someip_rt_3.2.0

_capi_someip_rt: _vsomeip $(SIMULATION_DEPS_ROOTFS)/lib/libCommonAPI-SomeIP.so
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/lib/libCommonAPI-SomeIP.so:
	$Q $(MAKE) --no-print-directory simula-capi-someip-rt

simula-capi-someip-rt: capi_someip_rt_
capi_someip_rt_: _cmake
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_$(CAPI_SOMEIP_RT_VERSION).log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(CAPI_SOMEIP_RT_URL)]" \
	  && git clone -q --branch github/master \
	         ${CAPI_SOMEIP_RT_URL} $(SIMULATION_DEPS_SOURCE)/$@ > $(SIMULATION_DEPS_SOURCE)/__download_$(CAPI_SOMEIP_RT_VERSION).log 2>&1 \
	  && cd $(SIMULATION_DEPS_SOURCE)/$@ \
	    && git checkout 0ad2bdc1807fc0f078b9f9368a47ff2f3366ed13 > /dev/null 2>&1
	$Q echo "[$@] just from git repo, no need to extract"
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && mkdir -p build && cd build \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake -DCMAKE_INSTALL_PREFIX=${SIMULATION_DEPS_ROOTFS} \
	             -DCMAKE_PREFIX_PATH=${SIMULATION_DEPS_ROOTFS} .. > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . -- -j $(shell nproc) > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@/build \
	  && echo "[$@] installing ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . --target install > ./__install.log 2>&1
	$Q echo "[$@] Done"


# Common API tools
CAPI_CORE_GENERATOR_URL?=https://github.com/GENIVI/capicxx-core-tools/releases/download/3.2.0.1/commonapi_core_generator.zip
CAPI_SOMEIP_GENERATOR_URL?=https://github.com/GENIVI/capicxx-someip-tools/releases/download/3.2.0.1/commonapi_someip_generator.zip
CAPI_CORE_GENERATOR_VERSION=capi_core_gen_3.2.0.1
CAPI_SOMEIP_GENERATOR_VERSION=capi_someip_gen_3.2.0.1

CAPI_CORE_TOOLS_URL?=https://github.com/GENIVI/capicxx-core-tools.git

.PHONY: _capi_tools simula-capi-core-generator simula-capi-someip-generator simula-capi-core-tools capi_core_tools_

_capi_tools: $(SIMULATION_HOST_XTOOLS)/bin/commonapi_core_generator/commonapi-core-generator-linux-x86_64 \
             $(SIMULATION_HOST_XTOOLS)/bin/commonapi_someip_generator/commonapi-someip-generator-linux-x86_64 \
             $(SIMULATION_DEPS_ROOTFS)/bin/E01HelloWorldService
	$Q echo "[$@] Already preparation"

$(SIMULATION_HOST_XTOOLS)/bin/commonapi_core_generator/commonapi-core-generator-linux-x86_64:
	$Q $(MAKE) --no-print-directory simula-capi-core-generator

$(SIMULATION_HOST_XTOOLS)/bin/commonapi_someip_generator/commonapi-someip-generator-linux-x86_64:
	$Q $(MAKE) --no-print-directory simula-capi-someip-generator

$(SIMULATION_DEPS_ROOTFS)/bin/E01HelloWorldService:
	$Q $(MAKE) --no-print-directory simula-capi-core-tools

simula-capi-core-generator:
	$Q echo "[capi_core_generator_] downloading binary tools from [$(CAPI_CORE_GENERATOR_URL)]" \
	  && cd $(SIMULATION_DEPS_SOURCE) \
	  && wget -O $(SIMULATION_DEPS_SOURCE)/$(CAPI_CORE_GENERATOR_VERSION).zip $(CAPI_CORE_GENERATOR_URL) > ./__download_$(CAPI_CORE_GENERATOR_VERSION).log 2>&1 \
	&& echo "[capi_core_generator_] extract zip files" \
	  && mkdir -p $(SIMULATION_HOST_XTOOLS)/bin/commonapi_core_generator \
	  && unzip $(SIMULATION_DEPS_SOURCE)/$(CAPI_CORE_GENERATOR_VERSION).zip -d $(SIMULATION_HOST_XTOOLS)/bin/commonapi_core_generator > /dev/null

simula-capi-someip-generator:
	$Q echo "[capi_someip_generator_] downloading binary tools from [$(CAPI_SOMEIP_GENERATOR_URL)]" \
	  && cd $(SIMULATION_DEPS_SOURCE) \
	  && wget -O $(SIMULATION_DEPS_SOURCE)/$(CAPI_SOMEIP_GENERATOR_VERSION).zip $(CAPI_SOMEIP_GENERATOR_URL) > ./__download_$(CAPI_SOMEIP_GENERATOR_VERSION).log 2>&1 \
	&& echo "[capi_someip_generator_] extract zip files" \
	  && mkdir -p $(SIMULATION_HOST_XTOOLS)/bin/commonapi_someip_generator \
	  && unzip $(SIMULATION_DEPS_SOURCE)/$(CAPI_SOMEIP_GENERATOR_VERSION).zip -d $(SIMULATION_HOST_XTOOLS)/bin/commonapi_someip_generator > /dev/null

simula-capi-core-tools: capi_core_tools_
capi_core_tools_: _cmake
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$@ $(SIMULATION_DEPS_SOURCE)/__download_capicxx-core-tools.log
#-> 2. [download]
	$Q echo "[$@] downloading from [$(CAPI_CORE_TOOLS_URL)]" \
	  && git clone -q --branch master \
	         ${CAPI_CORE_TOOLS_URL} $(SIMULATION_DEPS_SOURCE)/$@ > $(SIMULATION_DEPS_SOURCE)/__download_capicxx-core-tools.log 2>&1 \
	  && cd $(SIMULATION_DEPS_SOURCE)/$@ \
	    && git checkout 5ed80df56a64fdd9bf22b2f240a608e77be8262c > /dev/null 2>&1
	$Q echo "[$@] just from git repo, no need to extract"
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] patch the core tools for bypassing DBus" \
	    && git am $(SIMULATION_HOME)/deps/patches/0001-telaf-Bypass-DBus-for-common-api-demo.patch > /dev/null 2>&1 \
	  && echo "[$@] export generator tools" \
	    && export PATH=$(SIMULATION_HOST_XTOOLS)/bin:$$PATH \
	    && export PATH=$(SIMULATION_HOST_XTOOLS)/bin/commonapi_core_generator:$$PATH \
	    && export PATH=$(SIMULATION_HOST_XTOOLS)/bin/commonapi_someip_generator:$$PATH \
	  && echo "[$@] generate codes with x.fidl files" \
	    && cd CommonAPI-Examples/E01HelloWorld \
	    && commonapi-core-generator-linux-x86_64 -sk ./fidl/E01HelloWorld.fidl -d ./src-gen/core > __generate_idl_core.log 2>&1 \
	    && commonapi-someip-generator-linux-x86_64 ./fidl/E01HelloWorld-SomeIP.fdepl -d ./src-gen/someip > __generate_idl_someip.log 2>&1 \
	  && mkdir -p build && cd build \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake -DCMAKE_PREFIX_PATH=${SIMULATION_DEPS_ROOTFS} \
	             -DINSTALL_DIR=${SIMULATION_DEPS_ROOTFS} .. > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . -- -j $(shell nproc) > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@/CommonAPI-Examples/E01HelloWorld/build \
	  && echo "[$@] installing ..." \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake --build . --target install > ./__install.log 2>&1
	$Q echo "[$@] Done"

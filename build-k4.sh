#!/bin/bash
##############################################################################
# Setup script for CEPCSW:
# - build the cepcsw
#
# Usage:
# $ bash build.sh
# or:
# $ 
#
# Author: Tao Lin <lintao@ihep.ac.cn>
##############################################################################

function info:() {
    echo "INFO: $*" 1>&2
}

function error:() {
    echo "ERROR: $*" 1>&2
}

function check-cepcsw-envvar() {
    source /cvmfs/sw.hsf.org/key4hep/setup.sh
}

function build-dir() {
    local blddir=build

    # If detect the extra env var, append it to the build dir
    if [ -n "${k4_version}" ]; then
        blddir=${blddir}.${k4_version}
    fi
    if [ -n "${k4_platform}" ]; then
        blddir=${blddir}.${k4_platform}
    fi

    echo $blddir
}

function check-working-builddir() {
    local blddir=$(build-dir)
    if [ ! -d "$blddir" ]; then
        mkdir $blddir || {
            error: "Failed to create $blddir"
            return 1
        }
    fi
}

function run-cmake() {
    local blddir=$(build-dir)

    if [ ! -d "$blddir" ]; then
        error: "Failed to create $blddir"
        return 1
    fi

    cd $blddir

    local clhep_prefix=$(clhep-config --prefix)
    local clhep_include=${clhep_prefix}/include

    # locate the pandorapfa
    local pandorapfa=$(echo $CMAKE_PREFIX_PATH | tr ':' '\n' | grep pandorapfa | head -n1)
    info: "Find PandoraPFA: $pandorapfa"
    if [ -z "$pandorapfa" ]; then
        error: "Failed to find the PandoraPFA"
        return 1
    fi
    
    local pandorapfa_cmake_modules=${pandorapfa}/cmakemodules
    info: "Find PandoraPFA cmake: ${pandorapfa_cmake_modules}"
    if [ ! -d "${pandorapfa_cmake_modules}" ]; then
        error: "Failed to find the cmake modules for PandoraPFA: ${pandorapfa_cmake_modules}"
        return 1
    fi

    local cmake_modules=${pandorapfa_cmake_modules}

    cmake .. -DHOST_BINARY_TAG=${k4_platform} \
          -DCLHEP_INCLUDE_DIR=${clhep_include} \
          -DCMAKE_MODULE_PATH=${cmake_modules} || {
        error: "Failed to cmake"
        return 1
    }

}

function run-make() {
    make
}

##############################################################################
# Parse the command line options
##############################################################################

# The current default platform
k4_platform=x86_64-linux-gcc9-opt
k4_version=master

check-cepcsw-envvar || exit -1

check-working-builddir || exit -1

run-cmake || exit -1

run-make || exit -1

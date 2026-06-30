#!/bin/bash

##########################################################################################################################################
# [Tested Environment] 
#
# MSBuild version 18.6.3+84d3e95b4 for .NET Framework
# -- Building for: Visual Studio 18 2026
# -- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
# -- The CXX compiler identification is MSVC 19.51.36246.0
# -- Detecting CXX compiler ABI info
# -- Detecting CXX compiler ABI info - done
# -- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe - skipped
##########################################################################################################################################

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

source $basedir/install_cmake_and_prepare_msbuild_envs.sh

if [ -z $1 ]; then
    echo "Usage/1: $0 <Debug|Release>"
fi

BUILD_TYPE=$1
PB_VERSION=31.1
PB_SRC_TAR="protobuf-${PB_VERSION}.tar.gz"
PB_SRC_URL="https://github.com/protocolbuffers/protobuf/archive/refs/tags/v${PB_VERSION}.tar.gz"
PB_DOWNLOAD_TEMP="$HOME/Downloads/${PB_SRC_TAR}"
PB_EXTRACTION_TEMP="$HOME/Downloads/protobuf-${PB_VERSION}-extraction-tmp"
export PB_INSTALL_DIR="$HOME/Protobuf-${PB_VERSION}-${BUILD_TYPE}"

PB_DEPENDENCIES_DIR="$HOME/Protobuf-${PB_VERSION}-${BUILD_TYPE}-Dependencies"

ABSEIL_VERSION=20250127.0
ABSEIL_SRC_TAR=abseil-cpp-${ABSEIL_VERSION}.tar.gz
ABSEIL_SRC_URL=https://github.com/abseil/abseil-cpp/releases/download/${ABSEIL_VERSION}/${ABSEIL_SRC_TAR}
ABSEIL_DOWNLOAD_TEMP="$HOME/Downloads/${ABSEIL_SRC_TAR}"
ABSEIL_EXTRACTION_TEMP="$HOME/Downloads/abseil-${ABSEIL_VERSION}-extraction-tmp"
export ABSEIL_INSTALL_DIR="${PB_DEPENDENCIES_DIR}/Abseil-${ABSEIL_VERSION}"

if [ ! -d "${ABSEIL_INSTALL_DIR}" ]; then
    # Reference https://abseil.io/docs/cpp/tools/cmake-installs
    if [ ! -d "${ABSEIL_EXTRACTION_TEMP}" ]; then
        if [ ! -f ${ABSEIL_DOWNLOAD_TEMP} ]; then
            echo "Downloading ABSEIL source ${ABSEIL_VERSION}"...
            curl -L "${ABSEIL_SRC_URL}" -o "${ABSEIL_DOWNLOAD_TEMP}"
        fi
        echo Extracting Abseil ${ABSEIL_VERSION} from source tar...
        mkdir -p ${ABSEIL_EXTRACTION_TEMP}
        tar -xzvf ${ABSEIL_DOWNLOAD_TEMP} -C ${ABSEIL_EXTRACTION_TEMP} --strip-components 1
    fi

    echo Installing Abseil ${ABSEIL_VERSION} from extracted source...
    mkdir -p ${ABSEIL_EXTRACTION_TEMP}/build

    # "MSBuild/Visual Studio" will automatically transit into "multi-configuration build". 
    cd ${ABSEIL_EXTRACTION_TEMP}/build && \
        cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_CONFIGURATION_TYPES="Debug;Release;MinSizeRel;RelWithDebInfo" -DABSL_BUILD_MONOLITHIC_SHARED_LIBS=OFF -DABSL_ENABLE_INSTALL=ON -DABSL_MSVC_STATIC_RUNTIME=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DABSL_BUILD_TESTING=OFF -DABSL_BUILD_TEST_HELPERS=OFF -DABSL_USE_GOOGLETEST_HEAD=OFF -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=${ABSEIL_INSTALL_DIR} .. && \
        cmake --build . --config ${BUILD_TYPE} --target install -j 8 

    rm -rf ${ABSEIL_EXTRACTION_TEMP}/build
fi

# Download and install protobuf from source 

if [ ! -d "${PB_INSTALL_DIR}" ]; then
    if [ ! -d "${PB_EXTRACTION_TEMP}" ]; then
        if [ ! -f ${PB_DOWNLOAD_TEMP} ]; then
            echo "Downloading Protobuf source ${PB_VERSION}"...
            curl -L "${PB_SRC_URL}" -o "${PB_DOWNLOAD_TEMP}"
        fi
        mkdir -p ${PB_EXTRACTION_TEMP}
        echo Extracting Protobuf ${PB_VERSION} from source tar...
        tar -xzvf ${PB_DOWNLOAD_TEMP} -C ${PB_EXTRACTION_TEMP} --strip-components 1
    fi

    echo Installing Protobuf ${PB_VERSION} from extracted source...

    mkdir -p ${PB_EXTRACTION_TEMP}/build
    cd ${PB_EXTRACTION_TEMP}/build && \
        cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_CONFIGURATION_TYPES="Debug;Release;MinSizeRel;RelWithDebInfo" -Dabsl_DIR=${ABSEIL_INSTALL_DIR}/lib/cmake/absl -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=${PB_INSTALL_DIR} .. && \
        cmake --build . --config ${BUILD_TYPE} --target install -j 8

    rm -rf ${PB_EXTRACTION_TEMP}/build
fi

export PATH=$PATH:${PB_INSTALL_DIR}/bin # for "protoc.exe"

cd $basedir

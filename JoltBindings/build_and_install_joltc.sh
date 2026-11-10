#!/bin/bash

if [ $# -le 1 ]; then 
  echo "Usage/1: $0 <Debug|Release> <win64|linux64>"
  exit 1
fi

if [ "Debug" != $1 ] && [ "Release" != $1 ]; then 
  echo "Usage/2: $0 <Debug|Release> <win64|linux64>"
  exit 1
fi

if [ "win64" != $2 ] && [ "linux64" != $2 ]; then 
  echo "Usage/2: $0 <Debug|Release> <win64|linux64>"
  exit 1
fi

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
joltc_basedir="$basedir"joltc
unity_package_output_basedir="$basedir"../UnityPackageOutput

cmakeprojdir=$basedir/cmakeout/$2
mkdir -p $cmakeprojdir

MAXNJOBS=16
CORES=$(nproc 2>/dev/null || echo "${NUMBER_OF_PROCESSORS:-4}")
NJOBS=$(( CORES > MAXNJOBS ? MAXNJOBS : CORES ))

export BUILD_TYPE=$1 # for this session
if [ "linux64" == $2 ]; then
    # Both ${PROTOBUF_VERSION} and ${ABSEIL_VERSION} are exported by "<proj-root>/LinuxPbBuildDockerfile".
    export PB_INSTALL_DIR=/usr/protobuf-${PROTOBUF_VERSION}-${BUILD_TYPE} 
    export ABSEIL_INSTALL_DIR=/usr/abseil-${ABSEIL_VERSION}-${BUILD_TYPE} 
    echo "Exported envs PB_INSTALL_DIR=${PB_INSTALL_DIR}, ABSEIL_INSTALL_DIR=${ABSEIL_INSTALL_DIR}, CORES=${CORES}, NJOBS=${NJOBS}"
    # Exported to be used by "<proj-root>/JoltBindings/PbClasses.cmake".
fi

# This long "configuration & build" command is intentionally written to fit both "single-configuration" and "multi-configuration" build systems, within each some parameters might be ignored.

#if [ "linux64" == $2 ]; then
    #####Generic version####################
    cd $cmakeprojdir && \
        cmake -DCMAKE_UNITY_BUILD=ON -DCMAKE_UNITY_BUILD_BATCH_SIZE=8 -DCMAKE_CXX_STANDARD=17 -DCMAKE_CONFIGURATION_TYPES="Debug;Release;MinSizeRel;RelWithDebInfo" -DCMAKE_BUILD_TYPE=$1 $basedir && \
        cmake --build $cmakeprojdir --config $1 -j $NJOBS && \
        cmake --install $cmakeprojdir --prefix=$unity_package_output_basedir --config $1 -j $NJOBS
    #########################################
#else
#    #####Ninja version#######################
#    cd $cmakeprojdir && \
#        cmake -B $1 -G Ninja -DCMAKE_UNITY_BUILD=ON -DCMAKE_UNITY_BUILD_BATCH_SIZE=8 -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=$1 $basedir && \
#        cmake --build $1 --config $1 -j $NJOBS && \
#        cmake --install $1 --prefix=$unity_package_output_basedir --config $1 -j $NJOBS   
#    #########################################
#fi

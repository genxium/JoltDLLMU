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
echo "Building LibExportImportCppTest..."

cmakeprojdir=$basedir/cmakeout/$2
mkdir -p $cmakeprojdir

export BUILD_TYPE=$1 # for this session
if [ "linux64" == $2 ]; then
    # Both ${PROTOBUF_VERSION} and ${ABSEIL_VERSION} are exported by "<proj-root>/LinuxPbBuildDockerfile".
    export PB_INSTALL_DIR=/usr/protobuf-${PROTOBUF_VERSION}-${BUILD_TYPE} 
    export ABSEIL_INSTALL_DIR=/usr/abseil-${ABSEIL_VERSION}-${BUILD_TYPE} 
    echo "Exported envs PB_INSTALL_DIR=${PB_INSTALL_DIR}, ABSEIL_INSTALL_DIR=${ABSEIL_INSTALL_DIR}"
    # Exported to be used by "<proj-root>/LibExportImportCppTest/CMakeLists.txt".
fi

cd $cmakeprojdir && \
    cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_CONFIGURATION_TYPES="Debug;Release;MinSizeRel;RelWithDebInfo" -DCMAKE_BUILD_TYPE=${BUILD_TYPE} $basedir && \
    cmake --build $cmakeprojdir --config $1 -j 8 && \
    cmake --install $cmakeprojdir --config $1 -j 8   

echo "Built LibExportImportCppTest"

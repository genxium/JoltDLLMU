#!/bin/bash

if [ $# -le 1 ]; then 
  echo "Usage/1: $0 <Debug|Release> [Linux|LinuxDynamicPb]"
  exit 1
fi

BUILD_TYPE_LOWERCASE=
if [ "Debug" != $1 ] && [ "Release" != $1 ]; then 
  echo "Usage/2: $0 <Debug|Release> [Linux|LinuxDynamicPb]"
  exit 1
fi

if [[ "Debug" == $1 ]]; then
    BUILD_TYPE_LOWERCASE="debug"
else
    BUILD_TYPE_LOWERCASE="release"
fi

if [ ! -z $2 ] && [ "Linux" != $2 ] && [ "LinuxDynamicPb" != $2 ]; then 
  echo "Usage/3: $0 <Debug|Release> [Linux|LinuxDynamicPb]"
  exit 1
fi

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

reusing_id=
shared_lib_switch=
pos_indep_switch=

if [[ "Linux" == $2 ]]; then
    shared_lib_switch=OFF
    pos_indep_switch=ON
    reusing_id=joltc-linux-builder-$BUILD_TYPE_LOWERCASE
elif [[ "LinuxDynamicPb" == $2 ]]; then
    shared_lib_switch=ON
    pos_indep_switch=OFF
    reusing_id=joltc-linux-builder-dynamic-pb-$BUILD_TYPE_LOWERCASE
else 
    echo "Usage/4: $0 <Debug|Release> [Linux|LinuxDynamicPb]"
    exit 1
fi

if [ -z "$(docker images -q ${reusing_id} 2> /dev/null)" ]; then
    reusing_cmake_base_image_id=joltc-cmake-base
    if [ -z "$(docker images -q ${reusing_cmake_base_image_id} 2> /dev/null)" ]; then
        echo ""
        docker build -f $basedir/LinuxWithCMakeDockerfile -t ${reusing_cmake_base_image_id} .
    fi
    docker build --build-arg BUILD_TYPE=$1 --build-arg SHARED_LIBS_SWITCH=${shared_lib_switch} --build-arg POS_INDEP_SWITCH=${pos_indep_switch} -f $basedir/LinuxPbBuildDockerfile -t ${reusing_id} .
fi

docker run -it --rm --mount type=bind,src=$basedir/,dst=/app ${reusing_id} bash

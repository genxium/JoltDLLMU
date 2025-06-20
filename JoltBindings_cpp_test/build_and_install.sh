#!/bin/bash

if [[ ! -v VCPKG_INSTALLED_SHARE ]]; then 
   echo "Please set environment variable 'VCPKG_INSTALLED_SHARE' to point to the directory containing 'protobuf/protobuf-config.cmake' config file installed by 'vcpkg install protobuf protobuf:x64-windows'!"
   exit 1
fi

if [[ ! -v VCPKG_INSTALLED_INCLUDE ]]; then 
   echo "Please set environment variable 'VCPKG_INSTALLED_INCLUDE' to point to the directory containing protobuf header files installed by 'vcpkg install protobuf protobuf:x64-windows'!"
   exit 1
fi

if [ $# -ne 1 ]; then 
  echo "Usage: $0 [Debug|Release]"
  exit 1
fi

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
echo "Using VCPKG_INSTALLED_SHARE=${VCPKG_INSTALLED_SHARE}"
echo "Using VCPKG_INSTALLED_INCLUDE=${VCPKG_INSTALLED_INCLUDE}"
echo "Building cpp_test"
cmakeprojdir=$basedir/cmakeout
mkdir -p $cmakeprojdir
cd $cmakeprojdir && cmake -A x64 .. && cmake --build . --config $1 && cmake --install . --prefix=$basedir/cmakeout --config $1  
echo "Built cpp_test"

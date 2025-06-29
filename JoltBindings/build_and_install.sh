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

if [ "Debug" != $1 ] && [ "Release" != $1 ]; then 
  echo "Usage: $0 [Debug|Release]"
  exit 1
fi

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
joltc_basedir="$basedir"joltc
unity_package_output_basedir="$basedir"/../UnityPackageOutput
protoc -I=$basedir --csharp_out=. serializable_data.proto 

echo "Using VCPKG_INSTALLED_SHARE=${VCPKG_INSTALLED_SHARE}"
echo "Using VCPKG_INSTALLED_INCLUDE=${VCPKG_INSTALLED_INCLUDE}"
echo "Building joltc.dll with JoltPhysics engine..."
cmakeprojdir=$basedir/cmakeout
mkdir -p $cmakeprojdir
mkdir -p $cmakeprojdir/pb_gen
pb_gen_root=$cmakeprojdir/pb_gen
export PB_GEN_ROOT=$pb_gen_root # for this session
cd $cmakeprojdir && cmake -A x64 .. && cmake --build . --config $1 && cmake --install . --prefix=$unity_package_output_basedir --config $1  
echo "Built joltc.dll with JoltPhysics engine"

echo "Building joltphysics.dll for CSharp bindings..."
cd $basedir && dotnet build -c $1
echo "Built joltphysics.dll for CSharp bindings"

echo "Writing PbConsts for generic import..."
cd $basedir/../PbConstsWriter && dotnet build -c $1 && dotnet run --environment UNITY_RT_PLUGINS_OUTPATH=$unity_package_output_basedir/Runtime/Plugins -c $1
echo "Written PbConsts for generic import"

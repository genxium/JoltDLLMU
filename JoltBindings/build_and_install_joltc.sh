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

export BUILD_TYPE=$1 # for this session
if [ "win64" == $2 ]; then
    # "MSBuild/Visual Studio" will automatically transit into "multi-configuration build". 
    cd $cmakeprojdir && cmake -DCMAKE_CXX_STANDARD=17 $basedir && cmake --build $cmakeprojdir -j 8 --config $1 && cmake --install $cmakeprojdir -j 8 --prefix=$unity_package_output_basedir --config $1   
else
    cd $cmakeprojdir && cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_CONFIGURATION_TYPES=Debug,Release $basedir && cmake --build $cmakeprojdir -j 8 --config $1 && cmake --install $cmakeprojdir -j 8 --prefix=$unity_package_output_basedir --config $1   
fi

#!/bin/bash

if [ $# -le 0 ]; then 
  echo "Usage/1: $0 <Debug|Release>"
  exit 1
fi

if [ "Debug" != $1 ] && [ "Release" != $1 ]; then 
  echo "Usage/2: $0 <Debug|Release>"
  exit 1
fi

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
joltc_basedir="$basedir"joltc
unity_package_output_basedir="$basedir"../UnityPackageOutput

cmakeprojdir=$basedir/cmakeout
mkdir -p $cmakeprojdir
mkdir -p $cmakeprojdir/pb_gen
pb_gen_root=$cmakeprojdir/pb_gen
export PB_GEN_ROOT=$pb_gen_root # for this session
cd $cmakeprojdir && cmake -DCMAKE_CXX_STANDARD=17 $basedir && cmake --build $cmakeprojdir -j 8 --config $1 && cmake --install $cmakeprojdir -j 8 --prefix=$unity_package_output_basedir --config $1  

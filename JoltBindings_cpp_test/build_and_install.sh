#!/bin/bash

if [ $# -ne 1 ]; then 
  echo "Usage: $0 [Debug|Release]"
  exit 1
fi

if [ "Debug" != $1 ] && [ "Release" != $1 ]; then 
  echo "Usage: $0 [Debug|Release]"
  exit 1
fi

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
echo "Building cpp_test"
cmakeprojdir=$basedir/cmakeout
mkdir -p $cmakeprojdir
cd $cmakeprojdir && cmake .. && cmake --build . -j 8 --config $1 && cmake --install . -j 8 --prefix=$basedir/cmakeout --config $1  
echo "Built cpp_test"

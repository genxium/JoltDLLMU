#!/bin/bash

if [ $# -le 0 ]; then 
  echo "Usage/1: $0 <Debug|Release> [Linux]"
  exit 1
fi

if [ "Debug" != $1 ] && [ "Release" != $1 ]; then 
  echo "Usage/2: $0 <Debug|Release> [Linux]"
  exit 1
fi

if [ ! -z $2 ] && [ "Linux" != $2 ]; then 
  echo "Usage/3: $0 <Debug|Release> [Linux]"
  exit 1
fi

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
JoltBindings_basedir="$basedir"/JoltBindings
unity_package_output_basedir="$basedir"/UnityPackageOutput

echo "Writing PbConsts for generic import..."
protoc -I=$JoltBindings_basedir --csharp_out=$JoltBindings_basedir serializable_data.proto 
cd $basedir/PbConstsWriter && dotnet build -c $1 && dotnet run --environment UNITY_RT_PLUGINS_OUTPATH=$unity_package_output_basedir/Runtime/Plugins -c $1
echo "Written PbConsts for generic import"

clibname="joltc"
if [[ "Linux" == $2 ]]; then 
  clibname="libjoltc"
  echo "clibname changed to : $clibname"
fi

echo "Building $clibname with JoltPhysics engine..."
if [[ "Linux" == $2 ]]; then
    reusing_id=joltc-linux-builder
    if [ -z "$(docker images -q $reusing_id 2> /dev/null)" ]; then
        docker build -f $basedir/LinuxBuildDockerfile -t $reusing_id .
    fi
    docker run -it --rm --mount type=bind,src=$basedir/,dst=/app $reusing_id bash -c "cd JoltBindings && ./build_and_install_joltc.sh $1"
else
    $JoltBindings_basedir/build_and_install_joltc.sh $1
fi
echo "Built $clibname with JoltPhysics engine"

echo "Building joltphysics.dll for CSharp bindings..."
cd $JoltBindings_basedir && dotnet build -c $1
echo "Built joltphysics.dll for CSharp bindings"

LibExportImportCppTestName=LibExportImportCppTest
if [[ "Linux" == $2 ]]; then
    docker run -it --rm --mount type=bind,src=$basedir/,dst=/app $reusing_id bash -c "cd $LibExportImportCppTestName && ./build_and_install.sh $1"
else
    $basedir/$LibExportImportCppTestName/build_and_install.sh $1
fi

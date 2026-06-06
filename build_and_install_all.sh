#!/bin/bash

if [ $# -le 0 ]; then 
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
JoltBindings_basedir="$basedir"/JoltBindings
unity_package_output_basedir="$basedir"/UnityPackageOutput

echo "Writing PbConsts for generic import..."
source $basedir/install_protoc_and_prepare_envs.sh $1 # For "protoc.exe"
protoc -I=$JoltBindings_basedir --csharp_out=$JoltBindings_basedir serializable_data.proto 
cd $basedir/PbConstsWriter && dotnet build -c $1 && dotnet run --environment UNITY_RT_PLUGINS_OUTPATH=$unity_package_output_basedir/Runtime/Plugins -c $1
echo "Written PbConsts for generic import"

clibname="joltc"

reusing_id=
reusing_cmake_base_image_id=joltc-cmake-base
shared_lib_switch=
pos_indep_switch=
use_static_pb_value=
if [ ! -z $2 ]; then 
    if [ "Linux" == $2 ] || [ "LinuxDynamicPb" == $2 ]; then 
        reusing_cmake_base_image_id=joltc-cmake-base

        if [[ "Linux" == $2 ]]; then
            shared_lib_switch=OFF
            pos_indep_switch=ON
            reusing_id=joltc-linux-builder-$BUILD_TYPE_LOWERCASE
            use_static_pb_value=true
        elif [[ "LinuxDynamicPb" == $2 ]]; then
            shared_lib_switch=ON
            pos_indep_switch=OFF
            reusing_id=joltc-linux-builder-dynamic-pb-$BUILD_TYPE_LOWERCASE
            use_static_pb_value=false
        else 
            use_static_pb_value=true
        fi
    else 
        use_static_pb_value=true
    fi
else 
    use_static_pb_value=true
fi


if [ -z ${reusing_id} ]; then
    echo "Building ${clibname} with JoltPhysics engine, use_static_pb_value=${use_static_pb_value}..."
    USE_STATIC_PB=${use_static_pb_value} $JoltBindings_basedir/build_and_install_joltc.sh $1 win64
else
    echo "Building ${clibname} with JoltPhysics engine, use_static_pb_value=${use_static_pb_value}, reusing_id=${reusing_id}, shared_lib_switch=${shared_lib_switch}, pos_indep_switch=${pos_indep_switch}..."
    if [ -z "$(docker images -q ${reusing_id} 2> /dev/null)" ]; then
        if [ -z "$(docker images -q ${reusing_cmake_base_image_id} 2> /dev/null)" ]; then
            docker build -f $basedir/LinuxWithCMakeDockerfile -t ${reusing_cmake_base_image_id} .
        fi
        docker build --build-arg BUILD_TYPE=$1 --build-arg SHARED_LIBS_SWITCH=${shared_lib_switch} --build-arg POS_INDEP_SWITCH=${pos_indep_switch} -f $basedir/LinuxPbBuildDockerfile -t ${reusing_id} .
    fi
    docker run -it --rm --mount type=bind,src=$basedir/,dst=/app ${reusing_id} bash -c "cd JoltBindings && USE_STATIC_PB=${use_static_pb_value} ./build_and_install_joltc.sh $1 linux64"
fi

echo "Built $clibname with JoltPhysics engine"

echo "Building joltphysics.dll for CSharp bindings..."
cd $JoltBindings_basedir && dotnet build -c $1
echo "Built joltphysics.dll for CSharp bindings"

LibExportImportCppTestName=LibExportImportCppTest
if [[ "Linux" == $2 ]]; then
    docker run -it --rm --mount type=bind,src=$basedir/,dst=/app ${reusing_id} bash -c "cd $LibExportImportCppTestName && USE_STATIC_PB=${use_static_pb_value} ./build_and_install.sh $1 linux64"
elif [[ "LinuxDynamicPb" == $2 ]]; then
    docker run -it --rm --mount type=bind,src=$basedir/,dst=/app ${reusing_id} bash -c "cd $LibExportImportCppTestName && ./build_and_install.sh $1 linux64"
else
    USE_STATIC_PB=${use_static_pb_value} $basedir/$LibExportImportCppTestName/build_and_install.sh $1 win64 
fi

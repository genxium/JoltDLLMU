#!/usr/bin/env bash
set -e 

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

# Download and install prebuilt CMake
CMAKE_VERSION=4.3.3 # Only since 4.2.0 was support of "Visual Studio 18 2026" introduced
CMAKE_ZIP="cmake-${CMAKE_VERSION}-windows-x86_64.zip"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/${CMAKE_ZIP}"
CMAKE_DOWNLOAD_TEMP="$HOME/Downloads/${CMAKE_ZIP}"
CMAKE_EXTRACTION_TEMP="$HOME/Downloads/CMake-${CMAKE_VERSION}-Extraction-Temp"
CMAKE_INSTALL_DIR="$HOME/CMake-${CMAKE_VERSION}"

if [ ! -d "${CMAKE_INSTALL_DIR}" ]; then
    if [ ! -f ${CMAKE_DOWNLOAD_TEMP} ]; then
        echo "Downloading CMake ${CMAKE_VERSION}"...
        curl -L "${CMAKE_URL}" -o "${CMAKE_DOWNLOAD_TEMP}"
    fi

    echo Installing CMake ${CMAKE_VERSION} from zip...
    unzip ${CMAKE_DOWNLOAD_TEMP} -d ${CMAKE_EXTRACTION_TEMP} # We're using Git Bash on Windows, so "unzip" is certainly installed by now
    mkdir -p ${CMAKE_INSTALL_DIR}
    mv ${CMAKE_EXTRACTION_TEMP}/**/* ${CMAKE_INSTALL_DIR}/
    #rm ${CMAKE_DOWNLOAD_TEMP}
    rm -rf ${CMAKE_EXTRACTION_TEMP}
fi

export PATH=$PATH:${CMAKE_INSTALL_DIR}/bin # For "cmake.exe"

# Dynamically locate Visual Studio 2017/2019/2022 and export only necessary environment variables, i.e. $PATH
if [ -z "$VS_INSTALL_DIR" ]; then
    echo "-> Locating Visual Studio installation..."
    # Search common paths for the vswhere utility
    VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    
    if [ -f "$VSWHERE" ]; then
        # Use vswhere to find the latest VS installation path
        VS_INSTALL_DIR_RAW=$("$VSWHERE" -latest -property installationPath)
        VS_INSTALL_DIR=$(printf '%s' "$VS_INSTALL_DIR_RAW" | sed 's#\\#/#g' | sed 's#C:#/c#g')
        echo "-> Found Visual Studio at: $VS_INSTALL_DIR"
        MSVC_VER=$(< "$VS_INSTALL_DIR/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt")
        echo "-> Found MSVC_VER: $MSVC_VER"

        export MSVC_X64_CL_FOLDER="$VS_INSTALL_DIR/VC/Tools/MSVC/${MSVC_VER}/bin/Hostx64/x64"
        export MSVC_X86_CL_FOLDER="$VS_INSTALL_DIR/VC/Tools/MSVC/${MSVC_VER}/bin/Hostx86/x86"

        export PATH=$PATH:${MSVC_X64_CL_FOLDER}
    else
        echo "Warning: vswhere.exe not found. Defaulting to system environment."
    fi
fi

export PATH=$PATH:"$VS_INSTALL_DIR/MSBuild/Current/Bin" # For "msbuild.exe"

# Verify Installations by far
echo "--- Environment Verification ---"
msbuild.exe -version | head -n 1

cd $basedir

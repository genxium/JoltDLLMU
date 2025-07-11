#!/bin/bash

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
JoltBindings_basedir="$basedir"/JoltBindings
rm -rf $JoltBindings_basedir/cmakeout
rm -rf $JoltBindings_basedir/joltc/*.pb.*
rm -rf $JoltBindings_basedir/obj
rm -rf $JoltBindings_basedir/bin

UnityPackageOutputPath=$basedir/UnityPackageOutput
echo "UnityPackageOutputPath=${UnityPackageOutputPath}"

# *.meta files
rm -rf $UnityPackageOutputPath/*.meta
rm -rf $UnityPackageOutputPath/Runtime/*.meta
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.meta

# Dotnet/CShapr part
rm -rf $UnityPackageOutputPath/Runtime/*.dll
rm -rf $UnityPackageOutputPath/Runtime/*.pdb

# Native part
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.h
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.pb
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.gliffy
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.inl
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.dll 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.so 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.so.* 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.dylib 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.pdb 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.lib 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.exp 

# Delete header hierarchy
rm -rf $UnityPackageOutputPath/Runtime/Plugins/include/*
echo "" > $UnityPackageOutputPath/Runtime/Plugins/include/.keep

# Managed part
rm -rf $UnityPackageOutputPath/Runtime/*.cs 

# Also delete "cmakeout" for "LibExportImportCppTest"  
LibExportImportCppTestName=LibExportImportCppTest
rm -rf $basedir/$LibExportImportCppTestName/cmakeout 

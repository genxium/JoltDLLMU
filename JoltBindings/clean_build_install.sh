#!/bin/bash

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
rm -rf $basedir/cmakeout
rm -rf $basedir/joltc/*.pb.*
rm -rf $basedir/obj
rm -rf $basedir/bin

UnityPackageOutputPath=$basedir/../UnityPackageOutput
echo "UnityPackageOutputPath=${UnityPackageOutputPath}"

# *.meta files
rm -rf $UnityPackageOutputPath/*.meta
rm -rf $UnityPackageOutputPath/Runtime/*.meta
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.meta

# Native part
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.h
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.gliffy
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.inl
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.dll 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.pdb 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.lib 
rm -rf $UnityPackageOutputPath/Runtime/Plugins/*.exp 

# Delete header hierarchy
rm -rf $UnityPackageOutputPath/Runtime/Plugins/include/*
echo "" > $UnityPackageOutputPath/Runtime/Plugins/include/.keep

# Managed part
rm -rf $UnityPackageOutputPath/Runtime/*.cs 

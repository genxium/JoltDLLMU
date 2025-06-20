#!/bin/bash

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/
joltc_basedir="$basedir"joltc
protoc -I=$basedir --csharp_out=. serializable_data.proto 

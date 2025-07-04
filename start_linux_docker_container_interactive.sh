#!/bin/bash

basedir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

reusing_id=joltc-linux-builder
if [ -z "$(docker images -q $reusing_id 2> /dev/null)" ]; then
    docker build -f $basedir/LinuxBuildDockerfile -t $reusing_id .
fi
docker run -it --rm --mount type=bind,src=$basedir/,dst=/app $reusing_id bash

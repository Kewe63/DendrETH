#!/bin/sh
echo "using nilfoundation/zkllvm-template:${ZKLLVM_VERSION:=0.0.86}"

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
CURRENT_DIR=$(pwd)
COMMAND='for v in $(find -iname "*.cpp" -o -iname "*.hpp" -o -iname "*.h" -o -iname "*.c" | grep -v json.hpp); do echo "applying format to" $v; clang-format -i $v; done'

docker run --rm -it --name code_formatter \
       --volume ${SCRIPT_DIR}/../src:/src \
       --user $(id -u ${USER}):$(id -g ${USER}) \
       -w /src \
       ghcr.io/nilfoundation/zkllvm-template:${ZKLLVM_VERSION} \
       /bin/bash -c "$COMMAND"

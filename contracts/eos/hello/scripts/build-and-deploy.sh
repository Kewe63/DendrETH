#!/usr/bin/env bash

DENDRETH_DIR=$(git rev-parse --show-toplevel)
DENDRETH_ACCOUNT_IN_EOS="dendreth"
CONTRACTS_DIR=${DENDRETH_DIR}/contracts/eos/
HELLO_CONTRACT_DIR=${CONTRACTS_DIR}/hello
HELLO_CONTRACT_BUILD_DIR=${HELLO_CONTRACT_DIR}/build

NIM_FILE=${HELLO_CONTRACT_DIR}/src/nim/hello.nim
CPP_FILE=${HELLO_CONTRACT_DIR}/src/cpp/hello.cpp
WASM_CONTRACT=${HELLO_CONTRACT_BUILD_DIR}/hello.wasm
ABI_FILE=${HELLO_CONTRACT_BUILD_DIR}/hello.abi

# ------------------------------------------------------------
# Build the contract
# ------------------------------------------------------------

# Compile the nim files
echo ""
echo "* Compiling the nim files..."
nim-wasm c \
  --lib:${LOCAL_NIM_LIB} \
  --nimcache:${HELLO_CONTRACT_BUILD_DIR} \
  -o:${HELLO_CONTRACT_BUILD_DIR}/nim-hello.wasm \
  ${NIM_FILE}

# Get the list of compiled to c nim files
NIM_COMPILED_OBJECT_FILES=$(jq -r '.link' ${HELLO_CONTRACT_BUILD_DIR}/nim-hello.json | jq -r '.[]')
for NIM_COMPILED_OBJECT_FILE in ${NIM_COMPILED_OBJECT_FILES}
do
  NIM_COMPILED_C_FILES+=$(echo ${NIM_COMPILED_OBJECT_FILE} | rev | cut -c 3- | rev)
  NIM_COMPILED_C_FILES+=$(echo " ")
done

# Compile the whole contract
echo ""
echo "* Compiling the whole contract..."
cdt-cpp \
  -abigen \
  -o ${WASM_CONTRACT} \
  ${CPP_FILE} \
  ${NIM_COMPILED_C_FILES} \
  -I${LOCAL_NIM_LIB}

echo "Contract compiled successfully!"
echo "WASM contract: ${WASM_CONTRACT}"
echo "ABI file: ${ABI_FILE}"

# ------------------------------------------------------------
# Deploy the contract
# ------------------------------------------------------------

echo ""
echo "* Deploying the contract..."
cleos set contract ${DENDRETH_ACCOUNT_IN_EOS} ${CONTRACTS_DIR} ${WASM_CONTRACT} ${ABI_FILE} -p ${DENDRETH_ACCOUNT_IN_EOS}@active
[ $? -eq 0 ] && echo "Contract deployed successfully!" || { echo "Contract deployment failed!"; exit 1; }

echo ""
echo "* Executing the contract..."
cleos push action ${DENDRETH_ACCOUNT_IN_EOS} hi '["dendreth"]' -p ${DENDRETH_ACCOUNT_IN_EOS}@active

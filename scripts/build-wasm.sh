#!/bin/sh

PROJECT_DIR=$1
BUILD_DIR=$2
INSTALL_DIR=$3

source ${EMSDK}/emsdk_env.sh


cmake \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DBUILD_WASM=ON \
  -DCMAKE_MAKE_PROGRAM=ninja \
  -DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -G Ninja \
  -S $PROJECT_DIR \
  -B $BUILD_DIR

cmake --build ${BUILD_DIR} --target core -j 6

cp ${BUILD_DIR}/core.js ${INSTALL_DIR}
cp ${BUILD_DIR}/core.d.ts ${INSTALL_DIR}
cp ${BUILD_DIR}/core.wasm ${INSTALL_DIR}
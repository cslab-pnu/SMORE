#!/bin/bash
set -e

# build llvm
cd artifact/SMORE-LLVM
mkdir -p build

cmake -S llvm -B build -G Ninja \
  -DLLVM_ENABLE_PROJECTS="clang" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="ARM;X86" \
  -DBUILD_SHARED_LIBS=ON \
  -DCLANG_ENABLE_OPAQUE_POINTERS=OFF \
  -DCMAKE_INSTALL_PREFIX=$PWD/install

cmake --build build --target install -j$(nproc)

export PATH=$PWD/install/bin:$PATH
cd ../..


# build newlib-nano
cd artifact/newlib-nano-1.0

chmod +x configure
./configure \
  --prefix=$PWD/install \
  --target=arm-none-eabi \
  CC_FOR_TARGET=clang \
  AR_FOR_TARGET=llvm-ar \
  RANLIB_FOR_TARGET=llvm-ranlib \
  LD_FOR_TARGET=ld.lld \
  CFLAGS_FOR_TARGET="--target=arm-none-eabi -mcpu=cortex-m33 -mthumb -mfloat-abi=soft -mfpu=fpv5-sp-d16 -mllvm -enable-arm-insert-redzone -O3 -g -v -fshort-enums" \
  LDFLAGS_FOR_TARGET="-Wl,--trace-symbol -mcpu=cortex-m33 -mlittle-endian -mthumb -mfloat-abi=soft -mfpu=fpv5-sp-d16 --verbose"

make -j$(nproc)
make install
mv install/arm-none-eabi/lib/libc.a install/arm-none-eabi/lib/libc_nano.a
cd ../..


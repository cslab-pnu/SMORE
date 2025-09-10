#!/bin/bash
PATH=$PWD/artifact/SMORE-LLVM/install/bin:$PATH
export newlib_path=$PWD/artifact/newlib-nano-1.0/install/arm-none-eabi
export newlib_include_dir=$newlib_path/include
export newlib_lib_dir=$newlib_path/lib

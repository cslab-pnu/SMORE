APPLICATION = wasm-example
# If no BOARD is defined in the environment, use this default:
BOARD ?= nucleo-l552ze-q

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

USEPKG += wamr

ifneq ($(BOARD),native)
CFLAGS += -DTHREAD_STACKSIZE_MAIN='(6 * 1024)'
endif

export WAMR_CONFIG := $(abspath config.cmake)

BLOBS += test.wasm hello.wasm

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

TOOLCHAIN = llvm
WERROR = 0

CFLAGS += -mllvm -enable-arm-insert-redzone -DSMORE_EXT -DMODULE_MALLOC_TRACING
NEWLIB_INCLUDE_DIR = /home/yunju/CSLAB/SMORE/Ours/newlib-mem/arm-none-eabi/include
LINKFLAGS += -L/home/yunju/CSLAB/SMORE/Ours/newlib-mem/arm-none-eabi/lib -lc_nano -Wl,--allow-multiple-definition

CONTINUE_ON_EXPECTED_ERRORS=1

all: hello.wasm

hello.wasm: wasm_sample/hello.c wasm_sample/Makefile
	make -C wasm_sample hello.wasm
	mv wasm_sample/hello.wasm .

#######################################################
# Load the rest of the usual RIOT make infrastructure #
#######################################################

include $(RIOTBASE)/Makefile.include


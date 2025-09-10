APPLICATION = lua_basic

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-l552ze-q

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

TOOLCHAIN = llvm
WERROR = 0

CFLAGS += -mllvm -enable-arm-insert-redzone
NEWLIB_INCLUDE_DIR = /home/yunju/CSLAB/RedCanary/Ours/newlib-fpu/arm-none-eabi/include
LINKFLAGS += -L/home/yunju/CSLAB/RedCanary/Ours/newlib-fpu/arm-none-eabi/lib -lc_nano -Wl,--allow-multiple-definition

CONTINUE_ON_EXPECTED_ERRORS=1

ifneq ($(BOARD),native)
  # This stack size is large enough to run Lua print() functions of
  # various lengths. Other functions untested.
  CFLAGS += -DTHREAD_STACKSIZE_MAIN='(THREAD_STACKSIZE_DEFAULT+2048)'
endif

USEPKG += lua

# generate .lua.h header files of .lua files
BLOBS += $(wildcard *.lua)

include $(RIOTBASE)/Makefile.include

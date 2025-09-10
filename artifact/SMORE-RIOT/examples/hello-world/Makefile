# name of your application
APPLICATION = hello-world

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-l552ze-q

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 0

TOOLCHAIN = llvm
WERROR = 0

CFLAGS += -mllvm -enable-arm-insert-redzone -mllvm -enable-insert-redzone-opt
NEWLIB_INCLUDE_DIR = $newlib_include_dir
LINKFLAGS += -L$newlib_lib_dir -lc_nano -Wl,--allow-multiple-definition

include $(RIOTBASE)/Makefile.include

CFLAGS += -O3


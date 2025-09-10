# name of your application
APPLICATION = blinky

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

# Use a peripheral timer for the delay, if available
FEATURES_OPTIONAL += periph_timer

TOOLCHAIN=llvm
WERROR=0

CFLAGS += -mllvm -enable-arm-insert-redzone -DMODULE_MALLOC_TRACING
NEWLIB_INCLUDE_DIR = /home/yunju/CSLAB/SMORE/Ours/newlib-mem/arm-none-eabi/include
LINKFLAGS += -L/home/yunju/CSLAB/SMORE/Ours/newlib-mem/arm-none-eabi/lib -lc_nano -Wl,--allow-multiple-definition

include $(RIOTBASE)/Makefile.include

CFLAGS += -O3

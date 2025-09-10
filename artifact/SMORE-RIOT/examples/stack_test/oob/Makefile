# name of your application
APPLICATION = aha-compress

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-l552ze-q

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 0

TOOLCHAIN = llvm
WERROR = 0 

CFLAGS += -v -O1 -DLOG_LEVEL=LOG_ALL 
CFLAGS += -mllvm -enable-arm-insert-redzone -mllvm -enable-arm-silhouette-str2strt
NEWLIB_INCLUDE_DIR = /home/yunju/CSLAB/boobytrap-call/newlib-fp/arm-none-eabi/include
LINKFLAGS += -L/home/yunju/CSLAB/boobytrap-call/newlib-fp/arm-none-eabi/lib -lc_fp -Wl,--allow-multiple-definition

include $(RIOTBASE)/Makefile.include

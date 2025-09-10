# name of your application
APPLICATION = asymcute_mqttsn

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-l552ze-q 

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_ipv6_default
# Include MQTT-SN
USEMODULE += asymcute
# Add also the shell, some shell commands
USEMODULE += shell_cmds_default
USEMODULE += ps
# For testing we also include the ping command and some stats
USEMODULE += gnrc_icmpv6_echo

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

TOOLCHAIN = llvm
WERROR = 0

CFLAGS += -mllvm -enable-arm-insert-redzone 

NEWLIB_INCLUDE_DIR = /home/yunju/CSLAB/newlib-test/using-newlib/arm-none-eabi/include
LINKFLAGS += -L/home/yunju/CSLAB/newlib-test/using-newlib/arm-none-eabi/lib -lc_test -Wl,--allow-multiple-definition

include $(RIOTBASE)/Makefile.include

APPLICATION = testasync

BOARD ?= native


# NOTE: This test uses an experimental branch.
# NOTE: ../install.sh must be called at least one time.
RIOTBASE ?= $(CURDIR)/../../betas/async


# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_ipv6_default
# Add a routing protocol (optional)
USEMODULE += gnrc_rpl
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo

USEMODULE += gnrc_sock_udp

# NOTE: Enable/disable next line for for asynchronous communication
USEMODULE += gnrc_sock_async

# NOTE: Temporary while sock_asyn is in beta (this called by gnrc_sock_async)
USEMODULE += core_thread_flags

# NOTE: This app is making use of timeout (probably requires work)
USEMODULE += event_timeout

# Add also the shell and commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Set a radio custom channel if needed
ifneq (,$(filter cc110x,$(USEMODULE)))          # radio is cc110x sub-GHz
  DEFAULT_CHANNEL ?= 0
  CFLAGS += -DCC110X_DEFAULT_CHANNEL=$(DEFAULT_CHANNEL)
else
  ifneq (,$(filter at86rf212b,$(USEMODULE)))    # radio is IEEE 802.15.4 sub-GHz
    DEFAULT_CHANNEL ?= 5
    FLAGS += -DIEEE802154_DEFAULT_SUBGHZ_CHANNEL=$(DEFAULT_CHANNEL)
  else                                          # radio is IEEE 802.15.4 2.4 GHz
    DEFAULT_CHANNEL ?= 26
    CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=$(DEFAULT_CHANNEL)
  endif
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# NOTE: ifneq is unique to this repository
ifneq (,$(wildcard $(RIOTBASE)/Makefile.include))
include $(RIOTBASE)/Makefile.include
else
$(error This requires a beta branch. Plase, run: "install.sh from the top")
endif

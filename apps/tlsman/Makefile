APPLICATION = tlsman_simple

BOARD ?= native

# NOTE: This test uses an experimental branch.
# NOTE: ../install.sh must be called at least one time.
RIOTBASE ?= $(CURDIR)/../../betas/tlsman

# Minimun Network setting
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default

# Minimun Shell commands settings
USEMODULE += shell

# Optional modules
USEMODULE += gnrc_icmpv6_echo
USEMODULE += shell_commands
USEMODULE += ps

# Relevant modules for TLSMAN
USEMODULE += tlsman
USEMODULE += gnrc_sock_udp

# Special include directory with the (D)TLS keys required for the test
INCLUDES += -I$(APPDIR)/keys

# This is intented to be handled by sock_secure for normal applications
# NOTE (TODO): Should be equivalent to use `USEMODULE += tlsman_tinydtls`
USEPKG += tinydtls
ifneq (,$(filter tinydtls,$(USEPKG)))
	USEMODULE += tlsman_tinydtls

	CFLAGS += -DTHREAD_STACKSIZE_MAIN=\(3*THREAD_STACKSIZE_DEFAULT\)
	# NOTE: For "rehandhsake", tinydtls requires at least two peers.
	# With the default values, the boards will not crash but tinydtls will
	# be unable to generate the context.
	CFLAGS += -DDTLS_CONTEXT_MAX=2
	CFLAGS += -DDTLS_PEER_MAX=2
	CFLAGS += -DDTLS_HANDSHAKE_MAX=2

	ifneq (,$(filter gnrc_sock_async,$(USEMODULE)))
		# NOTE: Temporary while sock_asyn is in beta (this called by gnrc_sock_async)
		USEMODULE += core_thread_flags

		# NOTE: At least for tinyDTLS, we are making use of event timeout
		USEMODULE += event_timeout
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

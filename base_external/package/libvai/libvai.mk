# libvai.mk - Buildroot package makefile for the libvai library and example app

LIBVAI_VERSION = 1.0

# define the project root relative to Buildroot's top directory
PROJECT_ROOT = $(BR2_EXTERNAL)/..

# location of the library source code
LIBVAI_SITE = $(PROJECT_ROOT)/src/lib
LIBVAI_SITE_METHOD = local

# dependencies
LIBVAI_DEPENDENCIES =

# license
LIBVAI_LICENSE = GPLv2

# custom make options to pass include directory
LIBVAI_MAKE_OPTS = INCLUDES+="-I$(PROJECT_ROOT)/include"

# specify the targets to build
LIBVAI_TARGETS = libvai.so example

define LIBVAI_BUILD_CMDS
	$(MAKE) -C $(@D) $(LIBVAI_MAKE_OPTS) all
endef

define LIBVAI_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libvai.so $(TARGET_DIR)/usr/lib/libvai.so
	$(INSTALL) -D -m 0755 $(@D)/example $(TARGET_DIR)/usr/bin/example
endef

$(eval $(generic-package))

################################################################################
#
# gameplayos-core
#
################################################################################

GAMEPLAYOS_CORE_VERSION = 1.0
GAMEPLAYOS_CORE_SITE = $(BR2_EXTERNAL_GAMEPLAYOS_PATH)/package/gameplayos-core/src
GAMEPLAYOS_CORE_SITE_METHOD = local
GAMEPLAYOS_CORE_LICENSE = MIT

# Build only the two native GameplayOS processes; no command interpreter.
define GAMEPLAYOS_CORE_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -o $(@D)/gameplayos-init $(@D)/init.c
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -o $(@D)/gameplayos-ui $(@D)/ui.c
endef

define GAMEPLAYOS_CORE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/gameplayos-init $(TARGET_DIR)/sbin/init
	$(INSTALL) -D -m 0755 $(@D)/gameplayos-ui $(TARGET_DIR)/usr/bin/gameplayos-ui
endef

$(eval $(generic-package))

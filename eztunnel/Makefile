
include $(TOPDIR)/rules.mk

# Name, version and release number
PKG_NAME:=eztunnel
PKG_VERSION:=1.0
PKG_RELEASE:=1

# Source settings
SRC_DIR:=./eztunnel
SOURCE_FILE:=eztunnel.c

# This package's directory
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

# Package definition; instructs on how to build the package
define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Easy tunnel
  DEPENDS:=+libpthread
endef

define Package/$(PKG_NAME)/description
  Easy and not so secure tunnel over UDP
endef

# Package build instructions; invoke the build system to compile the package
define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/eztunnel $(SRC_DIR)/$(SOURCE_FILE) $(TARGET_LDFLAGS)
endef

# Package install instructions; create the ipk in the bin/packages directory
define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/eztunnel $(1)/usr/bin/
endef

# This line executes the defined instructions
$(eval $(call BuildPackage,eztunnel))

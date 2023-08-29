
include $(TOPDIR)/rules.mk

PKG_NAME:=eztun
PKG_VERSION:=0000000
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/eztun
  SECTION:=Development
  DEPENDS:=+kmod-tun
  TITLE:=Easy and not very secure UDP tunneling daemon
  FILES:=$(PKG_BUILD_DIR)/eztun/eztun
endef


define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) -r ./* $(PKG_BUILD_DIR)/
endef


MAKE_PKG := $(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		PATH="$(TARGET_PATH)" \

define Build/Compile
	$(MAKE_PKG) \
		SUBDIRS="$(PKG_BUILD_DIR)/modules" \
		all
	pwd
endef

define Package/eztun/install
	$(CP) -r ./files/* $(1)/
endef

$(eval $(call BuildPackage,eztun))




obj-$(CONFIG_PHY_R8A779F0_ETHERNET_SERDES)	+= r8a779f0-ether-serdes.o
obj-$(CONFIG_PHY_R8A78000_ETHERNET_PCS)		+= r8a78000-ether-pcs.o
obj-$(CONFIG_PHY_R8A78000_MP_PHY)		+= r8a78000-mp-phy.o

ifneq ($(CONFIG_PHY_R8A78000_MP_PHY_FW),"")
obj-$(CONFIG_PHY_R8A78000_MP_PHY)		+= mp_phy_fw_obj.o
$(obj)/mp_phy_fw_obj.o: $(shell cd $(srctree); readlink -f $(CONFIG_PHY_R8A78000_MP_PHY_FW)) FORCE
endif

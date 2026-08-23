KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(PWD)
#EXTRA_SYMBOLS_USB_PATH ?= $(M)/../../../../../aml-5.15/wifi/amlogic/drivers/w2l/aml_drv/Module.symvers
EXTRA_SYMBOLS_USB_PATH ?= $(M)/../../../../wifi/amlogic/drivers/w2l/aml_drv/Module.symvers

###############################################################################
# AMLBT
###############################################################################

EXTRA_CFLAGS += -DCONFIG_AML_BT_CHIP_W2L

AMLBT_MOD_NAME = w2l_bt

AMLBT_CFILES := \
    common.c \
    rc_list.c \
    debug_dev.c \
    amlbt_utils.c \
    chip.c \
    intf.c \
    intf_sdio.c \
    intf_usb.c \
    intf_uart.c \
    intf_uart_coex.c \
    intf_pcie.c \
    driver.c

$(AMLBT_MOD_NAME)-objs := $(AMLBT_CFILES:.c=.o)

$(warning KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS))
$(warning M=$(M))

###############################################################################
# Common
###############################################################################
obj-m += $(AMLBT_MOD_NAME).o

EXTRA_CFLAGS += -Wno-pointer-to-int-cast -Wno-format -Wno-unused-function
EXTRA_CFLAGS += -Wno-unused-variable -Wno-return-type -Wno-implicit-fallthrough
EXTRA_CFLAGS += -Wno-sizeof-pointer-memaccess -D_15P4_SEPARATION
EXTRA_CFLAGS += -Wno-incompatible-function-pointer-types -Wno-strict-prototypes

all:
	make -C $(KERNEL_SRC) M=$(M) KBUILD_EXTRA_SYMBOLS=$(EXTRA_SYMBOLS_USB_PATH) modules
modules_install:
	@$(MAKE) INSTALL_MOD_STRIP=1 M=$(M) -C $(KERNEL_SRC) modules_install
	mkdir -p ${OUT_DIR}/../vendor_lib/modules
	cd ${OUT_DIR}/$(M)/; find -name "*.ko" -exec cp {} ${OUT_DIR}/../vendor_lib/modules/ \;

usb:
	make -C $(KERNEL_SRC) M=$(PWD) $(AMLBT_MOD_NAME).ko

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean




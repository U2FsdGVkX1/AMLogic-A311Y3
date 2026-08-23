cp -a "$BOARD_DIR/overlay/"* "$ROOTFS/"

echo 'GRUB_CMDLINE_LINUX="console=ttyS0,921600 earlycon=aml_uart,0xffa1e000"' >> "$ROOTFS/etc/default/grub"
echo "GRUB_DEVICETREE=amlogic/a9_a311y3_by401_linux.dtb" >> "$ROOTFS/etc/default/grub"

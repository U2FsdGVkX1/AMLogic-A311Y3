from boards.common import KERNEL_INSTALL, default_images, default_partitions, firmware
from hox import Board, ConfigContext


def configure(ctx: ConfigContext) -> Board:
    arch = "aarch64"
    fw = firmware(arch)
    kernel = fw.kernel(
        "git@github.com:U2FsdGVkX1/AMLogic-A311Y3_kernel.git",
        "a9_a311y3_by401_defconfig",
        install=[KERNEL_INSTALL, ctx.BOARD_DIR / "install.sh"],
    )
    modules = ctx.adapter + [
        kernel,
        ctx.module("adla", kernel.env),
        ctx.module("w2l", kernel.env),
        ctx.module("uboot"),
    ]
    return Board(
        arch=arch,
        partitions=default_partitions(),
        images=default_images(),
        modules=modules,
    )

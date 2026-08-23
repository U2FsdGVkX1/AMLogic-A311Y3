from pathlib import Path

from actions import cp, git, shell
from hox import ConfigContext, Module

ARTIFACTS = (
    "u-boot.bin.signed",
    "u-boot.bin.sd.bin.signed",
    "u-boot.bin.usb.signed",
    "a9_by401-u-boot.aml.zip",
)


def configure(ctx: ConfigContext) -> Module:
    return Module(
        name="uboot",
        requires=[
            "git",
            "make",
            "zip",
            "python3",
            "openssl",
            "xxd",
            "bison",
            "flex",
            "aarch64-linux-gnu-gcc",
        ],
        env={
            "CROSS_COMPILE": "aarch64-linux-gnu-",
            "ARCH": "arm",
            "CONFIG_BYPASS_AOCPU": "y",
            "KCFLAGS": "-DCONFIG_YOCTO",
        },
        build=[
            git(src="git@github.com:U2FsdGVkX1/AMLogic-A311Y3_uboot.git", path=Path(".")),
            shell(
                cmds=[
                    "env -u SOURCE_DATE_EPOCH LDFLAGS= ./mk a9_by401 "
                    "-fastboot-write "
                    "--bl30 bl30/bin_ao/a9/a311y3/bl30.bin"
                ]
            ),
            *[cp(src=Path("build") / name, dest=Path(".")) for name in ARTIFACTS],
        ],
        install=[
            f'cp -f "$MODULE_DIR/{name}" "$OUTPUT_DIR"' for name in ARTIFACTS
        ],
    )

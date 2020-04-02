Prerequisites & Requirements
============================

This document describes the software and hardware requirents for building TF-A Tests for AArch32 and AArch64 target platforms.

It may be possible to build TF-A Tests with combinations of software and hardware that are different from those listed below. The software and hardware described in this document are officially supported.

Build Host
-------------------------

TF-A Tests may be built using either a Linux or a Windows build host machine.

The minimum recommended machine specification for building the software and
running the `FVP models`_ is a dual-core processor running at 2GHz with 12GB of
RAM. For best performance, use a machine with a quad-core processor running at
2.6GHz with 16GB of RAM.

The software has been tested on Ubuntu 16.04 LTS (64-bit). Packages used for
building the software were installed from that distribution unless otherwise
specified.

Toolchain
---------

Install the required packages to build TF-A Tests with the following command:

::

    sudo apt-get install device-tree-compiler build-essential make git perl libxml-libxml-perl

Download and install the GNU cross-toolchain from Linaro. The TF-A Tests have
been tested with version 9.2-2019.12 (gcc 9.2):

-  `AArch32 GNU cross-toolchain`_
-  `AArch64 GNU cross-toolchain`_

In addition, the following optional packages and tools may be needed:

-   For debugging, Arm `Development Studio 5 (DS-5)`_.

.. _FVP models: https://developer.arm.com/products/system-design/fixed-virtual-platforms
.. _AArch32 GNU cross-toolchain: https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-mingw-w64-i686-arm-none-eabi.tar.xz?revision=11ead78b-0442-489c-bd97-f18db1866f30&la=en&hash=5BCB8EE533636CF34FF99AB46CCB60CE11E4F1E1
.. _AArch64 GNU cross-toolchain: https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-mingw-w64-i686-aarch64-none-elf.tar.xz?revision=09ff6958-8d84-4694-a204-6413888aee5d&la=en&hash=D6ED846108EAD952460872D5F8BD15290FF3D50D
.. _Development Studio 5 (DS-5): https://developer.arm.com/products/software-development-tools/ds-5-development-studio

--------------

*Copyright (c) 2019, Arm Limited. All rights reserved.*

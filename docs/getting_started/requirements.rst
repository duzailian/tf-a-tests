Prerequisites & Requirements
============================

Host machine requirements
-------------------------

The minimum recommended machine specification for building the software and
running the `FVP models`_ is a dual-core processor running at 2GHz with 12GB of
RAM. For best performance, use a machine with a quad-core processor running at
2.6GHz with 16GB of RAM.

The software has been tested on Ubuntu 16.04 LTS (64-bit). Packages used for
building the software were installed from that distribution unless otherwise
specified.

Tools
-----

Install the required packages to build TF-A Tests with the following command:

::

    sudo apt-get install device-tree-compiler build-essential make git perl libxml-libxml-perl

Download and install the GNU cross-toolchain from Linaro. The TF-A Tests have
been tested with version 6.2-2016.11 (gcc 6.2):

-  `AArch32 GNU cross-toolchain`_
-  `AArch64 GNU cross-toolchain`_

In addition, the following optional packages and tools may be needed:

-   For debugging, Arm `Development Studio 5 (DS-5)`_.

.. _FVP models: https://developer.arm.com/products/system-design/fixed-virtual-platforms
.. _AArch32 GNU cross-toolchain: http://releases.linaro.org/components/toolchain/binaries/6.2-2016.11/arm-linux-gnueabihf/gcc-linaro-6.2.1-2016.11-x86_64_arm-linux-gnueabihf.tar.xz
.. _AArch64 GNU cross-toolchain: http://releases.linaro.org/components/toolchain/binaries/6.2-2016.11/aarch64-linux-gnu/gcc-linaro-6.2.1-2016.11-x86_64_aarch64-linux-gnu.tar.xz
.. _Development Studio 5 (DS-5): https://developer.arm.com/products/software-development-tools/ds-5-development-studio

--------------

*Copyright (c) 2019, Arm Limited. All rights reserved.*

#
# Copyright (c) 2018, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

CACTUS_DTB		:= $(BUILD_PLAT)/cactus/cactus.dtb
IVY_DTB			:= $(BUILD_PLAT)/ivy/ivy.dtb
SPTOOL			:= build/sptool

$(CACTUS_DTB) : cactus
	@echo "  DTBGEN  spm/cactus/cactus.dts"
	${Q}bash tools/generate_dtb/generate_dtb.sh \
		cactus spm/cactus/cactus.dts $(BUILD_PLAT)
	@echo
	@echo "Built $@ successfully"
	@echo

$(IVY_DTB) : ivy
	@echo "  DTBGEN  spm/ivy/ivy.dts"
	${Q}bash tools/generate_dtb/generate_dtb.sh \
		ivy spm/ivy/ivy.dts $(BUILD_PLAT)
	@echo
	@echo "Built $@ successfully"
	@echo

$(SPTOOL):
	${Q}make -C $(SPTOOL_PATH)
	${Q}mv $(SPTOOL_PATH)/sptool $(SPTOOL)
	@echo
	@echo "Built $@ successfully"
	@echo

sp_package : $(CACTUS_DTB) $(IVY_DTB) build/sptool
	${Q}$(SPTOOL)								\
		-o $(BUILD_PLAT)/sp_package.bin					\
		-i $(BUILD_PLAT)/cactus.bin:$(BUILD_PLAT)/cactus/cactus.dtb	\
		-i $(BUILD_PLAT)/ivy.bin:$(BUILD_PLAT)/ivy/ivy.dtb
	@echo
	@echo "Built $@ successfully"
	@echo

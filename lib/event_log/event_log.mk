#
# Copyright (c) 2020-2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

LIBEVLOG_PATH	?= contrib/libeventlog

EVENT_LOG_INCLUDES		+=	-I$(LIBEVLOG_PATH)/include \
							-I$(LIBEVLOG_PATH)/include/private


EVENT_LOG_SOURCES	:=	$(LIBEVLOG_PATH)/src/event_print.c \
						$(LIBEVLOG_PATH)/src/digest.c

ifdef CRYPTO_SUPPORT
# Measured Boot hash algorithm.
# SHA-256 (or stronger) is required for all devices that are TPM 2.0 compliant.
ifdef TPM_HASH_ALG
    $(warning "TPM_HASH_ALG is deprecated. Please use MBOOT_EL_HASH_ALG instead.")
    MBOOT_EL_HASH_ALG		:=	${TPM_HASH_ALG}
else
    MBOOT_EL_HASH_ALG		:=	sha256
endif

ifeq (${MBOOT_EL_HASH_ALG}, sha512)
    TPM_ALG_ID			:=	TPM_ALG_SHA512
    TCG_DIGEST_SIZE		:=	64U
else ifeq (${MBOOT_EL_HASH_ALG}, sha384)
    TPM_ALG_ID			:=	TPM_ALG_SHA384
    TCG_DIGEST_SIZE		:=	48U
else
    TPM_ALG_ID			:=	TPM_ALG_SHA256
    TCG_DIGEST_SIZE		:=	32U
endif #MBOOT_EL_HASH_ALG

# Set definitions for event log library.
$(eval $(call add_define,TFTF_DEFINES,TPM_ALG_ID))
$(eval $(call add_define,TFTF_DEFINES,EVENT_LOG_LEVEL))
$(eval $(call add_define,TFTF_DEFINES,TCG_DIGEST_SIZE))

endif

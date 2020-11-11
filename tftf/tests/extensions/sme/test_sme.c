/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <arch_features.h>
#include <arch_helpers.h>
#include <lib/extensions/sme.h>
#include <tftf_lib.h>

test_result_t test_sme_support(void)
{
    u_register_t reg;
    unsigned int current_vector_len;
    unsigned int requested_vector_len;
    unsigned int len_max;

    /* Skip the test if SME is not supported. */
    printf("Checking for SME support...");
    if (!feat_sme_supported()) {
        printf("not present\n");
        return TEST_RESULT_SKIPPED;
    }
    printf("present\n");

    /* Enable SME for use at NS EL2. */
    printf("Enabling SME for use at NS EL2...");
    if (sme_enable() != 0) {
        printf("failed\n");
        return TEST_RESULT_FAIL;
    }
    printf("done\n");

    /* Make sure TPIDR2_EL0 is accessible. */
    printf("Checking that TPIDR2_EL0 is accessible...");
    write_tpidr2_el0(0);
    if (read_tpidr2_el0() != 0) {
        printf("failed\n");
        return TEST_RESULT_FAIL;
    }
    write_tpidr2_el0(0xb0bafe77);
    if (read_tpidr2_el0() != 0xb0bafe77) {
        printf("failed\n");
        return TEST_RESULT_FAIL;
    }
    printf("success\n");

    /* Make sure we can start and stop streaming mode. */
    printf("Checking that we can enter streaming mode...");
    sme_smstart(false);
    read_smcr_el2();
    sme_smstop(false);
    sme_smstart(true);
    read_smcr_el2();
    sme_smstop(true);
    printf("success\n");

    /*
     * Iterate through values for LEN to detect supported vector lengths.
     * SME instructions aren't supported by GCC yet so for now this is all
     * we'll do.
     */
    printf("Enumerating supported vector sizes...\n");
    sme_smstart(false);

    /* Write SMCR_EL2 with the LEN max to find implemented width. */
    write_smcr_el2(SME_SMCR_LEN_MAX);
    len_max = (unsigned int)read_smcr_el2();
    printf("  Maximum SMCR_EL2.LEN value: 0x%x\n", len_max);

    for (unsigned int i = 0; i <= len_max; i++) {
        /* Load new value into SMCR_EL2.LEN */
        reg = read_smcr_el2();
        reg &= ~(SMCR_ELX_LEN_MASK << SMCR_ELX_LEN_SHIFT);
        reg |= (i << SMCR_ELX_LEN_SHIFT);
        write_smcr_el2(reg);

        /* Compute current and requested vector lengths in bits. */
        current_vector_len = ((unsigned int)sme_rdvl_1() * 8U);
        requested_vector_len = (i+1U)*128U;

        /*
         * We count down from the maximum SMLEN value, so if the values
         * match, we've found the largest supported value for SMLEN.
         */
        if (current_vector_len == requested_vector_len) {
            printf("  SUPPORTED:     ");
        } else {
            printf("  NOT SUPPORTED: ");
        }
        printf("%u bits (LEN=%u)\n", requested_vector_len, i);
    }
    sme_smstop(false);

    /* If FEAT_SME_FA64 then attempt to execute an illegal instruction. */
    printf("Checking for FEAT_SME_FA64 support...");
    if (feat_sme_fa64_supported()) {
        printf("present\n");
        sme_execute_illegal_instruction();
        printf("  Illegal instruction executed successfully.\n");
    } else {
        printf("not present\n");
    }

    return TEST_RESULT_SUCCESS;
}

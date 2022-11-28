/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arch_helpers.h>
#include <debug.h>
#include <errata_abi.h>
#include <platform.h>
#include <psci.h>
#include <smccc.h>
#include <tftf_lib.h>

/* Forward flag */
#define FORWARD_FLAG_EL1	0x00

/* Global pointer to point to individual cpu structs based on midr value */
em_cpu_t *cpu_ptr;

/* Errata list for cpu's */
em_cpu_t cortex_A710_errata_list = {
	.cpu_pn = 0xD47,
	.cpu_errata = {
		{2701952, 0x00, 0x21, 0x00},
		{2147715, 0x20, 0x20, 0x21},
		{2058056, 0x00, 0x10, 0x00},
		{2371105, 0x00, 0x20, 0x21},
		{2267065, 0x00, 0x20, 0x21}
	},
};

em_cpu_t cortex_A57_errata_list = {
	.cpu_pn = 0xD07,
	.cpu_errata = {
		{826974, 0x00, 0x11, 0x00},
		{1319537, 0x00, 0xFF, 0x00},
		{859972, 0x00, 0x13, 0x00},
		{817169, 0x00, 0x01, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A715_errata_list = {
	.cpu_pn = 0xD4D,
	.cpu_errata = {
		{2701951, 0x00, 0x11, 0x12},
		{1235679, 0x00, 0x00, 0x00},
		{1234567, 0x00, 0x01, 0x02},
		{-1}
	},
};

em_cpu_t cortex_A72_errata_list = {
	.cpu_pn = 0xD08,
	.cpu_errata = {
		{859971, 0x00, 0x03, 0x00},
		{1319367, 0x00, 0xFF, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A73_errata_list = {
	.cpu_pn = 0xD09,
	.cpu_errata = {
		{852427, 0x00, 0x00, 0x00},
		{855423, 0x00, 0x01, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A76_errata_list = {
	.cpu_pn = 0xD0B,
	.cpu_errata = {
		{1946160, 0x30, 0x41, 0x00},
		{1791580, 0x00, 0x40, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A510_errata_list = {
	.cpu_pn = 0xD46,
	.cpu_errata = {
		{1922240, 0x00, 0x00, 0x01},
		{2041909, 0x02, 0x02, 0x03},
		{2684597, 0x00, 0x12, 0x13},
		{2347730, 0x00, 0x11, 0x12},
		{2666669, 0x00, 0x11, 0x12}
	},
};

em_cpu_t cortex_X2_errata_list = {
	.cpu_pn = 0xD48,
	.cpu_errata = {
		{2002765, 0x00, 0x20, 0x00},
		{2147715, 0x20, 0x20, 0x21},
		{2701952, 0x00, 0x21, 0x00},
		{2768515, 0x00, 0x21, 0x00},
		{2371105, 0x00, 0x21, 0x00}
	},
};

em_cpu_t neoverse_N2_errata_list = {
	.cpu_pn = 0xD49,
	.cpu_errata = {
		{2002655, 0x00, 0x00},
		{2280757, 0x00, 0x00},
		{2728475, 0x00, 0x02, 0x03},
		{2376738, 0x00, 0x00, 0x01},
		{2743089, 0x00, 0x02, 0x03}
	},
};

em_cpu_t neoverse_V1_errata_list = {
	.cpu_pn = 0xD40,
	.cpu_errata = {
		{1966096, 0x10, 0x11, 0x00},
		{2294912, 0x00, 0x11, 0x00},
		{2701953, 0x00, 0x11, 0x12},
		{2779461, 0x00, 0x12, 0x00},
		{2108267, 0x00, 0x11}
	},
};

em_cpu_t cortex_X1_errata_list = {
	.cpu_pn = 0xD44,
	.cpu_errata = {
		{1688305, 0x00, 0x10, 0x00},
		{1821534, 0x00, 0x10, 0x00},
		{1827429, 0x00, 0x10, 0x00},
		{-1}
	},

};

em_cpu_t neoverse_V2_errata_list = {
	.cpu_pn = 0xD4F,
	.cpu_errata = {
		{2719103, 0x00, 0x01, 0x02},
		{1234567, 0x00, 0x00, 0x00},
		{1245678, 0x00, 0x00, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A75_errata_list = {
	.cpu_pn = 0xD0A,
	.cpu_errata = {
		{764081, 0x00, 0x00, 0x00},
		{790748, 0x00, 0x00, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A55_errata_list = {
	.cpu_pn = 0xD05,
	.cpu_errata = {
		{768277, 0x00, 0x00, 0x00},
		{1530923, 0x00, 0xFF, 0x00},
		{1221012, 0x00, 0x10, 0x00},
		{1530923, 0x00, 0xFF, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A78_AE_errata_list = {
	.cpu_pn = 0xD42,
	.cpu_errata = {
		{1941500, 0x00, 0x01, 0x00},
		{2376748, 0x00, 0x01, 0x00},
		{2712574, 0x00, 0x02, 0x00},
		{2376748, 0x00, 0x01, 0x00},
		{1951502, 0x00, 0x01, 0x00}
	},
};

em_cpu_t neoverse_N1_errata_list = {
	.cpu_pn = 0xD0C,
	.cpu_errata = {
		{1073348, 0x00, 0x10, 0x00},
		{1542419, 0x30, 0x40, 0x00},
		{1946160, 0x30, 0x41, 0x00},
		{1868343, 0x00, 0x40, 0x00},
		{1130799, 0x00, 0x20, 0x00}
	},
};

em_cpu_t cortex_A35_errata_list = {
	.cpu_pn = 0xD04,
	.cpu_errata = {
		{855472, 0x00, 0x00, 0x00},
		{1234567, 0x00, 0x00, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A53_errata_list = {
	.cpu_pn = 0xD03,
	.cpu_errata = {
		{835769, 0x00, 0x04, 0x00},
		{836870, 0x00, 0x03, 0x04},
		{1530924, 0x00, 0xFF, 0x00},
		{843419, 0x00, 0x04, 0x00},
		{855873, 0x03, 0xFF, 0x00}
	},
};


em_cpu_t cortex_A77_errata_list = {
	.cpu_pn = 0xD0D,
	.cpu_errata = {
		{1508412, 0x00, 0x10, 0x00},
		{1946167, 0x00, 0x11, 0x00},
		{2356587, 0x00, 0x11, 0x00},
		{2743100, 0x00, 0x11, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A78_errata_list = {
	.cpu_pn = 0xD41,
	.cpu_errata = {
		{2132060, 0x00, 0x12, 0x00},
		{2376745, 0x00, 0x12, 0x00},
		{2712571, 0x00, 0x12, 0x00},
		{-1}
	},
};

em_cpu_t cortex_A78C_errata_list = {
	.cpu_pn = 0xD4B,
	.cpu_errata = {
		{2242638, 0x01, 0x02, 0x00},
		{2376749, 0x01, 0x02, 0x00},
		{2712575, 0x01, 0x02, 0x00},
		{-1}
	},
};

/*
 * Test function checks for the em_version implemented
 * - Test fails if the version returned is < 1.0.
 * - Test passes if the version returned is == 1.0
 */
test_result_t test_em_version(void)
{
	test_result_t return_val;
	int32_t version_return = tftf_em_abi_version();

	if ((version_return == (EM_ABI_VERSION(1, 0)))) {
		return_val = TEST_RESULT_SUCCESS;
	} else {
		return_val = TEST_RESULT_FAIL;
	}
	INFO("test_em_version = %d\n", return_val);
	return return_val;
}

/*
 * Test function checks for the em_features implemented
 * Test fails if the em_feature is not implemented
 * or if the fid is invalid.
 */

test_result_t test_em_features(void)
{
	test_result_t return_val;
	int32_t version_return = tftf_em_abi_version();

	if (version_return == EM_NOT_SUPPORTED) {
		return TEST_RESULT_SKIPPED;
	}

	if (!(tftf_em_abi_feature_implemented(EM_CPU_ERRATUM_FEATURES))) {
		return_val = TEST_RESULT_FAIL;
	} else {
		return_val = TEST_RESULT_SUCCESS;
	}
	INFO("test_em_features = %d\n", return_val);
	return return_val;
}

/*
 * Test function checks for the em_cpu_feature implemented
 * Test fails if the em_cpu_feature is not implemented
 * or if the fid is invalid.
 */
test_result_t test_em_cpu_features(void)
{
	test_result_t return_val = TEST_RESULT_FAIL;
	smc_ret_values ret_val;
	int32_t version_return = tftf_em_abi_version();
	#ifdef __aarch64__
	uint32_t midr_val = read_midr_el1();
	#else
	uint32_t midr_val =  read_midr();
	#endif
	uint16_t rxpx_val_extracted = ((midr_val & MIDR_REV_MASK) << MIDR_REV_BITS) \
			 | ((midr_val >> MIDR_VAR_SHIFT) & MIDR_VAR_MASK);

	INFO("Midr val before extraction = %x and rxpx extracted val = %x\n\n", \
		midr_val, rxpx_val_extracted);
	midr_val = ((midr_val >> MIDR_PN_SHIFT) & MIDR_PN_MASK);

	if (version_return == EM_NOT_SUPPORTED) {
		return_val = TEST_RESULT_SKIPPED;
		return return_val;
	}

	if (!(tftf_em_abi_feature_implemented(EM_CPU_ERRATUM_FEATURES))) {
		return_val = TEST_RESULT_FAIL;
		return return_val;
	}

	switch (midr_val) {
	case 0xD09:
	{
		INFO("midr matches A73 -> %x\n", midr_val);
		cpu_ptr = &cortex_A73_errata_list;
		break;
	}
	case 0xD0B:
	{
		INFO("midr matches A76 -> %x\n", midr_val);
		cpu_ptr = &cortex_A76_errata_list;
		break;
	}
	case 0xD4D:
	{
		INFO("midr matches A715 -> %x\n", midr_val);
		cpu_ptr = &cortex_A715_errata_list;
		break;
	}
	case 0xD04:
	{
		INFO("midr val matches A35 -> %x\n", midr_val);
		cpu_ptr = &cortex_A35_errata_list;
		break;
	}
	case 0xD03:
	{
		INFO("midr val matches A53 = %x\n", midr_val);
		cpu_ptr = &cortex_A53_errata_list;
		break;
	}
	case 0xD07:
	{
		INFO("midr val matches A57 = %x\n", midr_val);
		cpu_ptr = &cortex_A57_errata_list;
		break;
	}
	case 0xD08:
	{
		INFO("midr val matches A72 = %x\n", midr_val);
		cpu_ptr = &cortex_A72_errata_list;
		break;
	}
	case 0xD0D:
	{
		INFO("midr val matches A77 = %x\n", midr_val);
		cpu_ptr = &cortex_A77_errata_list;
		break;
	}
	case 0xD41:
	{
		INFO("midr val matches A78 = %x\n", midr_val);
		cpu_ptr = &cortex_A78_errata_list;
		break;
	}
	case 0xD0C:
	{
		INFO("midr val matches neoverse N1 = %x\n", midr_val);
		cpu_ptr = &neoverse_N1_errata_list;
		break;
	}
	case 0xD4B:
	{
		INFO("midr val matches A78C = %x\n", midr_val);
		cpu_ptr = &cortex_A78C_errata_list;
		break;
	}
	case 0xD4F:
	{
		INFO("midr val matches Demeter -> %x\n", midr_val);
		cpu_ptr = &neoverse_V2_errata_list;
		break;
	}
	case 0xD47:
	{
		INFO("midr val matches A710 -> %x\n", midr_val);
		cpu_ptr = &cortex_A710_errata_list;
		break;
	}
	case 0xD46:
	{
		INFO("midr val matches A510 -> %x\n", midr_val);
		cpu_ptr = &cortex_A510_errata_list;
		break;
	}
	case 0xD48:
	{
		INFO("midr val matches X2 -> %x\n", midr_val);
		cpu_ptr = &cortex_X2_errata_list;
		break;
	}
	case 0xD49:
	{
		INFO("midr val matches N2 -> %x\n", midr_val);
		cpu_ptr = &neoverse_N2_errata_list;
		break;
	}
	case 0xD40:
	{
		INFO("midr val matches V1 -> %x\n", midr_val);
		cpu_ptr = &neoverse_V1_errata_list;
		break;
	}
	case 0xD44:
	{
		INFO("midr val matches X1 > %x\n", midr_val);
		cpu_ptr = &cortex_X1_errata_list;
		break;
	}
	case 0xD0A:
	{
		INFO("midr val matches A75 > %x\n", midr_val);
		cpu_ptr = &cortex_A75_errata_list;
		break;
	}
	case 0xD05:
	{
		INFO("midr val matches A55 > %x\n", midr_val);
		cpu_ptr = &cortex_A55_errata_list;
		break;
	}
	case 0xD42:
	{
		INFO("midr val matches A78_AE > %x\n", midr_val);
		cpu_ptr = &cortex_A78_AE_errata_list;
		 break;
	}
	default:
	{
		INFO("midr value did not match\n");
		break;
	}
	}

	for (int i = 0; i < TOTAL_NUM && cpu_ptr->cpu_errata[i].em_errata_id != -1; i++) {

		ret_val = tftf_em_abi_cpu_feature_implemented \
				(cpu_ptr->cpu_errata[i].em_errata_id, \
				FORWARD_FLAG_EL1);
		switch (ret_val.ret0) {

		case EM_NOT_AFFECTED:
		{
			return_val = (rxpx_val_extracted >= cpu_ptr->cpu_errata[i].hardware_mitigated)  \
					? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
			break;
		}
		case EM_AFFECTED:
		{
			return_val = (IN_RANGE(rxpx_val_extracted, cpu_ptr->cpu_errata[i].rxpx_low, \
					cpu_ptr->cpu_errata[i].rxpx_high)) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
			break;
		}
		case EM_HIGHER_EL_MITIGATION:
		{
			return_val = (IN_RANGE(rxpx_val_extracted, cpu_ptr->cpu_errata[i].rxpx_low, \
					cpu_ptr->cpu_errata[i].rxpx_high)) ? TEST_RESULT_SUCCESS : TEST_RESULT_FAIL;
			break;

		}
		case EM_UNKNOWN_ERRATUM:
		{
			if (IN_RANGE(rxpx_val_extracted, cpu_ptr->cpu_errata[i].rxpx_low, \
				cpu_ptr->cpu_errata[i].rxpx_high)) {

				return_val = TEST_RESULT_FAIL;
				break;
			} else if (rxpx_val_extracted >= cpu_ptr->cpu_errata[i].hardware_mitigated) {
				return_val = TEST_RESULT_FAIL;
				break;
			} else {
				return_val = TEST_RESULT_SUCCESS;
				break;
			}
			break;
		}
		default:
		{
			INFO("Return value did not match the expected returns\n");
			return_val = TEST_RESULT_FAIL;
			break;
		}
		}
		INFO("\n errata_id = %d and test_em_cpu_erratum_features = %ld\n",\
			cpu_ptr->cpu_errata[i].em_errata_id, ret_val.ret0);
	}

	return return_val;
}

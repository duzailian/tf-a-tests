#ifndef tbb_test_infra_h_INCLUDED
#define tbb_test_infra_h_INCLUDED

test_result_t corrupt_boot_fip_test(unsigned int offset);

#define test_assert(must_be_true) \
	if (!(must_be_true)) { \
		tftf_testcase_printf("Failed at %s:%d", __FILE__, __LINE__); \
		return TEST_RESULT_FAIL;\
	}

#define test_assert_skip(must_be_true) \
	if (!(must_be_true)) { \
		tftf_testcase_printf("Skipped at %s:%d", __FILE__, __LINE__); \
		return TEST_RESULT_SKIP;\
	}
#endif // tbb_test_infra_h_INCLUDED

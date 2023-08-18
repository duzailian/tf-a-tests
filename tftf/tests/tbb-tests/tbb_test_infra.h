#ifndef TBB_TEST_INFRA_H_INCLUDED
#define TBB_TEST_INFRA_H_INCLUDED

#include <tftf_lib.h>

test_result_t test_corrupt_boot_fip(unsigned int offset);

#define test_assert(must_be_true) \
	if (!(must_be_true)) { \
		tftf_testcase_printf("Failed at %s:%d", __FILE__, __LINE__); \
		return TEST_RESULT_FAIL;\
	}\


#define test_assert_skip(must_be_true) \
	if (!(must_be_true)) { \
		tftf_testcase_printf("Skipped at %s:%d", __FILE__, __LINE__); \
		return TEST_RESULT_SKIP;\
	}\

#endif // TBB_TEST_INFRA_H_INCLUDED


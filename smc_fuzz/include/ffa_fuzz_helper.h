/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fuzz_helper.h>
#include <power_management.h>
#include <test_helpers.h>
#include <tftf_lib.h>
#include <timer.h>
#include <runtime_services/ffa_helpers.h>
#include <runtime_services/ffa_svc.h>
#include "smcmalloc.h"

#ifndef ffa_id_get_funcid
#define ffa_id_get_funcid 0
#endif

#ifndef ffa_partition_info_get_regs_funcid
#define ffa_partition_info_get_regs_funcid 0
#endif

#ifndef ffa_version_funcid
#define ffa_version_funcid 0
#endif

#ifndef ffa_msg_send_direct_req_funcid
#define ffa_msg_send_direct_req_funcid 0
#endif

#ifndef ffa_msg_send_direct_resp_funcid
#define ffa_msg_send_direct_resp_funcid 0
#endif

#ifndef ffa_features_feat_id_funcid
#define ffa_features_feat_id_funcid 0
#endif

#ifndef ffa_features_func_id_funcid
#define ffa_features_func_id_funcid 0
#endif
#ifndef ffa_run_funcid
#define ffa_run_funcid 0
#endif
#ifndef ffa_notification_bitmap_create_funcid
#define ffa_notification_bitmap_create_funcid 0
#endif
#ifndef ffa_notification_bind_funcid
#define ffa_notification_bind_funcid 0
#endif
#ifndef ffa_notification_bitmap_destroy_funcid
#define ffa_notification_bitmap_destroy_funcid 0
#endif
void run_ffa_fuzz(int funcid, struct memmod *mmod);

/*
 * Copyright (c) 2020-2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <stddef.h>
#include <stdint.h>

#include <drivers/auth/crypto_mod.h>
#include "handoff.h"
#include "tcg.h"

/*
 * Set Event Log debug level to one of:
 *
 * LOG_LEVEL_ERROR
 * LOG_LEVEL_INFO
 * LOG_LEVEL_WARNING
 * LOG_LEVEL_VERBOSE
 */
#if EVENT_LOG_LEVEL   == LOG_LEVEL_ERROR
#define	LOG_EVENT	ERROR
#elif EVENT_LOG_LEVEL == LOG_LEVEL_NOTICE
#define	LOG_EVENT	NOTICE
#elif EVENT_LOG_LEVEL == LOG_LEVEL_WARNING
#define	LOG_EVENT	WARN
#elif EVENT_LOG_LEVEL == LOG_LEVEL_INFO
#define	LOG_EVENT	INFO
#elif EVENT_LOG_LEVEL == LOG_LEVEL_VERBOSE
#define	LOG_EVENT	VERBOSE
#else
#define LOG_EVENT printf
#endif

/* Number of hashing algorithms supported */
#define HASH_ALG_COUNT		1U

#define EVLOG_INVALID_ID	UINT32_MAX

#define MEMBER_SIZE(type, member) sizeof(((type *)0)->member)

typedef struct {
	unsigned int id;
	const char *name;
	unsigned int pcr;
} event_log_metadata_t;

#define	ID_EVENT_SIZE	(sizeof(id_event_headers_t) + \
			(sizeof(id_event_algorithm_size_t) * HASH_ALG_COUNT) + \
			sizeof(id_event_struct_data_t))

#define	LOC_EVENT_SIZE	(sizeof(event2_header_t) + \
			sizeof(tpmt_ha) + TCG_DIGEST_SIZE + \
			sizeof(event2_data_t) + \
			sizeof(startup_locality_event_t))

#define	LOG_MIN_SIZE	(ID_EVENT_SIZE + LOC_EVENT_SIZE)

#define EVENT2_HDR_SIZE	(sizeof(event2_header_t) + \
			sizeof(tpmt_ha) + TCG_DIGEST_SIZE + \
			sizeof(event2_data_t))

/* Functions' declarations */

/**
 * Initialise the Event Log buffer by setting global pointers
 * to manage log entries.
 *
 * @param[in] event_log_start   Base address of the Event Log buffer
 * @param[in] event_log_finish  End address of the Event Log buffer,
 *                               pointing to the first byte past the buffer
 */
int event_log_buf_init(uint8_t *event_log_start, uint8_t *event_log_finish);

/**
 * Dump the contents of the event log.
 *
 * This function outputs the event log buffer's contents for debugging
 * or auditing purposes.
 *
 * @param[in] log_addr  Pointer to the start of the event log buffer.
 * @param[in] log_size  Size of the event log buffer in bytes.
 *
 * @return 0 on success, or a negative error code if dumping fails.
 */
int event_log_dump(uint8_t *log_addr, size_t log_size);

/**
 * Initialize global variables for the Event Log.
 *
 * This function sets up the Event Log buffer, which is used to
 * store various payload measurements.
 *
 * @param[in] event_log_start  Base address of the Event Log buffer.
 * @param[in] event_log_finish End address of the Event Log buffer
 *                             (first byte past the buffer's end).
 *
 * @return 0 on success, or a negative error code on failure.
 */
int event_log_init(uint8_t *event_log_start, uint8_t *event_log_finish);

/**
 * Calculate the hash of an image, configuration data, or other input,
 * and record it in the event log.
 *
 * This function computes a cryptographic hash of the specified data
 * and logs it as an event for attestation purposes.
 *
 * @param[in] data_base     Base address of the data.
 * @param[in] data_size     Size of the data in bytes.
 * @param[in] data_id       Identifier for the data being measured.
 * @param[in] metadata_ptr  Pointer to event log metadata.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int event_log_measure_and_record(uintptr_t data_base, uint32_t data_size,
				 uint32_t data_id,
				 const event_log_metadata_t *metadata_ptr);

/**
 * Measure the given data and record its hash in the event log.
 *
 * This function calculates a cryptographic hash of the specified data
 * and stores it in the event log for attestation purposes.
 *
 * @param[in]  data_base  Base address of the data to measure.
 * @param[in]  data_size  Size of the data in bytes.
 * @param[out] hash_data  Buffer to store the computed hash
 *                        (up to CRYPTO_MD_MAX_SIZE bytes).
 *
 * @return 0 on success, or an error code on failure.
 */
int event_log_measure(uintptr_t data_base, uint32_t data_size,
		      unsigned char hash_data[CRYPTO_MD_MAX_SIZE]);

/**
 * Record a measurement as a TCG_PCR_EVENT2 event.
 *
 * @param[in] hash         Pointer to the hash data (TCG_DIGEST_SIZE bytes).
 * @param[in] event_type   Event type, as defined in tcg.h.
 * @param[in] metadata_ptr Pointer to an event_log_metadata_t structure.
 *
 * Ensure there is sufficient space in the event log buffer before calling this
 * function.
 */
int event_log_record(const uint8_t *hash, uint32_t event_type,
		     const event_log_metadata_t *metadata_ptr);

/**
 * Initialize the event log by writing the Specification ID
 * and Startup Locality events.
 *
 * This function prepares the event log by recording essential
 * metadata required for TPM-based measurements.
 *
 * @return 0 on success, or an error code on failure.
 */
int event_log_write_header(void);

/**
 * Write a SPECID event to the event log.
 *
 * This function records a TCG_PCR_EVENT2 structure containing the Specification
 * ID (SPECID) event, which provides details about the event log format and
 * supported algorithms.
 *
 * Ensure there is sufficient space in the event log buffer before calling this
 * function.
 *
 * @return 0 on success, or an error code on failure.
 */
int event_log_write_specid_event(void);

/**
 * Get the current size of the Event Log buffer (i.e., the used space).
 *
 * This function calculates the amount of space currently occupied
 * in the Event Log buffer.
 *
 * @param[in] event_log_start  Pointer to the start of the Event Log buffer.
 *
 * @return The current size of the Event Log buffer in bytes.
 */
size_t event_log_get_cur_size(uint8_t *event_log_start);

#endif /* EVENT_LOG_H */

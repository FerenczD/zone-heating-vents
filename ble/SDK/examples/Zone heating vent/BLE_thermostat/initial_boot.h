#ifndef INITIAL_BOOT_H
#define INITIAL_BOOT_H

#include <stdint.h>
#include <string.h>
#include "nrf.h"
#include "nordic_common.h"
#include "fds.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define CONFIG_FILE     0x1111                                       /* File ID and Key used for the configuration record in FDS. */
#define CONFIG_REC_KEY  0x2222

static uint32_t m_initial_boot = true;                                  /* True if pairing mode is enabled */ 

static bool volatile m_fds_initialized;

/* Store in flash */
static fds_record_t const m_boot_record =
{
    .file_id           = CONFIG_FILE,
    .key               = CONFIG_REC_KEY,
    .data.p_data       = &m_initial_boot,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(m_initial_boot)) / sizeof(uint32_t),
};

int initial_boot();
void factory_reset();

#endif
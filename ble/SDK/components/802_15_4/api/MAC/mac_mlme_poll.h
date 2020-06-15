/**
 * Copyright (c) 2016 - 2019 Nordic Semiconductor ASA and Luxoft Global Operations Gmbh.
 *
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef MAC_MLME_POLL_H_INCLUDED
#define MAC_MLME_POLL_H_INCLUDED

#include <stdint.h>
#include "mac_common.h"
#include "mac_task_scheduler.h"

/** @file
 * The MAC MLME Poll module declares the MAC Poll primitives and necessary types
 * according to the MAC specification.
 *
 * @defgroup mac_poll MAC MLME Poll API
 * @ingroup mac_15_4
 * @{
 * @brief Module to declare MAC MLME Poll API.
 * @details The MAC Poll module declares MLME Poll primitives and necessary types according to
 * the MAC specification. More specifically, MLME Poll request aka mlme_poll_req(), MLME Poll
 * indicaton aka mlme_poll_ind(), and MLME Poll confirm callback typedef aka mlme_poll_conf_cb_t
 * primitives are declared.
 */

/**@brief   MLME-POLL.confirm
 *
 * @details The MLME-POLL.confirm primitive reports the results of a request
 * to poll the coordinator for data.
 *
 * In accordance with IEEE Std 802.15.4-2006, section 7.1.16.2
 */
typedef struct
{
    mac_status_t    status;                     /**< Status of operation. */
} mlme_poll_conf_t;


/**@brief   MLME-POLL.request
 *
 * @details The MLME-POLL.request primitive prompts the device
 * to request data from the coordinator.
 *
 * In accordance with IEEE Std 802.15.4-2006, section 7.1.16.1
 */
typedef struct
{
    /** Do not edit this field. */
    mac_abstract_req_t  service;

    /** Confirm to this request. */
    mlme_poll_conf_t    confirm;

    mac_addr_mode_t     coord_addr_mode;        /**< Coordinator address mode. */
    uint16_t            coord_pan_id;           /**< Coordinator PAN ID. */
    mac_addr_t          coord_address;          /**< Coordinator address. */
#if (CONFIG_SECURE == 1)
    uint8_t             security_level;         /**< Security level. */
    uint8_t             key_id_mode;            /**< Key ID mode. */
    uint64_t            key_source;             /**< Key source. */
    uint8_t             key_index;              /**< Key index. */
#endif
} mlme_poll_req_t;

/** @brief MLME-Poll.indication
 *
 * @details The MLME-POLL.indication primitive indicates the reception
 * of a Data request command frame by the MAC sub-layer and issued to
 * the local SSCS (service specific convergence sublayer).
 */
typedef struct
{
    mac_addr_mode_t src_addr_mode;               /**< Source address mode. */
    mac_addr_t      src_address;                 /**< Source address. */
} mlme_poll_ind_t;


/**@brief   Prototype of the user-implemented MLME-POLL.confirm callback function.
 *
 * @details The MLME-POLL.confirm primitive is generated by the MLME and issued
 * to its next higher layer in response to an MLME-POLL.request primitive.
 * If the request was successful, the status parameter will be equal to SUCCESS,
 * indicating a successful poll for data. Otherwise, the status parameter indicates the
 * appropriate error code. The status values are fully described in 7.1.16.1.3 and
 * the subclauses referenced by 7.1.16.1.3.
 *
 * @param pointer to a confirmation primitive.
 */
typedef void (* mlme_poll_conf_cb_t)(mlme_poll_conf_t *);


/**@brief   MLME-POLL.request
 *
 * @details The MLME-POLL.request primitive is generated by the next higher layer and
 * issued to its MLME when data are to be requested from a coordinator.
 *
 * @param[in]   req       MLME-POLL.request parameters
 * @param[in]   conf_cb   User-implemented callback function, which will be
 *                        called by MLME in order to pass MLME-POLL.confirm to the user.
 */
void mlme_poll_req(mlme_poll_req_t * req, mlme_poll_conf_cb_t conf_cb);

/**@brief   MLME-POLL.indication
 *
 * @details The MLME-Poll.indication primitive notifies the next higher level that
 * a request for data has been received.
 *
 * @param[in] p_ind   pointer to a poll indication structure
 */
extern void mlme_poll_ind(mlme_poll_ind_t * p_ind);

/** @} */

#endif // MAC_MLME_POLL_H_INCLUDED

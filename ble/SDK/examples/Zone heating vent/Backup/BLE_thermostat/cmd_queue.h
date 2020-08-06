#ifndef CMD_QUEUE_H
#define CMD_QUEUE_H

/* Hash table related functions */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "uart_ble.h"

#define HASHSIZE 101

/* table entry: */
//typedef struct nlist nlist;

//typedef struct nlist {    
//    struct nlist* next;                        /* next entry in chain */
//    uint8_t* ble_addr;                  /* BLE MAC address */
//    vent_cmd_t* data;                   /* command stored */
//}nlist;

typedef struct nlist {    
    struct nlist* next;                        /* next entry in chain */
    uint8_t ble_addr[BLE_ADDR_LEN];                  /* BLE MAC address */
    vent_cmd_t data;                   /* command stored */
}nlist;

static nlist* hashtab[HASHSIZE]; /* pointer table */

nlist* cmd_lookup(uint8_t *ble_addr);

nlist* cmd_add(uint8_t* ble_addr, vent_cmd_t cmd);

unsigned hash(uint8_t* ble_addr);

int addr_cmp(uint8_t* s, uint8_t* target);

void free_cmd(nlist* np);

void delete_cmd(nlist* cmd);
#endif
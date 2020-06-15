#ifndef UTIL_H
#define UTIL_H

/* Utility functions for the Air vent*/

/* Libraries */
#include "ble_ves.h"

/* Definitions */
/* Vent status codes */
#define STATUS_VENT_ERROR               0
#define STATUS_VENT_OK                  1                                       
#define STATUS_VENT_OPENING             2                                      
#define STATUS_VENT_CLOSING             3                                       
#define STATUS_VENT_OPENED              4                                       
#define STATUS_VENT_CLOSED              5                

/* Global */
extern ble_ves_t m_ves;
extern bool status_notifications_enable;

/* Prototypes */

void status_update(int status);

#endif
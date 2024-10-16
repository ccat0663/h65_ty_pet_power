#ifndef _KEY_DRIVER_H 
#define _KEY_DRIVER_H 
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

enum{

    KEY_EVENT_LONG = 0,

    KEY_EVENT_MAX
};

typedef struct
{
    uint8_t event;

}key_event_t;


typedef void (*key_event_cb)(key_event_t *e);

void key_driver_init(key_event_cb cb);

void key_pro(uint16_t tick_ms);

#ifdef __cplusplus
}
#endif
#endif	// _KEY_DRIVER_H


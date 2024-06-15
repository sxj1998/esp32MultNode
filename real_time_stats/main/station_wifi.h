#ifndef __STATION_WIFI_H__
#define __STATION_WIFI_H__

#include <string.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_init_sta(void);

#ifdef __cplusplus
}
#endif

#endif /* __STATION_WIFI_H__ */

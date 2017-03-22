#include "arrow/storage.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arrow/utf8.h>
#include <debug.h>

#include <project.h>

typedef struct {
  int magic;
  char ssid[64];
  char pass[64];
  int sec;
  char padding[116];
  int config;
  char gateway_hid[64];
  char device_hid[64];
  char device_eid[64];
  char unused[64];
} flash_mem_t;

/*
int check_mgc() {
  int *c = flash_start();
  if ( *c != FLASH_MAGIC_NUMBER ) {
    return 0;
  }
  return 1;
}
*/

static flash_mem_t flash;

#define ROW 1024

static void read_flash() {
    static int read = 0;
    if (!read) {
        char *ptr;
        ptr = (CY_FLASH_BASE + ROW * CY_FLASH_SIZEOF_ROW);
        memcpy(&flash, ptr, sizeof(flash_mem_t));        
        read = 1;
    }
}

static void write_flash() {
    int size = 0;
    uint8 *buffer = (uint8 *)&flash;
    while( size < sizeof(flash_mem_t) ) {
        int res = CySysFlashWriteRow(ROW, buffer);
        if ( res != CY_SYS_FLASH_SUCCESS ) {
            DBG("wrire flash error %d", res);
            return;
        }
        size += CY_FLASH_SIZEOF_ROW;
    }
}

int restore_gateway_info(arrow_gateway_t *gateway) {
    read_flash();    
    DBG("restore gateway info\r\n");
    if ( utf8check(flash.gateway_hid) && strlen(flash.gateway_hid) > 0 ) {
      arrow_gateway_add_hid(gateway, flash.gateway_hid);
      return 0;
    }
    return -1;
}

void save_gateway_info(const arrow_gateway_t *gateway) {
  DBG("new registration\r\n");
  strcpy(flash.gateway_hid, gateway->hid);
  write_flash();
}

int restore_device_info(arrow_device_t *device) {
    if ( !utf8check(flash.device_hid) || strlen(flash.device_hid) == 0 ) {
      return -1;
    }
    arrow_device_set_hid(device, flash.device_hid);
  #if defined(__IBM__)
    if ( !utf8check(flash.device_eid) || strlen(flash.device_eid) == 0 ) {
      return -1;
    }
    arrow_device_set_eid(device, flash.device_eid);
  #endif
    return 0;
}

void save_device_info(arrow_device_t *device) {
  strcpy(flash.device_hid, device->hid);
#if defined(__IBM__)
  strcpy(flash.device_eid, device->eid);
#endif
  write_flash();
}

void save_wifi_setting(const char *ssid, const char *pass, int sec) {
  strcpy(flash.ssid, ssid);
  strcpy(flash.pass, pass);
  flash.sec = sec;
  write_flash();
}

int restore_wifi_setting(char *ssid, char *pass, int *sec) {
    strcpy(ssid, flash.ssid);
    strcpy(pass, flash.pass);
    *sec = flash.sec;
    return 0;
}

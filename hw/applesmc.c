/*
 *  Apple SMC controller
 *
 *  Copyright (c) 2007 Alexander Graf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * *****************************************************************
 *
 * In all Intel-based Apple hardware there is an SMC chip to control the
 * backlight, fans and several other generic device parameters. It also
 * contains the magic keys used to dongle Mac OS X to the device.
 *
 * This driver was mostly created by looking at the Linux AppleSMC driver
 * implementation and does not support IRQ.
 *
 */

#include "hw.h"
#include "pci.h"
#include "pc.h"
#include "console.h"
#include "qemu-timer.h"

/* data port used by Apple SMC */
#define APPLESMC_DATA_PORT	0x300
/* command/status port used by Apple SMC */
#define APPLESMC_CMD_PORT	0x304
#define APPLESMC_NR_PORTS	32 /* 0x300-0x31f */
#define APPLESMC_MAX_DATA_LENGTH 32

#define APPLESMC_READ_CMD	0x10
#define APPLESMC_WRITE_CMD	0x11
#define APPLESMC_GET_KEY_BY_INDEX_CMD	0x12
#define APPLESMC_GET_KEY_TYPE_CMD	0x13

static char osk[64] = "ourhardworkbythesewordsguardedpleasedontsteal(c)AppleComputerInc";

struct AppleSMCData {
    uint8_t len;
    char *key;
    char *data;
};

static struct AppleSMCData data[] = {
    { .key = "REV ", .len=6, .data="\0x01\0x13\0x0f\0x00\0x00\0x03" },
    { .key = "OSK0", .len=32, .data=osk },
    { .key = "OSK1", .len=32, .data=osk+32 },
    { .key = "NATJ", .len=1, .data="\0" },
    { .key = "MSSP", .len=1, .data="\0" },
    { .key = "MSSD", .len=1, .data="\0x3" },
    { .len=0 }
};

struct AppleSMCStatus {
    uint8_t cmd;
    uint8_t status;
    uint8_t key[4];
    uint8_t read_pos;
    uint8_t data_len;
    uint8_t data_pos;
    uint8_t data[255];
    uint8_t charactic[4];
};

static void applesmc_io_cmd_writeb(void *opaque, uint32_t addr, uint32_t val)
{
    struct AppleSMCStatus *s = (struct AppleSMCStatus *)opaque;
    printf("APPLESMC: CMD Write B: %#x = %#x\n", addr, val);
    switch(val) {
        case APPLESMC_READ_CMD:
            s->status = 0x0c;
            break;
    }
    s->cmd = val;
    s->read_pos = 0;
    s->data_pos = 0;
}

static void applesmc_fill_data(struct AppleSMCStatus *s)
{
    struct AppleSMCData *d;
    for(d=data; d->len; d++) {
        uint32_t key_data = *((uint32_t*)d->key);
        uint32_t key_current = *((uint32_t*)s->key);
        if(key_data == key_current) {
            printf("APPLESMC: Key matched (%s Len=%d Data=%s)\n", d->key, d->len, d->data);
            memcpy(s->data, d->data, d->len);
            return;
        }
    }
}

static void applesmc_io_data_writeb(void *opaque, uint32_t addr, uint32_t val)
{
    struct AppleSMCStatus *s = (struct AppleSMCStatus *)opaque;
    printf("APPLESMC: DATA Write B: %#x = %#x\n", addr, val);
    switch(s->cmd) {
        case APPLESMC_READ_CMD:
            if(s->read_pos < 4) {
                s->key[s->read_pos] = val;
                s->status = 0x04;
            } else if(s->read_pos == 4) {
                s->data_len = val;
                s->status = 0x05;
                s->data_pos = 0;
                printf("APPLESMC: Key = %c%c%c%c Len = %d\n", s->key[0], s->key[1], s->key[2], s->key[3], val);
                applesmc_fill_data(s);
            }
            s->read_pos++;
            break;
    }
}

static uint32_t applesmc_io_data_readb(void *opaque, uint32_t addr1)
{
    struct AppleSMCStatus *s = (struct AppleSMCStatus *)opaque;
    uint8_t retval = 0;
    switch(s->cmd) {
        case APPLESMC_READ_CMD:
            if(s->data_pos < s->data_len) {
                retval = s->data[s->data_pos];
                printf("APPLESMC: READ_DATA[%d] = %#hhx\n", s->data_pos, retval);
                s->data_pos++;
                if(s->data_pos == s->data_len) {
                    s->status = 0x00;
                    printf("APPLESMC: EOF\n");
                } else
                    s->status = 0x05;
            }
    }
    printf("APPLESMC: DATA Read b: %#x = %#x\n", addr1, retval);
    return retval;
}

static uint32_t applesmc_io_cmd_readb(void *opaque, uint32_t addr1)
{
    printf("APPLESMC: CMD Read B: %#x\n", addr1);
    return ((struct AppleSMCStatus*)opaque)->status;
}

void applesmc_setkey(char *key) {
    if(strlen(key) == 64) {
        memcpy(osk, key, 64);
    }
}

void applesmc_init() {
    struct ApleSMCStatus *s;
    s = qemu_mallocz(sizeof(struct AppleSMCStatus));

    if(osk[0] == 'T') {
        printf("WARNING: Using AppleSMC with invalid key\n");
    }
    register_ioport_read(APPLESMC_DATA_PORT, 4, 1, applesmc_io_data_readb, s);
    register_ioport_read(APPLESMC_CMD_PORT, 4, 1, applesmc_io_cmd_readb, s);
    register_ioport_write(APPLESMC_DATA_PORT, 4, 1, applesmc_io_data_writeb, s);
    register_ioport_write(APPLESMC_CMD_PORT, 4, 1, applesmc_io_cmd_writeb, s);
}


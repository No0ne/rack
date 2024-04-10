/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 No0ne (https://github.com/No0ne)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "macphy.pio.h"

u8 buf = 0x7b;

u8 const hid2mac[] = {
  0x7b, 0x7b, 0x7b, 0x7b, 0x01, 0x17, 0x11, 0x05, 0x1d, 0x07, 0x0b, 0x09, 0x45, 0x4d, 0x51, 0x4b,
  0x5d, 0x5b, 0x3f, 0x47, 0x19, 0x1f, 0x03, 0x23, 0x41, 0x13, 0x1b, 0x0f, 0x21, 0x0d, 0x25, 0x27,
  0x29, 0x2b, 0x2f, 0x2d, 0x35, 0x39, 0x33, 0x3b, 0x49, 0x7b, 0x67, 0x61, 0x63, 0x37, 0x31, 0x43,
  0x3d, 0x55
};

void kb_send_key(u8 key, bool state, u8 modifiers) {
  
  if(key > sizeof(hid2mac)) return;
  
  if(state) {
    buf = hid2mac[key];
  } else {
    buf = hid2mac[key] | 0x80;
  }
  
}

void kb_usb_receive(u8 const* report) {
  if(report[1] == 0) {
  
    if(report[0] != prev_rpt[0]) {
      u8 rbits = report[0];
      u8 pbits = prev_rpt[0];
      
      for(u8 j = 0; j < 8; j++) {
        if((rbits & 1) != (pbits & 1)) {
          kb_send_key(HID_KEY_CONTROL_LEFT + j, rbits & 1, report[0]);
        }
        
        rbits = rbits >> 1;
        pbits = pbits >> 1;
      }
    }
    
    for(u8 i = 2; i < 8; i++) {
      if(prev_rpt[i]) {
        bool brk = true;
        
        for(u8 j = 2; j < 8; j++) {
          if(prev_rpt[i] == report[j]) {
            brk = false;
            break;
          }
        }
        
        if(brk) {
          kb_send_key(prev_rpt[i], false, report[0]);
        }
      }
      
      if(report[i]) {
        bool make = true;
        
        for(u8 j = 2; j < 8; j++) {
          if(report[i] == prev_rpt[j]) {
            make = false;
            break;
          }
        }
        
        if(make) {
          kb_send_key(report[i], true, report[0]);
        }
      }
    }
    
    memcpy(prev_rpt, report, sizeof(prev_rpt));
  }
}

bool kb_task() {
  
  if(!pio_sm_is_rx_fifo_empty(pio0, 0)) {
    u32 fifo = pio_sm_get(pio0, 0);
    printf("%02x ", fifo);
    
    u32 bb;
    
    switch(fifo) {
      case 0x10:
      case 0x14:
        bb = buf << 24;
        if(buf != 0x7b) printf("%02x  ", buf);
        buf = 0x7b;
        pio_sm_put(pio0, 0, bb);
      break;
      
      case 0x16:
        bb = 0x0b << 24;
        printf("%02x  ", bb);
        pio_sm_put(pio0, 0, bb);
      break;
      
      case 0x36:
        bb = 0x7d << 24;
        printf("%02x  ", bb);
        pio_sm_put(pio0, 0, bb);
      break;
    }
  }
  
  return kb_enabled && !kb_phy.busy;
}

void kb_init(u8 gpio) {
  macphy_program_init(pio0, pio_claim_unused_sm(pio0, true), pio_add_program(pio0, &macphy_program), 11);
}

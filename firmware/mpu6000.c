/*
 * Copyright (c) 2013, M.Naruoka (fenrir)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * - Neither the name of the naruoka.org nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "c8051f380.h"
#include "main.h"
#include "config.h"

#include <string.h>

#include "mpu6000.h"

#include "util.h"
#include "type.h"
#include "data_hub.h"

#define cs_wait() wait_8n6clk(50)
#define clk_wait() wait_8n6clk(5)

typedef enum {
  SELF_TEST_X = 0x0D,
  SELF_TEST_Y = 0x0E,
  SELF_TEST_Z = 0x0F,
  SELF_TEST_A = 0x10,
  SMPLRT_DIV = 0x19,
  CONFIG = 0x1A,
  GYRO_CONFIG = 0x1B,
  ACCEL_CONFIG = 0x1C,
  FIFO_EN = 0x23,
  I2C_MST_CTRL = 0x24,
  I2C_SLV0_ADDR = 0x25,
  I2C_SLV0_REG = 0x26,
  I2C_SLV0_CTRL = 0x27,
  I2C_SLV1_ADDR = 0x28,
  I2C_SLV1_REG = 0x29,
  I2C_SLV1_CTRL = 0x2A,
  I2C_SLV2_ADDR = 0x2B,
  I2C_SLV2_REG = 0x2C,
  I2C_SLV2_CTRL = 0x2D,
  I2C_SLV3_ADDR = 0x2E,
  I2C_SLV3_REG = 0x2F,
  I2C_SLV3_CTRL = 0x30,
  I2C_SLV4_ADDR = 0x31,
  I2C_SLV4_REG = 0x32,
  I2C_SLV4_DO = 0x33,
  I2C_SLV4_CTRL = 0x34,
  I2C_SLV4_DI = 0x35,
  I2C_MST_STATUS = 0x36,
  INT_PIN_CFG = 0x37,
  INT_ENABLE = 0x38,
  INT_STATUS = 0x3A,
  ACCEL_OUT_BASE = 0x3B,
  TEMP_OUT_BASE = 0x41,
  GYRO_OUT_BASE = 0x43,
  EXT_SENS_DATA_BASE = 0x49,
  I2C_SLV0_DO = 0x63,
  I2C_SLV1_DO = 0x64,
  I2C_SLV2_DO = 0x65,
  I2C_SLV3_DO = 0x66,
  I2C_MST_DELAY_CTRL = 0x67,
  SIGNAL_PATH_RESET = 0x68,
  USER_CTRL = 0x6A,
  PWR_MGMT_1 = 0x6B,
  PWR_MGMT_2 = 0x6C,
  FIFO_COUNTH = 0x72,
  FIFO_COUNTL = 0x73,
  FIFO_R_W = 0x74,
  WHO_AM_I = 0x75,
} address_t;

/*
 * MPU-6000
 *
 * === Connection ===
 * C8051         MPU-6000
 *  P1.4(OUT) =>  SCK
 *  P1.5(OUT) =>  MOSI
 *  P1.6(OUT) =>  -CS
 *  P1.7(IN)  <=  MISO
 */

#ifdef USE_ASM_FOR_SFR_MANIP
#define clk_up()      {__asm orl _P1,SHARP  0x10 __endasm; }
#define clk_down()    {__asm anl _P1,SHARP ~0x10 __endasm; }
#define out_up()      {__asm orl _P1,SHARP  0x20 __endasm; }
#define out_down()    {__asm anl _P1,SHARP ~0x20 __endasm; }
#define cs_assert()   {__asm anl _P1,SHARP ~0x40 __endasm; }
#define cs_deassert() {__asm orl _P1,SHARP  0x40 __endasm; }
#else
#define clk_up()      (P1 |=  0x10)
#define clk_down()    (P1 &= ~0x10)
#define out_up()      (P1 |=  0x20)
#define out_down()    (P1 &= ~0x20)
#define cs_assert()   (P1 &= ~0x40)
#define cs_deassert() (P1 |=  0x40)
#endif
#define is_in_up()    (P1 & 0x80)

static void mpu6000_write(u8 *buf, u8 size){
  for(; size--; buf++){
    u8 mask = 0x80;
    do{
      clk_down();
      if((*buf) & mask){out_up();}else{out_down();}
      clk_wait();
      clk_up();
      clk_wait();
    }while(mask >>= 1);
  }
}

static void mpu6000_read(u8 *buf, u8 size){
  for(; size--; buf++){
    u8 temp = 0;
    u8 mask = 0x80;
    do{
      clk_down();
      clk_wait();
      clk_up();
      if(is_in_up()) temp |= mask;
      clk_wait();
    }while(mask >>= 1);
    *buf = temp;
  }
}

#define _mpu6000_set(address, value, attr) { \
  attr addr_value[2] = {address, value}; \
  cs_assert(); \
  cs_wait(); \
  mpu6000_write(addr_value, sizeof(addr_value)); \
  cs_deassert(); \
  cs_wait(); \
}

#define mpu6000_set(address, value) \
    _mpu6000_set(address, value, static const __code u8)

#define mpu6000_set2(address, value) \
    _mpu6000_set(address, value, u8)

#define mpu6000_get(address, value) { \
  static const __code u8 addr[1] = {0x80 | address}; \
  cs_assert(); \
  cs_wait(); \
  mpu6000_write(addr, sizeof(addr)); \
  mpu6000_read((u8 *)&(value), sizeof(value)); \
  cs_deassert(); \
  cs_wait(); \
}

volatile __bit mpu6000_capture = FALSE;

static __bit mpu6000_available = FALSE;

void mpu6000_init(){
  cs_deassert();
  clk_up();
  mpu6000_set(PWR_MGMT_1, 0x80); // Chip reset
  wait_ms(100);
  mpu6000_set(PWR_MGMT_1, 0x03); // Wake up device and select GyroZ clock (better performance)
  
  mpu6000_set(USER_CTRL, 0x34); // Enable Master I2C, disable primary I2C I/F, and reset FIFO.
  mpu6000_set(SMPLRT_DIV, 79); // SMPLRT_DIV = 79, 100Hz sampling;
  // CONFIG = 0; // Disable FSYNC, No DLPF
  mpu6000_set2(GYRO_CONFIG, config.inertial.gyro_config);
  mpu6000_set2(ACCEL_CONFIG, config.inertial.accel_config);
  mpu6000_set(FIFO_EN, 0xF8); // FIFO enabled for temperature(2), gyro(2 * 3), accelerometer(2 * 3). Total 14 bytes.
  mpu6000_set(I2C_MST_CTRL, (0xC8 | 13)); // Multi-master, Wait for external sensor, I2C stop then start cond., clk 400KHz
  mpu6000_set(USER_CTRL, 0x70); // Enable FIFO with Master I2C enabled, and primary I2C I/F disabled.

  {
    u8 who_am_i; // expecting 0x68
    mpu6000_get(WHO_AM_I, who_am_i);
    if(who_am_i == 0x68){
      mpu6000_available = TRUE;
    }
  }
}

static void make_packet(packet_t *packet){
  
  payload_t *dst = packet->current, *dst_end = packet->buf_end;
  
  // Check whether buffer size is sufficient
  if((dst_end - dst) < SYLPHIDE_PAGESIZE){
    return;
  }
  
  *(dst++) = 'A';
  *(dst++) = u32_lsbyte(tickcount);
  
  // Record time, LSB first
  //*((u32 *)(packet->current)) = global_ms;
  memcpy(dst, &global_ms, sizeof(global_ms));
  dst += sizeof(global_ms);
  
  memset(dst, 0, dst_end - dst);
  
  // Get values
  {
    // from FIFO, accelerometer, temperature, and gyro values are extracted.
    u8 buf[14], i;
    __data u8 *_buf;
    mpu6000_get(FIFO_R_W, buf);
    
    /* 
     * In the following, take care of 2�fs complement value.
     * raw => modified:
     * -128 = 0b10000000 (128) => 0b00000000 (0)
     * -36  = 0b11011100 (220) => 0b01011100 (92)
     *  36  = 0b00100100 (36)  => 0b10100100 (164)
     *  127 = 0b01111111 (127) => 0b11111111 (255)
     */
    
    _buf = buf;
    // accel, big endian, advances packet->current by 3 * 3 = 9 bytes
    for(i = 0; i < 3; ++i, _buf += 2){
      *_buf ^= 0x80;
      memcpy(++dst, _buf, 2);
      dst += 2;
    }
    _buf += 2; // skip temperature
    // gyro, big endian, advances packet->current by 3 * 3 = 9 bytes
    for(i = 0; i < 3; ++i, _buf += 2){
      *_buf ^= 0x80;
      memcpy(++dst, _buf, 2);
      dst += 2;
    }
    // ch.7, ch.8 unused, advances packet->current by 3 * 2 = 6 bytes
    dst += 6;
    // temperature, little endian
    *(dst++) = buf[7];
    *(dst++) = buf[6] ^ 0x80;
  }
  
  packet->current = dst;
}

void mpu6000_polling(){
  if(!mpu6000_available){return;}
  if(mpu6000_capture){

    // Whether data is ready or not.
    WORD_t fifo_count;
    mpu6000_get(FIFO_COUNTH, fifo_count.c[1]);
    mpu6000_get(FIFO_COUNTL, fifo_count.c[0]);

    if(fifo_count.i < 14){return;}
    
    mpu6000_capture = FALSE;
    data_hub_assign_page(make_packet);
    
    // Reset FIFO
    if(fifo_count.i > 14){
      mpu6000_set(USER_CTRL, 0x34);
      mpu6000_set(USER_CTRL, 0x70);
    }
  }
}

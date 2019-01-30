/**-------------------------------------------------------------------------
@file	agm_mpu9250.cpp

@brief	Implementation of TDK MPU-9250 accel, gyro, mag sensor

@author	Hoang Nguyen Hoan
@date	Nov. 18, 2017

@license

Copyright (c) 2017, I-SYST inc., all rights reserved

Permission to use, copy, modify, and distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright
notice and this permission notice appear in all copies, and none of the
names : I-SYST or its contributors may be used to endorse or
promote products derived from this software without specific prior written
permission.

For info or contributing contact : hnhoan at i-syst dot com

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------*/

#include "idelay.h"
#include "coredev/i2c.h"
#include "coredev/spi.h"
#include "sensors/agm_mpu9250.h"

#define DMP_CODE_SIZE           (3062)
#define DMP_START_ADDR			(0x400)

static const uint8_t s_DMPImage[DMP_CODE_SIZE] = {
    /* bank # 0 */
    0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00,
    0x00, 0x65, 0x00, 0x54, 0xff, 0xef, 0x00, 0x00, 0xfa, 0x80, 0x00, 0x0b, 0x12, 0x82, 0x00, 0x01,
    0x03, 0x0c, 0x30, 0xc3, 0x0e, 0x8c, 0x8c, 0xe9, 0x14, 0xd5, 0x40, 0x02, 0x13, 0x71, 0x0f, 0x8e,
    0x38, 0x83, 0xf8, 0x83, 0x30, 0x00, 0xf8, 0x83, 0x25, 0x8e, 0xf8, 0x83, 0x30, 0x00, 0xf8, 0x83,
    0xff, 0xff, 0xff, 0xff, 0x0f, 0xfe, 0xa9, 0xd6, 0x24, 0x00, 0x04, 0x00, 0x1a, 0x82, 0x79, 0xa1,
    0x00, 0x00, 0x00, 0x3c, 0xff, 0xff, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x38, 0x83, 0x6f, 0xa2,
    0x00, 0x3e, 0x03, 0x30, 0x40, 0x00, 0x00, 0x00, 0x02, 0xca, 0xe3, 0x09, 0x3e, 0x80, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00,
    0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x18, 0x6e, 0x00, 0x00, 0x06, 0x92, 0x0a, 0x16, 0xc0, 0xdf,
    0xff, 0xff, 0x02, 0x56, 0xfd, 0x8c, 0xd3, 0x77, 0xff, 0xe1, 0xc4, 0x96, 0xe0, 0xc5, 0xbe, 0xaa,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x0b, 0x2b, 0x00, 0x00, 0x16, 0x57, 0x00, 0x00, 0x03, 0x59,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xfa, 0x00, 0x02, 0x6c, 0x1d, 0x00, 0x00, 0x00, 0x00,
    0x3f, 0xff, 0xdf, 0xeb, 0x00, 0x3e, 0xb3, 0xb6, 0x00, 0x0d, 0x22, 0x78, 0x00, 0x00, 0x2f, 0x3c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x42, 0xb5, 0x00, 0x00, 0x39, 0xa2, 0x00, 0x00, 0xb3, 0x65,
    0xd9, 0x0e, 0x9f, 0xc9, 0x1d, 0xcf, 0x4c, 0x34, 0x30, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00,
    0x3b, 0xb6, 0x7a, 0xe8, 0x00, 0x64, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* bank # 1 */
    0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0xfa, 0x92, 0x10, 0x00, 0x22, 0x5e, 0x00, 0x0d, 0x22, 0x9f,
    0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0xff, 0x46, 0x00, 0x00, 0x63, 0xd4, 0x00, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x04, 0xd6, 0x00, 0x00, 0x04, 0xcc, 0x00, 0x00, 0x04, 0xcc, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x72, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x06, 0x00, 0x02, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x64, 0x00, 0x20, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x32, 0xf8, 0x98, 0x00, 0x00, 0xff, 0x65, 0x00, 0x00, 0x83, 0x0f, 0x00, 0x00,
    0xff, 0x9b, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0xb2, 0x6a, 0x00, 0x02, 0x00, 0x00,
    0x00, 0x01, 0xfb, 0x83, 0x00, 0x68, 0x00, 0x00, 0x00, 0xd9, 0xfc, 0x00, 0x7c, 0xf1, 0xff, 0x83,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x64, 0x03, 0xe8, 0x00, 0x64, 0x00, 0x28,
    0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x16, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf4, 0x00, 0x00, 0x10, 0x00,
    /* bank # 2 */
    0x00, 0x28, 0x00, 0x00, 0xff, 0xff, 0x45, 0x81, 0xff, 0xff, 0xfa, 0x72, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x05, 0x00, 0x05, 0xba, 0xc6, 0x00, 0x47, 0x78, 0xa2,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x14,
    0x00, 0x00, 0x25, 0x4d, 0x00, 0x2f, 0x70, 0x6d, 0x00, 0x00, 0x05, 0xae, 0x00, 0x0c, 0x02, 0xd0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x64, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e,
    0x00, 0x00, 0x0a, 0xc7, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0xff, 0xff, 0xff, 0x9c,
    0x00, 0x00, 0x0b, 0x2b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x64,
    0xff, 0xe5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* bank # 3 */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x24, 0x26, 0xd3,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x10, 0x00, 0x96, 0x00, 0x3c,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0c, 0x0a, 0x4e, 0x68, 0xcd, 0xcf, 0x77, 0x09, 0x50, 0x16, 0x67, 0x59, 0xc6, 0x19, 0xce, 0x82,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0xd7, 0x84, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc7, 0x93, 0x8f, 0x9d, 0x1e, 0x1b, 0x1c, 0x19,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0x18, 0x85, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x67, 0x7d, 0xdf, 0x7e, 0x72, 0x90, 0x2e, 0x55, 0x4c, 0xf6, 0xe6, 0x88,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* bank # 4 */
    0xd8, 0xdc, 0xb4, 0xb8, 0xb0, 0xd8, 0xb9, 0xab, 0xf3, 0xf8, 0xfa, 0xb3, 0xb7, 0xbb, 0x8e, 0x9e,
    0xae, 0xf1, 0x32, 0xf5, 0x1b, 0xf1, 0xb4, 0xb8, 0xb0, 0x80, 0x97, 0xf1, 0xa9, 0xdf, 0xdf, 0xdf,
    0xaa, 0xdf, 0xdf, 0xdf, 0xf2, 0xaa, 0xc5, 0xcd, 0xc7, 0xa9, 0x0c, 0xc9, 0x2c, 0x97, 0xf1, 0xa9,
    0x89, 0x26, 0x46, 0x66, 0xb2, 0x89, 0x99, 0xa9, 0x2d, 0x55, 0x7d, 0xb0, 0xb0, 0x8a, 0xa8, 0x96,
    0x36, 0x56, 0x76, 0xf1, 0xba, 0xa3, 0xb4, 0xb2, 0x80, 0xc0, 0xb8, 0xa8, 0x97, 0x11, 0xb2, 0x83,
    0x98, 0xba, 0xa3, 0xf0, 0x24, 0x08, 0x44, 0x10, 0x64, 0x18, 0xb2, 0xb9, 0xb4, 0x98, 0x83, 0xf1,
    0xa3, 0x29, 0x55, 0x7d, 0xba, 0xb5, 0xb1, 0xa3, 0x83, 0x93, 0xf0, 0x00, 0x28, 0x50, 0xf5, 0xb2,
    0xb6, 0xaa, 0x83, 0x93, 0x28, 0x54, 0x7c, 0xf1, 0xb9, 0xa3, 0x82, 0x93, 0x61, 0xba, 0xa2, 0xda,
    0xde, 0xdf, 0xdb, 0x81, 0x9a, 0xb9, 0xae, 0xf5, 0x60, 0x68, 0x70, 0xf1, 0xda, 0xba, 0xa2, 0xdf,
    0xd9, 0xba, 0xa2, 0xfa, 0xb9, 0xa3, 0x82, 0x92, 0xdb, 0x31, 0xba, 0xa2, 0xd9, 0xba, 0xa2, 0xf8,
    0xdf, 0x85, 0xa4, 0xd0, 0xc1, 0xbb, 0xad, 0x83, 0xc2, 0xc5, 0xc7, 0xb8, 0xa2, 0xdf, 0xdf, 0xdf,
    0xba, 0xa0, 0xdf, 0xdf, 0xdf, 0xd8, 0xd8, 0xf1, 0xb8, 0xaa, 0xb3, 0x8d, 0xb4, 0x98, 0x0d, 0x35,
    0x5d, 0xb2, 0xb6, 0xba, 0xaf, 0x8c, 0x96, 0x19, 0x8f, 0x9f, 0xa7, 0x0e, 0x16, 0x1e, 0xb4, 0x9a,
    0xb8, 0xaa, 0x87, 0x2c, 0x54, 0x7c, 0xba, 0xa4, 0xb0, 0x8a, 0xb6, 0x91, 0x32, 0x56, 0x76, 0xb2,
    0x84, 0x94, 0xa4, 0xc8, 0x08, 0xcd, 0xd8, 0xb8, 0xb4, 0xb0, 0xf1, 0x99, 0x82, 0xa8, 0x2d, 0x55,
    0x7d, 0x98, 0xa8, 0x0e, 0x16, 0x1e, 0xa2, 0x2c, 0x54, 0x7c, 0x92, 0xa4, 0xf0, 0x2c, 0x50, 0x78,
    /* bank # 5 */
    0xf1, 0x84, 0xa8, 0x98, 0xc4, 0xcd, 0xfc, 0xd8, 0x0d, 0xdb, 0xa8, 0xfc, 0x2d, 0xf3, 0xd9, 0xba,
    0xa6, 0xf8, 0xda, 0xba, 0xa6, 0xde, 0xd8, 0xba, 0xb2, 0xb6, 0x86, 0x96, 0xa6, 0xd0, 0xf3, 0xc8,
    0x41, 0xda, 0xa6, 0xc8, 0xf8, 0xd8, 0xb0, 0xb4, 0xb8, 0x82, 0xa8, 0x92, 0xf5, 0x2c, 0x54, 0x88,
    0x98, 0xf1, 0x35, 0xd9, 0xf4, 0x18, 0xd8, 0xf1, 0xa2, 0xd0, 0xf8, 0xf9, 0xa8, 0x84, 0xd9, 0xc7,
    0xdf, 0xf8, 0xf8, 0x83, 0xc5, 0xda, 0xdf, 0x69, 0xdf, 0x83, 0xc1, 0xd8, 0xf4, 0x01, 0x14, 0xf1,
    0xa8, 0x82, 0x4e, 0xa8, 0x84, 0xf3, 0x11, 0xd1, 0x82, 0xf5, 0xd9, 0x92, 0x28, 0x97, 0x88, 0xf1,
    0x09, 0xf4, 0x1c, 0x1c, 0xd8, 0x84, 0xa8, 0xf3, 0xc0, 0xf9, 0xd1, 0xd9, 0x97, 0x82, 0xf1, 0x29,
    0xf4, 0x0d, 0xd8, 0xf3, 0xf9, 0xf9, 0xd1, 0xd9, 0x82, 0xf4, 0xc2, 0x03, 0xd8, 0xde, 0xdf, 0x1a,
    0xd8, 0xf1, 0xa2, 0xfa, 0xf9, 0xa8, 0x84, 0x98, 0xd9, 0xc7, 0xdf, 0xf8, 0xf8, 0xf8, 0x83, 0xc7,
    0xda, 0xdf, 0x69, 0xdf, 0xf8, 0x83, 0xc3, 0xd8, 0xf4, 0x01, 0x14, 0xf1, 0x98, 0xa8, 0x82, 0x2e,
    0xa8, 0x84, 0xf3, 0x11, 0xd1, 0x82, 0xf5, 0xd9, 0x92, 0x50, 0x97, 0x88, 0xf1, 0x09, 0xf4, 0x1c,
    0xd8, 0x84, 0xa8, 0xf3, 0xc0, 0xf8, 0xf9, 0xd1, 0xd9, 0x97, 0x82, 0xf1, 0x49, 0xf4, 0x0d, 0xd8,
    0xf3, 0xf9, 0xf9, 0xd1, 0xd9, 0x82, 0xf4, 0xc4, 0x03, 0xd8, 0xde, 0xdf, 0xd8, 0xf1, 0xad, 0x88,
    0x98, 0xcc, 0xa8, 0x09, 0xf9, 0xd9, 0x82, 0x92, 0xa8, 0xf5, 0x7c, 0xf1, 0x88, 0x3a, 0xcf, 0x94,
    0x4a, 0x6e, 0x98, 0xdb, 0x69, 0x31, 0xda, 0xad, 0xf2, 0xde, 0xf9, 0xd8, 0x87, 0x95, 0xa8, 0xf2,
    0x21, 0xd1, 0xda, 0xa5, 0xf9, 0xf4, 0x17, 0xd9, 0xf1, 0xae, 0x8e, 0xd0, 0xc0, 0xc3, 0xae, 0x82,
    /* bank # 6 */
    0xc6, 0x84, 0xc3, 0xa8, 0x85, 0x95, 0xc8, 0xa5, 0x88, 0xf2, 0xc0, 0xf1, 0xf4, 0x01, 0x0e, 0xf1,
    0x8e, 0x9e, 0xa8, 0xc6, 0x3e, 0x56, 0xf5, 0x54, 0xf1, 0x88, 0x72, 0xf4, 0x01, 0x15, 0xf1, 0x98,
    0x45, 0x85, 0x6e, 0xf5, 0x8e, 0x9e, 0x04, 0x88, 0xf1, 0x42, 0x98, 0x5a, 0x8e, 0x9e, 0x06, 0x88,
    0x69, 0xf4, 0x01, 0x1c, 0xf1, 0x98, 0x1e, 0x11, 0x08, 0xd0, 0xf5, 0x04, 0xf1, 0x1e, 0x97, 0x02,
    0x02, 0x98, 0x36, 0x25, 0xdb, 0xf9, 0xd9, 0x85, 0xa5, 0xf3, 0xc1, 0xda, 0x85, 0xa5, 0xf3, 0xdf,
    0xd8, 0x85, 0x95, 0xa8, 0xf3, 0x09, 0xda, 0xa5, 0xfa, 0xd8, 0x82, 0x92, 0xa8, 0xf5, 0x78, 0xf1,
    0x88, 0x1a, 0x84, 0x9f, 0x26, 0x88, 0x98, 0x21, 0xda, 0xf4, 0x1d, 0xf3, 0xd8, 0x87, 0x9f, 0x39,
    0xd1, 0xaf, 0xd9, 0xdf, 0xdf, 0xfb, 0xf9, 0xf4, 0x0c, 0xf3, 0xd8, 0xfa, 0xd0, 0xf8, 0xda, 0xf9,
    0xf9, 0xd0, 0xdf, 0xd9, 0xf9, 0xd8, 0xf4, 0x0b, 0xd8, 0xf3, 0x87, 0x9f, 0x39, 0xd1, 0xaf, 0xd9,
    0xdf, 0xdf, 0xf4, 0x1d, 0xf3, 0xd8, 0xfa, 0xfc, 0xa8, 0x69, 0xf9, 0xf9, 0xaf, 0xd0, 0xda, 0xde,
    0xfa, 0xd9, 0xf8, 0x8f, 0x9f, 0xa8, 0xf1, 0xcc, 0xf3, 0x98, 0xdb, 0x45, 0xd9, 0xaf, 0xdf, 0xd0,
    0xf8, 0xd8, 0xf1, 0x8f, 0x9f, 0xa8, 0xca, 0xf3, 0x88, 0x09, 0xda, 0xaf, 0x8f, 0xcb, 0xf8, 0xd8,
    0xf2, 0xad, 0x97, 0x8d, 0x0c, 0xd9, 0xa5, 0xdf, 0xf9, 0xba, 0xa6, 0xf3, 0xfa, 0xf4, 0x12, 0xf2,
    0xd8, 0x95, 0x0d, 0xd1, 0xd9, 0xba, 0xa6, 0xf3, 0xfa, 0xda, 0xa5, 0xf2, 0xc1, 0xba, 0xa6, 0xf3,
    0xdf, 0xd8, 0xf1, 0xba, 0xb2, 0xb6, 0x86, 0x96, 0xa6, 0xd0, 0xca, 0xf3, 0x49, 0xda, 0xa6, 0xcb,
    0xf8, 0xd8, 0xb0, 0xb4, 0xb8, 0xd8, 0xad, 0x84, 0xf2, 0xc0, 0xdf, 0xf1, 0x8f, 0xcb, 0xc3, 0xa8,
    /* bank # 7 */
    0xb2, 0xb6, 0x86, 0x96, 0xc8, 0xc1, 0xcb, 0xc3, 0xf3, 0xb0, 0xb4, 0x88, 0x98, 0xa8, 0x21, 0xdb,
    0x71, 0x8d, 0x9d, 0x71, 0x85, 0x95, 0x21, 0xd9, 0xad, 0xf2, 0xfa, 0xd8, 0x85, 0x97, 0xa8, 0x28,
    0xd9, 0xf4, 0x08, 0xd8, 0xf2, 0x8d, 0x29, 0xda, 0xf4, 0x05, 0xd9, 0xf2, 0x85, 0xa4, 0xc2, 0xf2,
    0xd8, 0xa8, 0x8d, 0x94, 0x01, 0xd1, 0xd9, 0xf4, 0x11, 0xf2, 0xd8, 0x87, 0x21, 0xd8, 0xf4, 0x0a,
    0xd8, 0xf2, 0x84, 0x98, 0xa8, 0xc8, 0x01, 0xd1, 0xd9, 0xf4, 0x11, 0xd8, 0xf3, 0xa4, 0xc8, 0xbb,
    0xaf, 0xd0, 0xf2, 0xde, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xd8, 0xf1, 0xb8, 0xf6,
    0xb5, 0xb9, 0xb0, 0x8a, 0x95, 0xa3, 0xde, 0x3c, 0xa3, 0xd9, 0xf8, 0xd8, 0x5c, 0xa3, 0xd9, 0xf8,
    0xd8, 0x7c, 0xa3, 0xd9, 0xf8, 0xd8, 0xf8, 0xf9, 0xd1, 0xa5, 0xd9, 0xdf, 0xda, 0xfa, 0xd8, 0xb1,
    0x85, 0x30, 0xf7, 0xd9, 0xde, 0xd8, 0xf8, 0x30, 0xad, 0xda, 0xde, 0xd8, 0xf2, 0xb4, 0x8c, 0x99,
    0xa3, 0x2d, 0x55, 0x7d, 0xa0, 0x83, 0xdf, 0xdf, 0xdf, 0xb5, 0x91, 0xa0, 0xf6, 0x29, 0xd9, 0xfb,
    0xd8, 0xa0, 0xfc, 0x29, 0xd9, 0xfa, 0xd8, 0xa0, 0xd0, 0x51, 0xd9, 0xf8, 0xd8, 0xfc, 0x51, 0xd9,
    0xf9, 0xd8, 0x79, 0xd9, 0xfb, 0xd8, 0xa0, 0xd0, 0xfc, 0x79, 0xd9, 0xfa, 0xd8, 0xa1, 0xf9, 0xf9,
    0xf9, 0xf9, 0xf9, 0xa0, 0xda, 0xdf, 0xdf, 0xdf, 0xd8, 0xa1, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xac,
    0xde, 0xf8, 0xad, 0xde, 0x83, 0x93, 0xac, 0x2c, 0x54, 0x7c, 0xf1, 0xa8, 0xdf, 0xdf, 0xdf, 0xf6,
    0x9d, 0x2c, 0xda, 0xa0, 0xdf, 0xd9, 0xfa, 0xdb, 0x2d, 0xf8, 0xd8, 0xa8, 0x50, 0xda, 0xa0, 0xd0,
    0xde, 0xd9, 0xd0, 0xf8, 0xf8, 0xf8, 0xdb, 0x55, 0xf8, 0xd8, 0xa8, 0x78, 0xda, 0xa0, 0xd0, 0xdf,
    /* bank # 8 */
    0xd9, 0xd0, 0xfa, 0xf8, 0xf8, 0xf8, 0xf8, 0xdb, 0x7d, 0xf8, 0xd8, 0x9c, 0xa8, 0x8c, 0xf5, 0x30,
    0xdb, 0x38, 0xd9, 0xd0, 0xde, 0xdf, 0xa0, 0xd0, 0xde, 0xdf, 0xd8, 0xa8, 0x48, 0xdb, 0x58, 0xd9,
    0xdf, 0xd0, 0xde, 0xa0, 0xdf, 0xd0, 0xde, 0xd8, 0xa8, 0x68, 0xdb, 0x70, 0xd9, 0xdf, 0xdf, 0xa0,
    0xdf, 0xdf, 0xd8, 0xf1, 0xa8, 0x88, 0x90, 0x2c, 0x54, 0x7c, 0x98, 0xa8, 0xd0, 0x5c, 0x38, 0xd1,
    0xda, 0xf2, 0xae, 0x8c, 0xdf, 0xf9, 0xd8, 0xb0, 0x87, 0xa8, 0xc1, 0xc1, 0xb1, 0x88, 0xa8, 0xc6,
    0xf9, 0xf9, 0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xa8,
    0xf9, 0xda, 0x36, 0xd8, 0xa8, 0xf9, 0xda, 0x36, 0xd8, 0xf7, 0x8d, 0x9d, 0xad, 0xf8, 0x18, 0xda,
    0xf2, 0xae, 0xdf, 0xd8, 0xf7, 0xad, 0xfa, 0x30, 0xd9, 0xa4, 0xde, 0xf9, 0xd8, 0xf2, 0xae, 0xde,
    0xfa, 0xf9, 0x83, 0xa7, 0xd9, 0xc3, 0xc5, 0xc7, 0xf1, 0x88, 0x9b, 0xa7, 0x7a, 0xad, 0xf7, 0xde,
    0xdf, 0xa4, 0xf8, 0x84, 0x94, 0x08, 0xa7, 0x97, 0xf3, 0x00, 0xae, 0xf2, 0x98, 0x19, 0xa4, 0x88,
    0xc6, 0xa3, 0x94, 0x88, 0xf6, 0x32, 0xdf, 0xf2, 0x83, 0x93, 0xdb, 0x09, 0xd9, 0xf2, 0xaa, 0xdf,
    0xd8, 0xd8, 0xae, 0xf8, 0xf9, 0xd1, 0xda, 0xf3, 0xa4, 0xde, 0xa7, 0xf1, 0x88, 0x9b, 0x7a, 0xd8,
    0xf3, 0x84, 0x94, 0xae, 0x19, 0xf9, 0xda, 0xaa, 0xf1, 0xdf, 0xd8, 0xa8, 0x81, 0xc0, 0xc3, 0xc5,
    0xc7, 0xa3, 0x92, 0x83, 0xf6, 0x28, 0xad, 0xde, 0xd9, 0xf8, 0xd8, 0xa3, 0x50, 0xad, 0xd9, 0xf8,
    0xd8, 0xa3, 0x78, 0xad, 0xd9, 0xf8, 0xd8, 0xf8, 0xf9, 0xd1, 0xa1, 0xda, 0xde, 0xc3, 0xc5, 0xc7,
    0xd8, 0xa1, 0x81, 0x94, 0xf8, 0x18, 0xf2, 0xb0, 0x89, 0xac, 0xc3, 0xc5, 0xc7, 0xf1, 0xd8, 0xb8,
    /* bank # 9 */
    0xb4, 0xb0, 0x97, 0x86, 0xa8, 0x31, 0x9b, 0x06, 0x99, 0x07, 0xab, 0x97, 0x28, 0x88, 0x9b, 0xf0,
    0x0c, 0x20, 0x14, 0x40, 0xb0, 0xb4, 0xb8, 0xf0, 0xa8, 0x8a, 0x9a, 0x28, 0x50, 0x78, 0xb7, 0x9b,
    0xa8, 0x29, 0x51, 0x79, 0x24, 0x70, 0x59, 0x44, 0x69, 0x38, 0x64, 0x48, 0x31, 0xf1, 0xbb, 0xab,
    0x88, 0x00, 0x2c, 0x54, 0x7c, 0xf0, 0xb3, 0x8b, 0xb8, 0xa8, 0x04, 0x28, 0x50, 0x78, 0xf1, 0xb0,
    0x88, 0xb4, 0x97, 0x26, 0xa8, 0x59, 0x98, 0xbb, 0xab, 0xb3, 0x8b, 0x02, 0x26, 0x46, 0x66, 0xb0,
    0xb8, 0xf0, 0x8a, 0x9c, 0xa8, 0x29, 0x51, 0x79, 0x8b, 0x29, 0x51, 0x79, 0x8a, 0x24, 0x70, 0x59,
    0x8b, 0x20, 0x58, 0x71, 0x8a, 0x44, 0x69, 0x38, 0x8b, 0x39, 0x40, 0x68, 0x8a, 0x64, 0x48, 0x31,
    0x8b, 0x30, 0x49, 0x60, 0x88, 0xf1, 0xac, 0x00, 0x2c, 0x54, 0x7c, 0xf0, 0x8c, 0xa8, 0x04, 0x28,
    0x50, 0x78, 0xf1, 0x88, 0x97, 0x26, 0xa8, 0x59, 0x98, 0xac, 0x8c, 0x02, 0x26, 0x46, 0x66, 0xf0,
    0x89, 0x9c, 0xa8, 0x29, 0x51, 0x79, 0x24, 0x70, 0x59, 0x44, 0x69, 0x38, 0x64, 0x48, 0x31, 0xa9,
    0x88, 0x09, 0x20, 0x59, 0x70, 0xab, 0x11, 0x38, 0x40, 0x69, 0xa8, 0x19, 0x31, 0x48, 0x60, 0x8c,
    0xa8, 0x3c, 0x41, 0x5c, 0x20, 0x7c, 0x00, 0xf1, 0x87, 0x98, 0x19, 0x86, 0xa8, 0x6e, 0x76, 0x7e,
    0xa9, 0x99, 0x88, 0x2d, 0x55, 0x7d, 0xd8, 0xb1, 0xb5, 0xb9, 0xa3, 0xdf, 0xdf, 0xdf, 0xae, 0xd0,
    0xdf, 0xaa, 0xd0, 0xde, 0xf2, 0xab, 0xf8, 0xf9, 0xd9, 0xb0, 0x87, 0xc4, 0xaa, 0xf1, 0xdf, 0xdf,
    0xbb, 0xaf, 0xdf, 0xdf, 0xb9, 0xd8, 0xb1, 0xf1, 0xa3, 0x97, 0x8e, 0x60, 0xdf, 0xb0, 0x84, 0xf2,
    0xc8, 0xf8, 0xf9, 0xd9, 0xde, 0xd8, 0x93, 0x85, 0xf1, 0x4a, 0xb1, 0x83, 0xa3, 0x08, 0xb5, 0x83,
    /* bank # 10 */
    0x9a, 0x08, 0x10, 0xb7, 0x9f, 0x10, 0xd8, 0xf1, 0xb0, 0xba, 0xae, 0xb0, 0x8a, 0xc2, 0xb2, 0xb6,
    0x8e, 0x9e, 0xf1, 0xfb, 0xd9, 0xf4, 0x1d, 0xd8, 0xf9, 0xd9, 0x0c, 0xf1, 0xd8, 0xf8, 0xf8, 0xad,
    0x61, 0xd9, 0xae, 0xfb, 0xd8, 0xf4, 0x0c, 0xf1, 0xd8, 0xf8, 0xf8, 0xad, 0x19, 0xd9, 0xae, 0xfb,
    0xdf, 0xd8, 0xf4, 0x16, 0xf1, 0xd8, 0xf8, 0xad, 0x8d, 0x61, 0xd9, 0xf4, 0xf4, 0xac, 0xf5, 0x9c,
    0x9c, 0x8d, 0xdf, 0x2b, 0xba, 0xb6, 0xae, 0xfa, 0xf8, 0xf4, 0x0b, 0xd8, 0xf1, 0xae, 0xd0, 0xf8,
    0xad, 0x51, 0xda, 0xae, 0xfa, 0xf8, 0xf1, 0xd8, 0xb9, 0xb1, 0xb6, 0xa3, 0x83, 0x9c, 0x08, 0xb9,
    0xb1, 0x83, 0x9a, 0xb5, 0xaa, 0xc0, 0xfd, 0x30, 0x83, 0xb7, 0x9f, 0x10, 0xb5, 0x8b, 0x93, 0xf2,
    0x02, 0x02, 0xd1, 0xab, 0xda, 0xde, 0xd8, 0xf1, 0xb0, 0x80, 0xba, 0xab, 0xc0, 0xc3, 0xb2, 0x84,
    0xc1, 0xc3, 0xd8, 0xb1, 0xb9, 0xf3, 0x8b, 0xa3, 0x91, 0xb6, 0x09, 0xb4, 0xd9, 0xab, 0xde, 0xb0,
    0x87, 0x9c, 0xb9, 0xa3, 0xdd, 0xf1, 0xb3, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0xb0, 0x87, 0xa3, 0xa3,
    0xa3, 0xa3, 0xb2, 0x8b, 0xb6, 0x9b, 0xf2, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
    0xa3, 0xf1, 0xb0, 0x87, 0xb5, 0x9a, 0xa3, 0xf3, 0x9b, 0xa3, 0xa3, 0xdc, 0xba, 0xac, 0xdf, 0xb9,
    0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3,
    0xd8, 0xd8, 0xd8, 0xbb, 0xb3, 0xb7, 0xf1, 0xaa, 0xf9, 0xda, 0xff, 0xd9, 0x80, 0x9a, 0xaa, 0x28,
    0xb4, 0x80, 0x98, 0xa7, 0x20, 0xb7, 0x97, 0x87, 0xa8, 0x66, 0x88, 0xf0, 0x79, 0x51, 0xf1, 0x90,
    0x2c, 0x87, 0x0c, 0xa7, 0x81, 0x97, 0x62, 0x93, 0xf0, 0x71, 0x71, 0x60, 0x85, 0x94, 0x01, 0x29,
    /* bank # 11 */
    0x51, 0x79, 0x90, 0xa5, 0xf1, 0x28, 0x4c, 0x6c, 0x87, 0x0c, 0x95, 0x18, 0x85, 0x78, 0xa3, 0x83,
    0x90, 0x28, 0x4c, 0x6c, 0x88, 0x6c, 0xd8, 0xf3, 0xa2, 0x82, 0x00, 0xf2, 0x10, 0xa8, 0x92, 0x19,
    0x80, 0xa2, 0xf2, 0xd9, 0x26, 0xd8, 0xf1, 0x88, 0xa8, 0x4d, 0xd9, 0x48, 0xd8, 0x96, 0xa8, 0x39,
    0x80, 0xd9, 0x3c, 0xd8, 0x95, 0x80, 0xa8, 0x39, 0xa6, 0x86, 0x98, 0xd9, 0x2c, 0xda, 0x87, 0xa7,
    0x2c, 0xd8, 0xa8, 0x89, 0x95, 0x19, 0xa9, 0x80, 0xd9, 0x38, 0xd8, 0xa8, 0x89, 0x39, 0xa9, 0x80,
    0xda, 0x3c, 0xd8, 0xa8, 0x2e, 0xa8, 0x39, 0x90, 0xd9, 0x0c, 0xd8, 0xa8, 0x95, 0x31, 0x98, 0xd9,
    0x0c, 0xd8, 0xa8, 0x09, 0xd9, 0xff, 0xd8, 0x01, 0xda, 0xff, 0xd8, 0x95, 0x39, 0xa9, 0xda, 0x26,
    0xff, 0xd8, 0x90, 0xa8, 0x0d, 0x89, 0x99, 0xa8, 0x10, 0x80, 0x98, 0x21, 0xda, 0x2e, 0xd8, 0x89,
    0x99, 0xa8, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8, 0x86, 0x96, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8,
    0x87, 0x31, 0x80, 0xda, 0x2e, 0xd8, 0xa8, 0x82, 0x92, 0xf3, 0x41, 0x80, 0xf1, 0xd9, 0x2e, 0xd8,
    0xa8, 0x82, 0xf3, 0x19, 0x80, 0xf1, 0xd9, 0x2e, 0xd8, 0x82, 0xac, 0xf3, 0xc0, 0xa2, 0x80, 0x22,
    0xf1, 0xa6, 0x2e, 0xa7, 0x2e, 0xa9, 0x22, 0x98, 0xa8, 0x29, 0xda, 0xac, 0xde, 0xff, 0xd8, 0xa2,
    0xf2, 0x2a, 0xf1, 0xa9, 0x2e, 0x82, 0x92, 0xa8, 0xf2, 0x31, 0x80, 0xa6, 0x96, 0xf1, 0xd9, 0x00,
    0xac, 0x8c, 0x9c, 0x0c, 0x30, 0xac, 0xde, 0xd0, 0xde, 0xff, 0xd8, 0x8c, 0x9c, 0xac, 0xd0, 0x10,
    0xac, 0xde, 0x80, 0x92, 0xa2, 0xf2, 0x4c, 0x82, 0xa8, 0xf1, 0xca, 0xf2, 0x35, 0xf1, 0x96, 0x88,
    0xa6, 0xd9, 0x00, 0xd8, 0xf1, 0xff
};

bool AgmMpu9250::Init(uint32_t DevAddr, DeviceIntrf *pIntrf, Timer *pTimer)
{
	if (vbInitialized)
		return true;;

	if (pIntrf == NULL)
		return false;

	uint8_t regaddr;
	uint8_t d;
	uint8_t userctrl = /*MPU9250_AG_USER_CTRL_FIFO_EN | MPU9250_AG_USER_CTRL_DMP_EN |*/ MPU9250_AG_USER_CTRL_I2C_MST_EN;
	uint8_t mst = 0;

	Interface(pIntrf);
	DeviceAddess(DevAddr);

	if (pTimer != NULL)
	{
		vpTimer = pTimer;
	}

/*	if (DevAddr == MPU9250_I2C_DEV_ADDR0 || DevAddr == MPU9250_I2C_DEV_ADDR1)
	{
		// I2C mode
		vbSpi = false;
	}
	else*/
	if (pIntrf->Type() == DEVINTRF_TYPE_SPI)
	{
//		vbSpi = true;

		// in SPI mode, use i2c master mode to access Mag device (AK8963C)
		userctrl |= MPU9250_AG_USER_CTRL_I2C_MST_EN | MPU9250_AG_USER_CTRL_I2C_IF_DIS;
		mst = MPU9250_AG_I2C_MST_CTRL_WAIT_FOR_ES | 13;
	}

	// Read chip id
	regaddr = MPU9250_AG_WHO_AM_I;
	d = Read8((uint8_t*)&regaddr, 1);

	if (d != MPU9250_AG_WHO_AM_I_ID)
	{
		return false;
	}

	Reset();

	DeviceID(d);
	Valid(true);

	// NOTE : require delay for reset to stabilize
	// the chip would not respond properly to motion detection
	usDelay(500000);

	//UploadDMPImage();

	// Disable all interrupt
	regaddr = MPU9250_AG_INT_ENABLE;
	Write8(&regaddr, 1, 0);

	//regaddr = MPU9250_AG_CONFIG;
	//Write8(&regaddr, 1, MPU9250_AG_CONFIG_FIFO_MODE_BLOCKING);

	vbInitialized = true;

	//regaddr = MPU9250_AG_PWR_MGMT_1;
	//Write8(&regaddr, 1, MPU9250_AG_PWR_MGMT_1_SLEEP);
	//return true;

	// Init master I2C interface
	regaddr = MPU9250_AG_USER_CTRL;
	Write8(&regaddr, 1, userctrl);

	regaddr = MPU9250_AG_I2C_MST_CTRL;
	Write8(&regaddr, 1, mst);

	// Enable FIFO

    // Undocumented register
	// shares 4kB of memory between the DMP and the FIFO. Since the
    // first 3kB are needed by the DMP, we'll use the last 1kB for the FIFO.
	regaddr = MPU9250_AG_ACCEL_CONFIG2;
	Write8(&regaddr, 1, MPU9250_AG_ACCEL_CONFIG2_ACCEL_FCHOICE_B | MPU9250_AG_ACCEL_CONFIG2_FIFO_SIZE_1024);


	//regaddr = MPU9250_AG_FIFO_EN;
	//Write8(&regaddr, 1, MPU9250_AG_FIFO_EN_ACCEL |
	//		MPU9250_AG_FIFO_EN_GYRO_ZOUT | MPU9250_AG_FIFO_EN_GYRO_YOUT |
	//		MPU9250_AG_FIFO_EN_GYRO_XOUT | MPU9250_AG_FIFO_EN_TEMP_OUT);


	return true;
}

bool AgmMpu9250::Init(const ACCELSENSOR_CFG &CfgData, DeviceIntrf *pIntrf, Timer *pTimer)
{
	uint8_t regaddr;
	uint8_t d;

	if (Init(CfgData.DevAddr, pIntrf, pTimer) == false)
		return false;

	regaddr = MPU9250_AG_LP_ACCEL_ODR;

	if (CfgData.Freq < 400)
	{
		Write8(&regaddr, 1, 0);
		vSampFreq = 240;	// 0.24 Hz
	}
	else if (CfgData.Freq < 900)
	{
		Write8(&regaddr, 1, 1);
		vSampFreq = 490;	// 0.49 Hz
	}
	else if (CfgData.Freq < 1500)
	{
		Write8(&regaddr, 1, 2);
		vSampFreq = 980;	// 0.98 Hz
	}
	else if (CfgData.Freq < 2500)
	{
		Write8(&regaddr, 1, 3);
		vSampFreq = 1950;	// 1.95 Hz
	}
	else if (CfgData.Freq < 3500)
	{
		Write8(&regaddr, 1, 4);
		vSampFreq = 3910;	// 3.91 Hz
	}
	else if (CfgData.Freq < 10000)
	{
		Write8(&regaddr, 1, 5);
		vSampFreq = 7810;	// 7.81 Hz
	}
	else if (CfgData.Freq < 20000)
	{
		Write8(&regaddr, 1, 6);
		vSampFreq = 15630;	// 15.63 Hz
	}
	else if (CfgData.Freq < 50000)
	{
		Write8(&regaddr, 1, 7);
		vSampFreq = 31250;	// 31.25 Hz
	}
	else if (CfgData.Freq < 100000)
	{
		Write8(&regaddr, 1, 8);
		vSampFreq = 62500;	// 62.5 Hz
	}
	else if (CfgData.Freq < 200000)
	{
		Write8(&regaddr, 1, 9);
		vSampFreq = 125000;	// 125 Hz
	}
	else if (CfgData.Freq < 500)
	{
		Write8(&regaddr, 1, 10);
		vSampFreq = 250000;	// 250 Hz
	}
	else
	{
		Write8(&regaddr, 1, 11);
		vSampFreq = 500000;	// 500 Hz
	}

	AccelSensor::Range(MPU9250_ACC_MAX_RANGE);
	Scale(CfgData.Scale);
	LowPassFreq(vSampFreq / 2000);

	regaddr = MPU9250_AG_INT_ENABLE;
	Write8(&regaddr, 1, MPU9250_AG_INT_ENABLE_RAW_RDY_EN);

	msDelay(100);

	regaddr = MPU9250_AG_PWR_MGMT_1;
	Write8(&regaddr, 1, MPU9250_AG_PWR_MGMT_1_CYCLE | MPU9250_AG_PWR_MGMT_1_CLKSEL_AUTO);

	vbSensorEnabled[0] = true;

	return true;
}

bool AgmMpu9250::Init(const GYROSENSOR_CFG &CfgData, DeviceIntrf *pIntrf, Timer *pTimer)
{
	if (Init(CfgData.DevAddr, pIntrf, pTimer) == false)
		return false;

	uint8_t regaddr;
	uint8_t d = 0;
	uint8_t fchoice = 0;
	uint32_t f = CfgData.Freq >> 1;

	if (f == 0)
	{
		fchoice = 1;
	}
	if (f < 10000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_5HZ;
	}
	else if (f < 20000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_10HZ;
	}
	else if (f < 30000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_20HZ;
	}
	else if (f < 60000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_41HZ;
	}
	else if (f < 150000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_92HZ;
	}
	else if (f < 220000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_184HZ;
	}
	else if (f < 40000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_250HZ;
	}
	else if (f < 400000)
	{
		d = MPU9250_AG_CONFIG_DLPF_CFG_3600HZ;
	}
	else
	{
		// 8800Hz
		fchoice = 1;
	}

	regaddr = MPU9250_AG_CONFIG;
	Write8(&regaddr, 1, d);

	regaddr = MPU9250_AG_GYRO_CONFIG;
	Write8(&regaddr, 1, fchoice);

	Sensitivity(CfgData.Sensitivity);

	vbSensorEnabled[1] = true;

	//UploadDMPImage();

	return true;
}

bool AgmMpu9250::Init(const MAGSENSOR_CFG &CfgData, DeviceIntrf *pIntrf, Timer *pTimer)
{
	uint8_t regaddr;
	uint8_t d[4];

	if (Init(CfgData.DevAddr, pIntrf, pTimer) == false)
		return false;

	msDelay(200);

	regaddr = MPU9250_MAG_WIA;
	Read(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, d, 1);

	if (d[0] != MPU9250_MAG_WIA_DEVICE_ID)
	{
		return false;
	}

	msDelay(1);

	// Read ROM sensitivity adjustment values
	regaddr = MPU9250_MAG_CTRL1;
	d[0] = MPU9250_MAG_CTRL1_MODE_PWRDOWN;
	Write(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, d, 1);

	msDelay(1);

	d[0] = MPU9250_MAG_CTRL1_MODE_FUSEROM_ACCESS;
	Write(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, d, 1);

	msDelay(100);

	regaddr = MPU9250_MAG_ASAX;
	Read(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, d, 3);

	vMagSenAdj[0] = (int16_t)d[0] - 128;
	vMagSenAdj[1] = (int16_t)d[1] - 128;
	vMagSenAdj[2] = (int16_t)d[2] - 128;

	// Transition out of reading ROM
	regaddr = MPU9250_MAG_CTRL1;
	d[0] = MPU9250_MAG_CTRL1_MODE_PWRDOWN;
	Write(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, d, 1);

	MagSensor::vPrecision = 14;
	vMagCtrl1Val = 0;
	MagSensor::vScale = 8190;

	if (CfgData.Precision >= 16)
	{
		MagSensor::vPrecision = 16;
		MagSensor::vScale = 32760;
		vMagCtrl1Val = MPU9250_MAG_CTRL1_BIT_16;
	}

	if (CfgData.OpMode == SENSOR_OPMODE_CONTINUOUS)
	{
		if (CfgData.Freq < 50000)
		{
			// Select 8Hz
			vMagCtrl1Val |= MPU9250_MAG_CTRL1_MODE_8HZ;
			MagSensor::Mode(CfgData.OpMode, 8000000);
		}
		else
		{
			// Select 100Hz
			vMagCtrl1Val |= MPU9250_MAG_CTRL1_MODE_100HZ;
			MagSensor::Mode(CfgData.OpMode, 100000000);
		}
	}
	else
	{
		vMagCtrl1Val |= MPU9250_MAG_CTRL1_MODE_SINGLE;
		MagSensor::Mode(CfgData.OpMode, 0);
	}

	msDelay(10);

	regaddr = MPU9250_MAG_CTRL1;
	Write(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, &vMagCtrl1Val, 1);

	vbSensorEnabled[2] = true;

	return true;
}

bool AgmMpu9250::Enable()
{
	uint8_t regaddr = MPU9250_AG_PWR_MGMT_1;
	uint8_t d;

	Write8(&regaddr, 1, MPU9250_AG_PWR_MGMT_1_CYCLE | MPU9250_AG_PWR_MGMT_1_GYRO_STANDBY |
			MPU9250_AG_PWR_MGMT_1_CLKSEL_INTERNAL);

	regaddr = MPU9250_AG_PWR_MGMT_2;


	d = 0;

	// Enable Accel & Gyro
	if (vbSensorEnabled[0] == false)
	{
		d |= MPU9250_AG_PWR_MGMT_2_DIS_ZA |
			 MPU9250_AG_PWR_MGMT_2_DIS_YA |
			 MPU9250_AG_PWR_MGMT_2_DIS_XA;
	}
	if (vbSensorEnabled[1] == false)
	{
		d |= MPU9250_AG_PWR_MGMT_2_DIS_ZG |
			 MPU9250_AG_PWR_MGMT_2_DIS_YG |
			 MPU9250_AG_PWR_MGMT_2_DIS_XG;
	}
	Write8(&regaddr, 1, d);

	if (vbSensorEnabled[2] == true)
	{
		printf("Mag Enabled\r\n");

	}

	// Enable Mag
	//regaddr = MPU9250_MAG_CTRL1;
	//Write(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, &vMagCtrl1Val, 1);

	return true;
}

void AgmMpu9250::Disable()
{
	uint8_t regaddr = MPU9250_AG_PWR_MGMT_2;
	uint8_t d;
//Reset();
//msDelay(2000);

	regaddr = MPU9250_AG_PWR_MGMT_1;
	d = Read8(&regaddr, 1);
	d |= MPU9250_AG_PWR_MGMT_1_SLEEP;
	Write8(&regaddr, 1, d);//MPU9250_AG_PWR_MGMT_1_SLEEP | MPU9250_AG_PWR_MGMT_1_PD_PTAT |
						//MPU9250_AG_PWR_MGMT_1_CYCLE | 3);//MPU9250_AG_PWR_MGMT_1_GYRO_STANDBY);

	return;

	regaddr = MPU9250_AG_USER_CTRL;
	Write8(&regaddr, 1, MPU9250_AG_USER_CTRL_I2C_MST_EN);

	// Disable Mag
	regaddr = MPU9250_MAG_CTRL1;
	//uint8_t d = 0;
	Write(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, &d, 1);

	// Disable Accel Gyro
	Write8(&regaddr, 1,
		 MPU9250_AG_PWR_MGMT_2_DIS_ZG | MPU9250_AG_PWR_MGMT_2_DIS_YG | MPU9250_AG_PWR_MGMT_2_DIS_XG |
		 MPU9250_AG_PWR_MGMT_2_DIS_ZA | MPU9250_AG_PWR_MGMT_2_DIS_YA | MPU9250_AG_PWR_MGMT_2_DIS_XA);

	regaddr = MPU9250_AG_PWR_MGMT_1;
	d = Read8(&regaddr, 1);
	d |= MPU9250_AG_PWR_MGMT_1_SLEEP;
	Write8(&regaddr, 1, d);//MPU9250_AG_PWR_MGMT_1_SLEEP | MPU9250_AG_PWR_MGMT_1_PD_PTAT |
//	Write8(&regaddr, 1, MPU9250_AG_PWR_MGMT_1_SLEEP | MPU9250_AG_PWR_MGMT_1_PD_PTAT |
//						MPU9250_AG_PWR_MGMT_1_GYRO_STANDBY);

}

void AgmMpu9250::Reset()
{
	uint8_t regaddr = MPU9250_AG_PWR_MGMT_1;

	Write8(&regaddr, 1, MPU9250_AG_PWR_MGMT_1_H_RESET);
}

bool AgmMpu9250::StartSampling()
{
	return true;
}

// Implement wake on motion
bool AgmMpu9250::WakeOnEvent(bool bEnable, int Threshold)
{
    uint8_t regaddr;

	if (bEnable == true)
	{
		Reset();

		msDelay(2000);

	    regaddr = MPU9250_AG_PWR_MGMT_1;
	    Write8(&regaddr, 1, 0);

	    regaddr = MPU9250_AG_PWR_MGMT_2;
		Write8(&regaddr, 1, /**MPU9250_AG_PWR_MGMT_2_DIS_XA | MPU9250_AG_PWR_MGMT_2_DIS_YA |*/
				MPU9250_AG_PWR_MGMT_2_DIS_XG | MPU9250_AG_PWR_MGMT_2_DIS_YG | MPU9250_AG_PWR_MGMT_2_DIS_ZG);

		regaddr = MPU9250_AG_ACCEL_CONFIG2;
	    Write8(&regaddr, 1, MPU9250_AG_ACCEL_CONFIG2_ACCEL_FCHOICE_B | MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_5HZ);

	    regaddr = MPU9250_AG_INT_ENABLE;
	    Write8(&regaddr, 1, MPU9250_AG_INT_ENABLE_WOM_EN);

	    regaddr = MPU9250_AG_MOT_DETECT_CTRL;
	    Write8(&regaddr, 1, MPU9250_AG_MOT_DETECT_CTRL_ACCEL_INTEL_MODE | MPU9250_AG_MOT_DETECT_CTRL_ACCEL_INTEL_EN);

	    regaddr = MPU9250_AG_WOM_THR;
	    Write8(&regaddr, 1, Threshold);

	    regaddr = MPU9250_AG_LP_ACCEL_ODR;
	    Write8(&regaddr, 1, 0);

		regaddr = MPU9250_AG_PWR_MGMT_1;
		Write8(&regaddr, 1, MPU9250_AG_PWR_MGMT_1_CYCLE);

	}
	else
	{
	    regaddr = MPU9250_AG_INT_ENABLE;
	    Write8(&regaddr, 1, 0);

	    regaddr = MPU9250_AG_PWR_MGMT_1;
		Write8(&regaddr, 1, 0);
	}

	return true;
}

// Accel low pass frequency
uint32_t AgmMpu9250::LowPassFreq(uint32_t Freq)
{
	uint8_t regaddr = MPU9250_AG_ACCEL_CONFIG2;
	uint d = 0;

	if (Freq == 0)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_ACCEL_FCHOICE_B;
		AccelSensor::LowPassFreq(1130);
	}
	else if (Freq < 10)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_5HZ;
		AccelSensor::LowPassFreq(5);
	}
	else if (Freq < 20)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_10HZ;
		AccelSensor::LowPassFreq(10);
	}
	else if (Freq < 40)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_20HZ;
		AccelSensor::LowPassFreq(20);
	}
	else if (Freq < 50)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_41HZ;
		AccelSensor::LowPassFreq(41);
	}
	else if (Freq < 100)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_92HZ;
		AccelSensor::LowPassFreq(92);
	}
	else if (Freq < 200)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_184HZ;
		AccelSensor::LowPassFreq(184);
	}
	else if (Freq < 500)
	{
		d = MPU9250_AG_ACCEL_CONFIG2_A_DLPFCFG_460HZ;
		AccelSensor::LowPassFreq(460);
	}
	else
	{
		d = MPU9250_AG_ACCEL_CONFIG2_ACCEL_FCHOICE_B;
		AccelSensor::LowPassFreq(1130);
	}

	Write8(&regaddr, 1, d | MPU9250_AG_ACCEL_CONFIG2_FIFO_SIZE_1024);

	return AccelSensor::LowPassFreq();
}

// Accel scale
uint16_t AgmMpu9250::Scale(uint16_t Value)
{
	uint8_t regaddr = MPU9250_AG_ACCEL_CONFIG;

	if (Value < 3)
	{
		Write8(&regaddr, 1, MPU9250_AG_ACCEL_CONFIG_ACCEL_FS_SEL_2G);
		AccelSensor::Scale(2);
	}
	else if (Value < 6)
	{
		Write8(&regaddr, 1, MPU9250_AG_ACCEL_CONFIG_ACCEL_FS_SEL_4G);
		AccelSensor::Scale(4);
	}
	else if (Value < 12)
	{
		Write8(&regaddr, 1, MPU9250_AG_ACCEL_CONFIG_ACCEL_FS_SEL_8G);
		AccelSensor::Scale(8);
	}
	else
	{
		Write8(&regaddr, 1, MPU9250_AG_ACCEL_CONFIG_ACCEL_FS_SEL_16G);
		AccelSensor::Scale(16);
	}

	return AccelSensor::Scale();
}

// Gyro scale
uint32_t AgmMpu9250::Sensitivity(uint32_t Value)
{
	uint8_t regaddr = MPU9250_AG_GYRO_CONFIG;
	uint8_t d = Read8(&regaddr, 1) & MPU9250_AG_GYRO_CONFIG_FCHOICE_MASK;

	if (Value < 500)
	{
		d |= MPU9250_AG_GYRO_CONFIG_GYRO_FS_SEL_250DPS;
		GyroSensor::Sensitivity(250);
	}
	else if (Value < 1000)
	{
		d |= MPU9250_AG_GYRO_CONFIG_GYRO_FS_SEL_500DPS;
		GyroSensor::Sensitivity(500);
	}
	else if (Value < 2000)
	{
		d |= MPU9250_AG_GYRO_CONFIG_GYRO_FS_SEL_1000DPS;
		GyroSensor::Sensitivity(1000);
	}
	else
	{
		d |= MPU9250_AG_GYRO_CONFIG_GYRO_FS_SEL_2000DPS;
		GyroSensor::Sensitivity(2000);
	}

	Write8(&regaddr, 1, d);

	return GyroSensor::Sensitivity();
}

bool AgmMpu9250::UpdateData()
{
	uint8_t regaddr = MPU9250_AG_FIFO_COUNT_H;//MPU9250_AG_ACCEL_XOUT_H;
	uint8_t d[20];
	int32_t val;

	Read(&regaddr, 1, (uint8_t*)d, 2);
	val = ((d[0] & 0xF) << 8) | d[1];

	//printf("%d\r\n", val);

	if (val > 0)
	{
		int cnt = min(val, 18);
		regaddr = MPU9250_AG_FIFO_R_W;
	//	Read(&regaddr, 1, d, cnt);
	}

	vSampleCnt++;

	if (vpTimer)
	{
		vSampleTime = vpTimer->uSecond();
	}

	regaddr = MPU9250_AG_ACCEL_XOUT_H;
	Read(&regaddr, 1, (uint8_t*)d, 6);

	AccelSensor::vData.Scale =  AccelSensor::Scale();
	AccelSensor::vData.Range = 0x7FFF;
	AccelSensor::vData.X = ((int32_t)d[0] << 8) | d[1];
	AccelSensor::vData.Y = ((int32_t)d[2] << 8) | d[3];
	AccelSensor::vData.Z = ((int32_t)d[4] << 8) | d[5];
	AccelSensor::vData.Timestamp = vSampleTime;

	regaddr = MPU9250_AG_GYRO_XOUT_H;
	Read(&regaddr, 1, (uint8_t*)d, 6);

	GyroSensor::vData.X = ((int32_t)d[0] << 8) | d[1];
	GyroSensor::vData.Y = ((int32_t)d[2] << 8) | d[3];
	GyroSensor::vData.Z = ((int32_t)d[4] << 8) | d[5];
	GyroSensor::vData.Timestamp = vSampleTime;

/*	val = ((((int16_t)d[0] << 8) | d[1]) << 8) / GyroSensor::vSensitivity;
	GyroSensor::vData.X = val;
	val = ((((int16_t)d[2] << 8) | d[3]) << 8) / GyroSensor::vSensitivity;
	GyroSensor::vData.Y = val;
	val = ((((int32_t)d[4] << 8) | d[5]) << 8L) / GyroSensor::vSensitivity;
	GyroSensor::vData.Z = val;*/

	regaddr = MPU9250_MAG_ST1;
	Read(MPU9250_MAG_I2C_DEVADDR, &regaddr, 1, (uint8_t*)d, 8);

	if (d[14] & MPU9250_MAG_ST1_DRDY)
	{
		val = (((int16_t)d[0]) << 8L) | d[1];
		val += (val * vMagSenAdj[0]) >> 8L;
		MagSensor::vData.X = (int16_t)(val * (MPU9250_MAG_MAX_FLUX_DENSITY << 8) / MagSensor::vScale);

		val = (((int16_t)d[2]) << 8) | d[3];
		val += (val * vMagSenAdj[1]) >> 8L;
		MagSensor::vData.Y = (int16_t)(val * (MPU9250_MAG_MAX_FLUX_DENSITY << 8) / MagSensor::vScale);

		val = (((int16_t)d[4]) << 8) | d[5];
		val += (val * vMagSenAdj[2]) >> 8L;
		MagSensor::vData.Z = (int16_t)(val * (MPU9250_MAG_MAX_FLUX_DENSITY << 8) / MagSensor::vScale);

		MagSensor::vData.Timestamp = vSampleTime;
	}

	return true;
}
/*
bool AgmMpu9250::Read(ACCELSENSOR_DATA &Data)
{
	Data = AccelSensor::vData;

	return true;
}

bool AgmMpu9250::Read(GYROSENSOR_DATA &Data)
{
	Data = GyroSensor::vData;

	return true;
}

bool AgmMpu9250::Read(MAGSENSOR_DATA &Data)
{
	Data = MagSensor::vData;

	return true;
}
*/
int AgmMpu9250::Read(uint8_t *pCmdAddr, int CmdAddrLen, uint8_t *pBuff, int BuffLen)
{
	if (vpIntrf->Type() == DEVINTRF_TYPE_SPI)
	{
		*pCmdAddr |= 0x80;
	}

	return Device::Read(pCmdAddr, CmdAddrLen, pBuff, BuffLen);
}


int AgmMpu9250::Write(uint8_t *pCmdAddr, int CmdAddrLen, uint8_t *pData, int DataLen)
{
	if (vpIntrf->Type() == DEVINTRF_TYPE_SPI)
	{
		*pCmdAddr &= 0x7F;
	}

	return Device::Write(pCmdAddr, CmdAddrLen, pData, DataLen);
}

int AgmMpu9250::Read(uint8_t DevAddr, uint8_t *pCmdAddr, int CmdAddrLen, uint8_t *pBuff, int BuffLen)
{
	int retval = 0;

	if (vpIntrf->Type() == DEVINTRF_TYPE_SPI)
	{
		uint8_t regaddr;
		uint8_t d[8];

		d[0] = MPU9250_AG_I2C_SLV0_ADDR;
		d[1] = DevAddr | MPU9250_AG_I2C_SLV0_ADDR_I2C_SLVO_RD;
		d[2] = *pCmdAddr;

		while (BuffLen > 0)
		{
			int cnt = min(15, BuffLen);

			d[3] = MPU9250_AG_I2C_SLV0_CTRL_I2C_SLV0_EN |cnt;

			Write(d, 4, NULL, 0);

			// Delay require for transfer to complete
			//usDelay(500 + (cnt << 4));
			msDelay(1);

			regaddr = MPU9250_AG_EXT_SENS_DATA_00;

			cnt = Read(&regaddr, 1, pBuff, cnt);
			if (cnt <=0)
				break;

			pBuff += cnt;
			BuffLen -= cnt;
			retval += cnt;
		}
	}
	else
	{
		retval = vpIntrf->Read(DevAddr, pCmdAddr, CmdAddrLen, pBuff, BuffLen);
	}

	return retval;
}

int AgmMpu9250::Write(uint8_t DevAddr, uint8_t *pCmdAddr, int CmdAddrLen, uint8_t *pData, int DataLen)
{
	int retval = 0;

	if (vpIntrf->Type() == DEVINTRF_TYPE_SPI)
	{
		uint8_t regaddr;
		uint8_t d[8];

		d[0] = MPU9250_AG_I2C_SLV0_ADDR;
		d[1] = DevAddr;
		d[2] = *pCmdAddr;
		d[3] = MPU9250_AG_I2C_SLV0_CTRL_I2C_SLV0_EN | 1;

		while (DataLen > 0)
		{
			regaddr = MPU9250_AG_I2C_SLV0_DO;
			Write8(&regaddr, 1, *pData);

			Write(d, 4, NULL, 0);

			d[2]++;
			pData++;
			DataLen--;
			retval++;
		}
	}
	else
	{
		retval = vpIntrf->Write(DevAddr, pCmdAddr, CmdAddrLen, pData, DataLen);
	}

	return retval;
}

void AgmMpu9250::IntHandler()
{
	uint8_t regaddr = MPU9250_AG_INT_STATUS;
	uint8_t d;

	d = Read8(&regaddr, 1);
	if (d & MPU9250_AG_INT_STATUS_RAW_DATA_RDY_INT)
	{
		UpdateData();
	}
}

void AgmMpu9250::ResetFifo()
{
	uint8_t regaddr;
	uint8_t d = 0;

	regaddr = MPU9250_AG_INT_ENABLE;
	Write8(&regaddr, 1, 0);

	regaddr = MPU9250_AG_FIFO_EN;
	Write8(&regaddr, 1, 0);

	regaddr = MPU9250_AG_USER_CTRL;
	Write8(&regaddr, 1, 0);

	Write8(&regaddr, 1, MPU9250_AG_USER_CTRL_FIFO_RST);

	d = MPU9250_AG_USER_CTRL_FIFO_EN;

	if (InterfaceType() == DEVINTRF_TYPE_SPI)
	{
		d |= MPU9250_AG_USER_CTRL_I2C_MST_EN;
	}
	Write8(&regaddr, 1, d);

	msDelay(50);

	regaddr = MPU9250_AG_INT_ENABLE;
	Write8(&regaddr, 1, MPU9250_AG_INT_ENABLE_RAW_RDY_EN);

	regaddr = MPU9250_AG_FIFO_EN;
	Write8(&regaddr, 1, 0xFF);
}

bool AgmMpu9250::UploadDMPImage()
{
	int len = DMP_CODE_SIZE;
	uint8_t *p = (uint8_t*)s_DMPImage;
	uint8_t regaddr;
	uint16_t memaddr = 0;
	uint8_t d[2];

	while (len > 0)
	{
		int l = min(len, MPU9250_DMP_MEM_PAGE_SIZE);

		regaddr = MPU9250_DMP_MEM_BANKSEL;
		d[0] = memaddr >> 8;
		d[1] = memaddr & 0xFF;

		Write(&regaddr, 1, d, 2);

		regaddr = MPU9250_DMP_MEM_RW;
		Write(&regaddr, 1, p, l);

		p += l;
		memaddr += l;
		len -= l;
	}

	len = DMP_CODE_SIZE;
	p = (uint8_t*)s_DMPImage;
	memaddr = 0;

	// Verify
	while (len > 0)
	{
		uint8_t m[MPU9250_DMP_MEM_PAGE_SIZE];
		int l = min(len, MPU9250_DMP_MEM_PAGE_SIZE);

		regaddr = MPU9250_DMP_MEM_BANKSEL;
		d[0] = memaddr >> 8;
		d[1] = memaddr & 0xFF;

		Write(&regaddr, 1, d, 2);

		regaddr = MPU9250_DMP_MEM_RW;
		Read(&regaddr, 1, m, l);

		if (memcmp(p, m, l) != 0)
		{
			return false;
		}

		p += l;
		memaddr += l;
		len -= l;
	}

	// Write DMP program start address
	d[0] = DMP_START_ADDR >> 8;
	d[1] = DMP_START_ADDR & 0xFF;
	regaddr = MPU9250_DMP_PROG_START;
	Write(&regaddr, 1, d, 2);

	return true;
}


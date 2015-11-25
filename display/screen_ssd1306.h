/* 
 * Copyright (c) 2014, Daan Pape
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *     1. Redistributions of source code must retain the above copyright 
 *        notice, this list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright 
 *        notice, this list of conditions and the following disclaimer in the 
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * File:   screen_ssd1306.h
 * Created on September 9, 2014, 11:11 AM
 */

#ifndef SCREEN_SSD1306_H_
#define SCREEN_SSD1306_H_

#include <stdint.h>

/* Driver commands */
#define SCREEN_SSD1306_ON			0xAF
#define SCREEN_SSD1306_OFF			0xAE
#define SCREEN_SSD1306_NORMALDISP               0xA6
#define SCREEN_SSD1306_INVERTDISP		0xA7
#define SCREEN_SSD1306_	ALLON			0xA5
#define SCREEN_SSD1306_ALLON_RESUME		0xA4

#define SCREEN_SSD1306_SET_CONTRAST		0x81
#define SCREEN_SSD1306_SET_OFFSET		0xD3
#define SCREEN_SSD1306_SET_COMPINS		0xDA
#define SCREEN_SSD1306_SET_VCOMDETECT		0xDB
#define SCREEN_SSD1306_SET_CLOCKDIV		0xD5
#define SCREEN_SSD1306_SET_PRECHARGE		0xD9
#define SCREEN_SSD1306_SET_MULTIPLEX		0xA8
#define SCREEN_SSD1306_SET_LOWCOLUMN		0x00
#define SCREEN_SSD1306_SET_HIGHCOLUMN		0x10
#define SCREEN_SSD1306_SET_STARTLINE		0x40
#define SCREEN_SSD1306_SET_MEMMODE		0x20
#define SCREEN_SSD1306_SET_COMSCANINC		0xC0
#define SCREEN_SSD1306_SET_COMSCANDEC		0xC8
#define SCREEN_SSD1306_SET_SEGREMAP		0xA0
#define SCREEN_SSD1306_SET_CHARGEPUMP		0x8D
#define SCREEN_SSD1306_SET_EXT_VCC		0x1
#define SCREEN_SSD1306_SET_SWITCHAPVCC		0x2


/*
 * Send SPI command to the display
 * @command the command to send
 */
void screen_ssd1306_send_command(uint8_t command);

/*
 * Send SPI data to the display
 * @data the byte to send
 */
void screen_ssd1306_send_data(uint8_t data);

/*
 * Initialize SSD1306 screen OLED device
 */
void screen_ssd1306_init(void);

void screen_ssd1306_set_startpage(uint8_t page);
void screen_ssd1306_set_startcolumn(uint8_t column);
void screen_ssd1306_clear(void);

#endif /* SCREEN_SSD1306_H_ */

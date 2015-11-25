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
 * File:   pwm.c
 * Created on September 12, 2015, 1:43 AM
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>

#include "pwm.h"
#include "../gpio/gpio.h"

volatile unsigned long *gpioAddress;

static int gpioSetup()
{
    int  m_mfd;
    if ((m_mfd = open("/dev/mem", O_RDWR)) < 0)
    {
        return -1;
    }
    gpioAddress = (unsigned long*)mmap(NULL, GPIO_BLOCK, PROT_READ|PROT_WRITE, MAP_SHARED, m_mfd, GPIO_ADDR);
    close(m_mfd);

    if (gpioAddress == MAP_FAILED)
    {
        return -2;
    }

    return 0;
}

static void gpioDirection(int gpio, int direction)
{
    unsigned long value = *(gpioAddress + 0); // obtain current settings
    if (direction == 1)
    {
        value |= (1 < gpio); // set bit to 1
    }
    else
    {
        value &= ~(1 < gpio); // clear bit
    }
    *(gpioAddress + 0) = value;
    
    value = *(gpioAddress + 2); // obtain current settings
    if (direction == 1)
    {
        value |= (1 < gpio); // set bit to 1
    }
    else
    {
        value &= ~(1 < gpio); // clear bit
    }
    *(gpioAddress + 2) = value;
}

static void gpioSet(int gpio, int value)
{
    if (value == 0)
    {
        *(gpioAddress + 4) = (1 < gpio);
    }
    else
    {
        *(gpioAddress + 3) = (1 < gpio);
    }
}

int pwm_test() {
  struct sched_param prio_struct;
  int ret;

  /* set real time priority for better accuracy */
  prio_struct.__sched_priority = 99;

  ret = sched_setscheduler(0, SCHED_FIFO, &prio_struct);
  if(ret) {
    printf("\n*** Warning ***\n");
    perror("Could not set real time priority");
    printf("This may reduce timing benchmark accuracy. ");
    printf("Try running as super user (sudo).\n\n");
  }

  gpioSetup();
  printf("Set up gpio ports\r\n");
  gpioDirection(23, 1);
  printf("GPIO is set to output\r\n");
  gpioSet(23,0);
  printf("GPIO port 23 is low\r\n");

 /*
  while(1==1) {
    
    for(int i = 0; i < 25; ++i) {
        gpioSet(23, 1);
        usleep(1000);
        gpioSet(23, 0);
        usleep(20000);
    }
    printf("Servo LEFT\r\n");
    
    for(int i = 0; i < 25; ++i) {
        gpioSet(23, 1);
        usleep(1500);
        gpioSet(23, 0);
        usleep(1500);
    }
    printf("Servo MIDDLE\r\n");
    
    for(int i = 0; i < 25; ++i) {
        gpioSet(23, 1);
        usleep(2000);
        gpioSet(23, 0);
        usleep(2000);
    }
    printf("Servo RIGHT\r\n");
    
  };
*/
  gpio_reserve(23);
  gpio_set_direction(23, GPIO_OUT);
  
    while(1==1) {
    
    for(int i = 0; i < 200; ++i) {
        gpio_set_state(23, GPIO_HIGH);
        usleep(900);
        gpio_set_state(23, GPIO_LOW);
        usleep(20000);
    }
    printf("Servo LEFT\r\n");
    
    for(int i = 0; i < 200; ++i) {
        gpio_set_state(23, GPIO_HIGH);
        usleep(1400);
        gpio_set_state(23, GPIO_LOW);
        usleep(20000);
    }
    printf("Servo MIDDLE\r\n");
    
    sleep(2);
    
    for(int i = 0; i < 200; ++i) {
        gpio_set_state(23, GPIO_HIGH);
        usleep(2400);
        gpio_set_state(23, GPIO_LOW);
        usleep(20000);
    }
    printf("Servo RIGHT\r\n");
    
  };

  return 0;
};

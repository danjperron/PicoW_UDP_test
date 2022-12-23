/**
 * Copyright (c) 2022 Daniel Perron
 *
 * M.I.T.license
 */

#include <string.h>
#include <time.h>


#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "nokia5110_LCD.h"

#define  SEND_TO_IP  "10.11.12.114"
#define  SEND_TO_PORT 6001

#define  RCV_FROM_IP IP_ADDR_ANY
#define  RCV_FROM_PORT 6002

#define UDP_SEND_FLAG
#undef UDP_SEND_FLAG

#define BUF_SIZE 64

char string_buffer[BUF_SIZE];

alarm_pool_t * alarmP;
repeating_timer_t  RptTimer;

struct udp_pcb  * send_udp_pcb;
struct udp_pcb  * rcv_udp_pcb;

bool TimerFlag=false;

static bool Send_udp_stuff(repeating_timer_t *timer)
{
      TimerFlag=true;
      return true;
}


#ifdef UDP_SEND_FLAG

void SendUDP(char * IP , int port, void * data, int data_size)
{
      ip_addr_t   destAddr;
      ip4addr_aton(IP,&destAddr);
      struct pbuf * p = pbuf_alloc(PBUF_TRANSPORT,data_size+1,PBUF_RAM);
      char *pt = (char *) p->payload;
      memcpy(pt,data,data_size);
      pt[data_size]='\0';
      cyw43_arch_lwip_begin();
      udp_sendto(send_udp_pcb,p,&destAddr,port);
      cyw43_arch_lwip_end();
      pbuf_free(p);
}

#endif

void RcvFromUDP(void * arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t*addr,u16_t port)
{
     char *buffer=calloc(p->len+1,sizeof(char));  // use calloc to >
     LCD_clrScr();
     sprintf(string_buffer,"%d.%d.%d.%d",
		addr->addr&0xff,
		(addr->addr>>8)&0xff,
		(addr->addr>>16)&0xff,
		addr->addr>>24);
     LCD_print(string_buffer,0,0);
     sprintf(string_buffer,"Port=%d",port);
     LCD_print(string_buffer,0,1);
     sprintf(string_buffer,"Length = %d",p->len);
     LCD_print(string_buffer,0,2);
     strncpy(buffer,(char *)p->payload,p->len);
     LCD_print(buffer,0,3);
     free(buffer);
     pbuf_free(p);
}



int main() {
    int loop;
    char buffer[BUF_SIZE];
     extern struct netif gnetif;
    stdio_init_all();
    LCD_init();
    for(loop=0;loop<30;loop++)
       LCD_setPixel(loop,loop,loop %2);
    if (cyw43_arch_init()) {
        LCD_print("Init failed!",0,0);
        return 1;
    }
    cyw43_pm_value(CYW43_NO_POWERSAVE_MODE,200,1,1,10);

    cyw43_arch_enable_sta_mode();

    LCD_print("WiFi ...",0,0);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        LCD_print("failed! ",0,1);
        return 1;
    } else {
        LCD_print("Connected.",0,1);
        strcpy(string_buffer,ipaddr_ntoa(((const ip_addr_t *)&cyw43_state.netif[0].ip_addr)));
        LCD_print(string_buffer,0,2);

  }

#ifdef UDP_SEND_FLAG
   alarm_pool_init_default();
   alarmP = alarm_pool_get_default();
   alarm_pool_add_repeating_timer_ms(alarmP,20000,Send_udp_stuff,NULL,&RptTimer);
   send_udp_pcb = udp_new();
#endif

   rcv_udp_pcb = udp_new();

   err_t err = udp_bind(rcv_udp_pcb,RCV_FROM_IP,RCV_FROM_PORT);
   udp_recv(rcv_udp_pcb, RcvFromUDP,NULL);

    while(1)
    {
#ifdef UDP_SEND_FLAG 
      if(TimerFlag)
          {
            memset(buffer,0,BUF_SIZE);
            sprintf(buffer,"%u\n",loop++);
            SendUDP(SEND_TO_IP,SEND_TO_PORT,buffer,BUF_SIZE);
            TimerFlag=false;
          }
#endif
     sleep_ms(10);
    cyw43_arch_poll();
    }

#ifdef UDP_SEND_FLAG
    udp_remove(send_udp_pcb);
#endif
    udp_remove(rcv_udp_pcb);
    cyw43_arch_deinit();
    return 0;
}

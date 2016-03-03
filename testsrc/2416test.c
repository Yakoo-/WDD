#include "s3c2416.h"
#include "types.h"

#define LED	(0) // gpc0 for led

#define BUZZER	(0) // gpf0 for buzzer

void Delay_ms(uint ms);
void GPIO_Init(void);
void LED_Ctrl(uchar on);
void Beep(uint ms);

int main(void)
{
    GPIO_Init();
    LED_Ctrl(1);
    Beep(300);
    while (1){
    
    }

    return 1;
}

void GPIO_Init(void)
{
    rGPCCON &= ~(0x03 << (LED << 1));
    rGPCCON |= (0x01 << (LED << 1));
    rGPFCON &= ~(0x03 << (BUZZER << 1));
    rGPFCON |= (0x01 << (BUZZER <<1));
}

void LED_Ctrl(uchar on)
{
    if( on ){
	rGPCDAT &= ~(1 << LED);
    } else {
	rGPCDAT |= (1 << LED);
    }
}

void Beep(uint ms)
{
    rGPFDAT |= (1 << BUZZER);
    Delay_ms(ms);
    rGPCDAT &= ~(1 << BUZZER);
    Delay_ms(20);
}


void Delay_ms(uint ms)
{
    uint i = 0; 

    i = 200000;
    while (0 < ms--){
	while(0 < i--);
    }
}


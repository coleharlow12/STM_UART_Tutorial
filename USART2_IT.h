/*
The purpose of this file is to manage the interrupts associated with the USART2 port. 
 */

#ifndef USART2_IT_H
#define USART2_IT_H

// Defines the size of the Queue I just arbitrarily chose 256 it could be many other values
#define QUEUE_SIZE 512

//Includes
#include <stdint.h>
#include "Mavlink.h"
#include "SFI1.h"

//Defines the Queue Structure used to store and receive data from the TX and RX register. 
typedef struct Receive_Transmit_Queue
{
    //Variables "pointer" Read and "Pointer" Write. Note that initially when the queue is empty these. 
    //variables are equal. At least I believe that is the case. 
    //Note that these are not real pointers they are used for indexing. 
    uint16_t pRD,pWR;
    uint16_t sizeRecieve;
    uint8_t q[QUEUE_SIZE];
    
} RTQueue;

//Interrupt Handler Function
void USART2_IRQHandler(void);

//Puts the char C onto the Transmit Queue
int putcharC(int C);
//Gets a character from the Receive Queue
int getcharC(void);

#endif
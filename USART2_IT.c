/*
The purpose of this code is to allow for the sending and reception of data using the USART2 port on the STM32. 
To do this we need the following things
1) Interrupts: We need to use and enable the Transmit (register) Empty interrupt TXE and the Recieved (register) not empty interrupt 
    RXNE. This allows us to direct the uC to actually send and recieve data. 
2) We also need a circular transmit and recieve buffer. This buffer holds recieved data until it is read and also holds transmit data
    until it is transmitted.
3) There are also additional structures and functions used to determine when the buffer is empty or full. The size of the buffer is 
    declared in USART2_IT.h
4) If we intialize pRD and pWR to specific values we can know the amount of data in each of the two registers
 */

#include "stm32f334x8.h"
#include "USART2_IT.h"
#include "main.h"

//We just need one circular buffer. 
RTQueue RX_Buffer;
RTQueue TX_Buffer;

//Declaration of Helper Functions
static int QueueFull(RTQueue *q);
static int QueueEmpty(RTQueue *q);
static int enqueue(RTQueue *q, uint8_t data);
static int dequeue(RTQueue *q, uint8_t *data);

//Additional nonstatic functoins
int getchar(void);
int putchar(int c);

static int TxPrimed = 0;
int RxOverflow = 0;
void USART2_IRQHandler(void)
{
  // Handles the case that the Receive Register is not empty.
  // Be sure to note that the receive register is seperate from the circular buffer.
  if(LL_USART_IsActiveFlag_RXNE(USART2))
  {
    //This line recieves the data from the TX register. 
    uint8_t data;
    data = LL_USART_ReceiveData8(USART2) & 0xFF;
    //If enqueue returns one 
    if (!enqueue(&RX_Buffer, data))
    {
        RxOverflow = 1;
    }

    if(RX_Buffer.sizeRecieve >= 11)
    {
        //Triggers the software interrupt. This interrupt will automatically turn itself off. 
        NVIC->STIR = EXTI1_IRQn;
    }
  }

  //Handles the case that the TX register is Empty
  if(LL_USART_IsActiveFlag_TXE(USART2) && LL_USART_IsEnabledIT_TXE(USART2))
  {
    //This is the data that needs to be transmitted it is dequed from the
    uint8_t data;

    /* write one byte to the transmit data register if there is data to be written */
    if (dequeue(&TX_Buffer, &data))
    {
        //Once the byte to be sent is removed from the queue. 
        LL_USART_TransmitData8(USART2,data);
    }
    else
    {
        // if we have nothing to send, disable the interrupt
        // and wait for a kick
        LL_USART_DisableIT_TXE(USART2);
        TxPrimed = 0;
    }   
  }   
  return;  
}

/*________________________________________________________START HELPER FUNCTIONS________________________________________________________ */

//QueueFull: This function returns 1 if the QUEUE is full it has 1 parameter
//RTQueue: This parameter is a pointer to the Queue that we are checking to see if is full
/**
 * @breif   This function checks if the queue is full and returns a one if it is full, otherwise a 0 is returned
 * @notes
 * @param[in]   RTque is the queue which we are checking for fullness
 * @param[out]  There are no outputs
 * @retval  This function returns 1 if the queue is full and 0 otherwise
 */
static int QueueFull(RTQueue *RTque)
{
    //Remember the BUFFER is circular. Essentially what this does is checks that if the write pointer has wrapped around to
    //Be equal to the RD pointer. If this is the case then we have a full buffer. The % sign is modulus division it returns
    //The remainder 
    return(((RTque->pWR + 1) % QUEUE_SIZE) == RTque->pRD);
}

/**
 * @breif   This function checks if the que is empty
 * @notes   
 * @param[in]   This function checks if the queue RTque is full
 * @param[out]  There are no outputs
 * @retval  This function returns one if the queue is empty and 0 otherwise 
 */
static int QueueEmpty(RTQueue *RTque)
{
    //Accesses the pointer to the Read data. Which is stored in the structure RTque
    return(RTque->pRD == RTque->pWR);
}

/**
 * @brief   The purpose of this function is to add a byte of data to the buffer from the USART receive buffer
 * @notes   This function writes data to the write section of the buffer
 * @param[in]   RTque This is the que that the data to send is stored in
 *              Data This is the byte of data that needs written onto the buffer
 * @param[out]  This function has no outputs other than the 
 * @retval  This function returns one if the data was added successfully to the que and 0 if not
 */
static int enqueue(RTQueue *RTque, uint8_t data)
{
    //Checks that the Queue is not full
    if(QueueFull(RTque))
        //If the Queue is full we return 0 
        {return 0;}
    else
    {
        //This first part of this statement accesses the que within the RTque structure and uses it to store Data at the
        //Next available addreess
        RTque->q[RTque->pWR] = data;
        //This resets the RTque pWR by first checking if pWR+1 is equal to the Queue size
        // If this is true then pWR is set to be zero. This is because a que with length L
        // is indexed from 0 to L-1. 
        // In the cast that pWR + 1 is not qual to QUEUE_SIZE then increment pWR
        RTque->pWR = ((RTque->pWR + 1) == QUEUE_SIZE) ? 0 : RTque->pWR+1;

        RTque->sizeRecieve = ((RTque->sizeRecieve) + 1);
    }
    return 1;
}

/**
 * @brief   Pulls a byte of data from the buffer and stores it in the variable pointed to by data
 * @note    This function pulls data from a circular buffer (RTque structure) this data is stored in 
 *          the buffer by the function enqueue
 * @param[in] The input variable is the circular buffer which is defined as a RTque structure in 
 *          this structure is defined in USART2_IT.h
 *          The input variable data is a pointer to the variable where we would like the read byte to be
 *          stored
 * @param[out] There are no output variables.
 * @retval 
 */
static int dequeue(RTQueue *RTque, uint8_t *data)
{
    //Checks to make sure the RTque is not empty. 
    if (QueueEmpty(RTque))
        //If the Que is empty return zero
        {return 0;}
    else
    {
        //This recieves the data and stores it at the address specified by *data. 
        *data = RTque->q[RTque->pRD];
        //Method of adjusting the pRD index. 
        RTque->pRD = ((RTque->pRD+1) == QUEUE_SIZE) ? 0 : RTque->pRD + 1;
        //Decrease the size of the Que that is currently being received
        RTque->sizeRecieve = ((RTque->sizeRecieve) -1);
    }
    return 1;
}
/*______________________________________________________END HELPER FUNCTIONS_________________________________________________________ */


/**
 * @brief   This function is used to get one byte from the circular buffer declared in this file.
 * @note    RX_Buffer is a buffer declared in the file USART2_IT.c
 * @param[in]   There are no input parameters in this function
 * @param[out]  There are no outputs from this function other than the return value
 * @retval This function returns the value of the byte read from the buffer
 */
int getcharC()
{
    uint8_t data;
    while (dequeue(&RX_Buffer, &data))
        {return data;}  
}

/**
 *  @brief  This function is used to put one byte of data onto the TX_Buffer
 *  @note   This function works by using the function enqueue to check if there is 
 *          data to be read from the buffer
 * @param[in]   This function has one input c which it adds to the TX_Buffer
 * @param[out]  There are no outputs other than the return value
 * @retval  There is no return value

int putcharC(int c)
{
    while (!enqueue(&TX_Buffer, c))
    {
        if(!TxPrimed)
        {
            TxPrimed = 1;
            LL_USART_EnableIT_TXE(USART2);
        }
    }
}

//****************************************************************************
//
//! \addtogroup main
//! @{
//
//****************************************************************************

#include <stdio.h>
#include <string.h>

// Driverlib Includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "systick.h"
#include "pin.h"
#include "prcm.h"
#include "utils.h"
#include "rom_map.h"
#include "interrupt.h"

// SimpleLink include
#include "simplelink.h"

// Free-RTOS\TI-RTOS include
#include "osi.h"

// Common interface includes
#include "network_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "udma_if.h"
#include "common.h"

#include "pinmux.h"
#include "httpserverapp.h"


//*****************************************************************************
//                      GLOBAL VARIABLES for VECTOR TABLE
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//                          LOCAL DEFINES
//*****************************************************************************
#define APP_NAME		        "WebSocket"
#define SPAWN_TASK_PRIORITY     9
#define HTTP_SERVER_APP_TASK_PRIORITY  1
#define OSI_STACK_SIZE          2048

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
  /* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
  IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
  IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
  //
  // Enable Processor
  //
  MAP_IntMasterEnable();
  MAP_IntEnable(FAULT_SYSTICK);
  PRCMCC3200MCUInit();
}

//****************************************************************************
//							MAIN FUNCTION
//****************************************************************************
void main() {
  //
  // Board Initialization
  //
  BoardInit();
  
  //
  // Enable and configure DMA
  //
  UDMAInit();
  //
  // Pinmux for UART
  //
  PinMuxConfig();
  
  //
  // Start the SimpleLink Host
  //
  VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
  //
  // Start the HttpServer Task
  //
  //
  
  osi_TaskCreate(HttpServerAppTask,
                 "WebSocketApp",
                 OSI_STACK_SIZE,
                 NULL,
                 HTTP_SERVER_APP_TASK_PRIORITY,
                 NULL );
  
//  UART_PRINT("HttpServerApp Initialized \n\r");
  
  
  //Start the task scheduler
    
  osi_start();
  //  InitializeAppVariables();
  
  return;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

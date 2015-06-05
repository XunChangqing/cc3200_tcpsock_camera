//*****************************************************************************
// httpserver_app.c
//
// camera application macro & APIs
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************
//*****************************************************************************
//
//! \addtogroup Httpserverapp
//! @{
//
//*****************************************************************************

#include <string.h>
#include <stdlib.h>

// Driverlib Includes
#include "rom_map.h"
#include "hw_types.h"
#include "prcm.h"
#include "utils.h"

// SimpleLink include
#include "simplelink.h"

// Free-RTOS/TI-RTOS include
#include "osi.h"

// HTTP lib includes
#include "HttpCore.h"
#include "HttpRequest.h"

// Common-interface includes
#include "network_if.h"
#include "uart_if.h"
#include "common.h"

#include "websocketapp.h"
#include "httpserverapp.h"
#include "camera_app.h"
#include "i2cconfig.h"
#include "mt9d111.h"


/****************************************************************************/
/*				MACROS										*/
/****************************************************************************/
#define CAMERA_SERVICE_PRIORITY     1

/****************************************************************************
                              Global variables
****************************************************************************/
char *g_Buffer;
UINT8 g_success = 0;
//int g_close = 0;
UINT16 g_uConnection;
OsiTaskHandle g_iCameraTaskHdl = 0;

//void WebSocketCloseSessionHandler(void)
//{
//	g_close = 1;
//}

//void CameraAppTask(void *param)
//{
//	UINT8 Opcode = 0x02;
//	struct HttpBlob Write;
//
//	InitCameraComponents(640, 480);
//
//	while(1)
//	{
//		if(g_close == 0)
//		{
//			Write.uLength = StartCamera((char **)&Write.pData);
//
//			if(!sl_WebSocketSend(g_uConnection, Write, Opcode))
//			{
//				while(1);
//			}
//		}
//	}
//
//}


/*!
 * 	\brief 					This websocket Event is called when WebSocket Server receives data
 * 							from client.
 *
 *
 * 	\param[in]  uConnection	Websocket Client Id
 * 	\param[in] *ReadBuffer		Pointer to the buffer that holds the payload.
 *
 * 	\return					none.
 *     					
 */
//void WebSocketRecvEventHandler(UINT16 uConnection, char *ReadBuffer)
//{
//	char *camera = "capture";
//
//	/*
//	 * UINT8 Opcode;
//	 * struct HttpBlob Write;
//	*/
//
//	g_uConnection = uConnection;
//
//	g_Buffer = ReadBuffer;
//	g_close = 0;
//	if (!strcmp(ReadBuffer,camera))
//	{
//		if(!g_iCameraTaskHdl)
//		{
//			osi_TaskCreate(CameraAppTask,
//								   "CameraApp",
//									1024,
//									NULL,
//									CAMERA_SERVICE_PRIORITY,
//									&g_iCameraTaskHdl);
//		}
//
//	}
//	//Free memory as we are not using anywhere later
//	free(g_Buffer);
//	g_Buffer = NULL;
//	/* Enter websocket application code here */
//	return;
//}


/*!
 * 	\brief 						This websocket Event indicates successful handshake with client
 * 								Once this is called the server can start sending data packets over websocket using
 * 								the sl_WebSocketSend API.
 *
 *
 * 	\param[in] uConnection			Websocket Client Id
 *
 * 	\return						none
 */
//void WebSocketHandshakeEventHandler(UINT16 uConnection)
//{
//	g_success = 1;
//	g_uConnection = uConnection;
//}


//****************************************************************************
//
//! Task function start the device and crete a TCP server showcasing the smart
//! plug
//!
//****************************************************************************
#define IP_ADDR             0xc0a80164 /* 192.168.1.100 */
#define PORT_NUM            31500
#define BUF_SIZE            1400
#define TCP_PACKET_COUNT    1000

// Application specific status/error codes
typedef enum{
  // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
  SOCKET_CREATE_ERROR = -0x7D0,
  BIND_ERROR = SOCKET_CREATE_ERROR - 1,
  LISTEN_ERROR = BIND_ERROR -1,
  SOCKET_OPT_ERROR = LISTEN_ERROR -1,
  CONNECT_ERROR = SOCKET_OPT_ERROR -1,
  ACCEPT_ERROR = CONNECT_ERROR - 1,
  SEND_ERROR = ACCEPT_ERROR -1,
  RECV_ERROR = SEND_ERROR -1,
  SOCKET_CLOSE_ERROR = RECV_ERROR -1,
  DEVICE_NOT_IN_STATION_MODE = SOCKET_CLOSE_ERROR - 1,
  STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
int BsdTcpClient(unsigned short usPort);

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
unsigned long  g_ulDestinationIp = IP_ADDR;
unsigned int   g_uiPortNum = PORT_NUM;
volatile unsigned long  g_ulPacketCount = TCP_PACKET_COUNT;
unsigned char  g_ucConnectionStatus = 0;
unsigned char  g_ucSimplelinkstarted = 0;
unsigned long  g_ulIpAddr = 0;
char g_cBsdBuf[BUF_SIZE];

//****************************************************************************
//
//! \brief Opening a TCP client side socket and sending data
//!
//! This function opens a TCP socket and tries to connect to a Server IP_ADDR
//!    waiting on port PORT_NUM.
//!    If the socket connection is successful then the function will send 1000
//! TCP packets to the server.
//!
//! \param[in]      port number on which the server will be listening on
//!
//! \return    0 on success, -1 on Error.
//
//****************************************************************************
int BsdTcpClient(unsigned short usPort)
{
  int             iCounter;
//  short           sTestBufLen;
  SlSockAddrIn_t  sAddr;
  int             iAddrSize;
  int             iSockID;
  int             iStatus;
//  long            lLoopCount = 0;
  
  // filling the buffer
  for (iCounter=0 ; iCounter<BUF_SIZE ; iCounter++)
  {
    g_cBsdBuf[iCounter] = (char)(iCounter % 10);
  }
  
//  sTestBufLen  = BUF_SIZE;
  
  //filling the TCP server socket address
  sAddr.sin_family = SL_AF_INET;
  sAddr.sin_port = sl_Htons((unsigned short)usPort);
  sAddr.sin_addr.s_addr = sl_Htonl((unsigned int)g_ulDestinationIp);
  
  iAddrSize = sizeof(SlSockAddrIn_t);
  
  // creating a TCP socket
  iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
  if( iSockID < 0 )
  {
    ASSERT_ON_ERROR(SOCKET_CREATE_ERROR);
  }
  
  // connecting to TCP server
  iStatus = sl_Connect(iSockID, ( SlSockAddr_t *)&sAddr, iAddrSize);
  if( iStatus < 0 )
  {
    // error
    sl_Close(iSockID);       
    ASSERT_ON_ERROR(CONNECT_ERROR);
  }
  
//  UINT8 Opcode = 0x02;
//  struct HttpBlob Write;
  UINT16 uLength;
  UINT8* pData;
 
  InitCameraComponents(640, 480);
  
  while(1)
  {
//      uLength = 8192;
//      iStatus = sl_Send(iSockID, &uLength, 2, 0);

      uLength = StartCamera((char **)&pData);
      iStatus = sl_Send(iSockID, &uLength, 2, 0);
      iStatus = sl_Send(iSockID, pData, uLength, 0);
      if( iStatus < 0 )
      {
        // error
        sl_Close(iSockID);
        ASSERT_ON_ERROR(SEND_ERROR);
      }      
  }

  // sending multiple packets to the TCP server
//  while (lLoopCount < g_ulPacketCount)
//  {
//    // sending packet
//    iStatus = sl_Send(iSockID, g_cBsdBuf, sTestBufLen, 0 );
//    if( iStatus < 0 )
//    {
//      // error
//      sl_Close(iSockID);
//      ASSERT_ON_ERROR(SEND_ERROR);
//    }
//    lLoopCount++;
//  }
  
  //  Report("Sent %u packets successfully\n\r",g_ulPacketCount);
  
  iStatus = sl_Close(iSockID);
  //closing the socket after sending 1000 packets
  ASSERT_ON_ERROR(iStatus);
  
  return SUCCESS;
}

//****************************************************************************
//
//! \brief Opening a TCP server side socket and receiving data
//!
//! This function opens a TCP socket in Listen mode and waits for an incoming
//!    TCP connection.
//! If a socket connection is established then the function will try to read
//!    1000 TCP packets from the connected client.
//!
//! \param[in] port number on which the server will be listening on
//!
//! \return     0 on success, -1 on error.
//!
//! \note   This function will wait for an incoming connection till
//!                     one is established
//
//****************************************************************************
int BsdTcpServer(unsigned short usPort)
{
    SlSockAddrIn_t  sAddr;
    SlSockAddrIn_t  sLocalAddr;
    int             iCounter;
    int             iAddrSize;
    int             iSockID;
    int             iStatus;
    int             iNewSockID;
    long            lLoopCount = 0;
    long            lNonBlocking = 1;
    int             iTestBufLen;

//    // filling the buffer
//    for (iCounter=0 ; iCounter<BUF_SIZE ; iCounter++)
//    {
//        g_cBsdBuf[iCounter] = (char)(iCounter % 10);
//    }

//    iTestBufLen  = BUF_SIZE;

    //filling the TCP server socket address
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = sl_Htons((unsigned short)usPort);
    sLocalAddr.sin_addr.s_addr = 0;

    // creating a TCP socket
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    if( iSockID < 0 )
    {
        // error
        ASSERT_ON_ERROR(SOCKET_CREATE_ERROR);
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    // binding the TCP socket to the TCP server address
    iStatus = sl_Bind(iSockID, (SlSockAddr_t *)&sLocalAddr, iAddrSize);
    if( iStatus < 0 )
    {
        // error
        sl_Close(iSockID);
        ASSERT_ON_ERROR(BIND_ERROR);
    }

    // putting the socket for listening to the incoming TCP connection
    iStatus = sl_Listen(iSockID, 0);
    if( iStatus < 0 )
    {
        sl_Close(iSockID);
        ASSERT_ON_ERROR(LISTEN_ERROR);
    }
    
    InitCameraComponents(640, 480);

    // setting socket option to make the socket as non blocking
//    iStatus = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_NONBLOCKING, 
//                            &lNonBlocking, sizeof(lNonBlocking));
//    if( iStatus < 0 )
//    {
//        sl_Close(iSockID);
//        ASSERT_ON_ERROR(SOCKET_OPT_ERROR);
//    }
    iNewSockID = SL_EAGAIN;

    // waiting for an incoming TCP connection
    while( iNewSockID < 0 )
    {
        // accepts a connection form a TCP client, if there is any
        // otherwise returns SL_EAGAIN
        iNewSockID = sl_Accept(iSockID, ( struct SlSockAddr_t *)&sAddr, 
                                (SlSocklen_t*)&iAddrSize);
        if( iNewSockID == SL_EAGAIN )
        {
           MAP_UtilsDelay(10000);
        }
        else if( iNewSockID < 0 )
        {
            // error
            sl_Close(iNewSockID);
            sl_Close(iSockID);
            ASSERT_ON_ERROR(ACCEPT_ERROR);
        }
    }
    
    UINT16 uLength;
    UINT8* pData;
    
    while(1)
    {
      uLength = StartCamera((char **)&pData);
      iStatus = sl_Send(iNewSockID, &uLength, 2, 0);
      int len_send = 0;
      while(len_send<uLength){
        len_send += sl_Send(iNewSockID, pData, uLength, 0);
      }
//      if( iStatus < 0 )
//      {
//        // error
//        sl_Close(iSockID);
//        ASSERT_ON_ERROR(SEND_ERROR);
//      }      
    }

    // waits for 1000 packets from the connected TCP client
//    while (lLoopCount < g_ulPacketCount)
//    {
//        iStatus = sl_Recv(iNewSockID, g_cBsdBuf, iTestBufLen, 0);
//        if( iStatus <= 0 )
//        {
//          // error
//          sl_Close(iNewSockID);
//          sl_Close(iSockID);
//          ASSERT_ON_ERROR(RECV_ERROR);
//        }
//
//        lLoopCount++;
//    }

//    Report("Recieved %u packets successfully\n\r",g_ulPacketCount);
    
    // close the connected socket after receiving from connected TCP client
    iStatus = sl_Close(iNewSockID);    
    ASSERT_ON_ERROR(iStatus);
    // close the listening socket
    iStatus = sl_Close(iSockID);
    ASSERT_ON_ERROR(iStatus);   

    return SUCCESS;
}


void HttpServerAppTask(void * param)
{
  long lRetVal = -1;
  
  lRetVal = Network_IF_InitDriver(ROLE_STA);
  
  SlSecParams_t secParams = {0};
  secParams.Key = (signed char*)SECURITY_KEY;
  secParams.KeyLen = strlen(SECURITY_KEY);
  secParams.Type = SECURITY_TYPE;
  lRetVal = Network_IF_ConnectAP(SSID_NAME, secParams);
  
  if(lRetVal < 0)
  {    
    LOOP_FOREVER();
  }
  
//  BsdTcpClient(PORT_NUM);
  BsdTcpServer(PORT_NUM);
  //  
  //	//Start SimpleLink in AP Mode
  //	lRetVal = Network_IF_InitDriver(ROLE_AP);
  //    if(lRetVal < 0)
  //    {
  //        ERR_PRINT(lRetVal);
  //        LOOP_FOREVER();
  //    }	
  //
  //	//Stop Internal HTTP Server
  //	lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
  //    if(lRetVal < 0)
  //    {
  //        ERR_PRINT(lRetVal);
  //        LOOP_FOREVER();
  //    }	
  //
  //	//Run APPS HTTP Server
  //	HttpServerInitAndRun(NULL);
  //
  //	LOOP_FOREVER();

}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

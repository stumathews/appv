/*************************************************************************
*
* Copyright (C) Citrix Systems, Inc. All Rights Reserved.
*
* AppV Offline Streaming Virtual Driver
*
* REMARKS:
*	This driver will work with all clients.  It has been enhanced
*	to send data immediately when working with the HPC client (Version >= 13.0).
*	
*	When the HPC client is detected, this driver works without regular polling.
*	When ICA data arrives from the server, responses are sent immediately.
*	If the client is busy and cannot send the data immediately, the driver
*	is informed of that fact, and the DriverPoll function will be called
*	later when the driver should attempt to send again.
*
*************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef DOS32
#define CITRIX 1
#include <sys/types.h>
#include "wfc32.h"
#endif

#ifndef unix
#include <windows.h>         /* for far etc. */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "citrix.h"
#include "clib.h"

/* from src/shared/inc/citrix */
#include "ica.h"
#include "ica-c2h.h"
#include "appv.h"

/* from src/inc */
#include "icaconst.h"
#include "clterr.h"
#include "wdapi.h"
#include "vdapi.h"
#include "vd.h"
#include "ctxdebug.h"
#include "logapi.h"
#include "miapi.h"

#include "cwire.h"
#include "cmacro.h"
#include "asprintf.h"

#if defined(macintosh) || defined(unix)
#include "pingwire.h"
#endif

#ifdef DEBUG
#pragma optimize ("", off)	// turn off optimization
#endif

#define NOT_USED_BUT_REQUIRED  
#define DRIVER_API 
#define DRIVER_API_CALLBACK 
#define SIZEOF_CONSOLIDATION_BUFFER 2000		// size of the consolidation buffer to allocate (arbitrary 2000 for the sample).
#define NUMBER_OF_MEMORY_SECTIONS 1             // number of memory buffers to send as a single packet

/* Driver functions that need to be implemented - called by WinStation driver */
DRIVER_API INT DriverOpen(PVD pVD, PVDOPEN pVdOpen, PUINT16 puiSize);
DRIVER_API INT DriverClose(PVD pVD, PDLLCLOSE pVdClose, PUINT16 puiSize);
DRIVER_API INT DriverInfo(PVD pVD, PDLLINFO pVdInfo, PUINT16 puiSize);
DRIVER_API INT DriverPoll(PVD pVD, PVOID pVdPoll, PUINT16 puiSize);
DRIVER_API INT DriverGetLastError(PVD pVD, PVDLASTERROR pVdLastError);
NOT_USED_BUT_REQUIRED DRIVER_API INT DriverQueryInformation(PVD pVD, PVDQUERYINFORMATION pVdQueryInformation, PUINT16 puiSize);
NOT_USED_BUT_REQUIRED DRIVER_API INT DriverSetInformation(PVD pVD, PVDSETINFORMATION pVdSetInformation, PUINT16 puiSize);

#ifdef __cplusplus
}
#endif

#import "..\TestClassLibrary\bin\Release\TestLibrary.tlb" raw_interfaces_only

using namespace TestClassLibrary;

DRIVER_API_CALLBACK static void WFCAPI ICADataArrival(PVD pVD, USHORT uchan, LPBYTE pBuf, USHORT Length);

// Utility function that will send data on the channel
int PrepareAvailableDataForEnhancedSend(void);
void DebugWrite(const char* message, ...);

PVOID g_pWd = NULL;

// Pointer to the Send function that driver poll will use
PQUEUEVIRTUALWRITEPROC LegacySend  = NULL;

// Pointer to the HPC engine SendData function.
PSENDDATAPROC SendData = NULL; 

// We'll keep track of if we've got data to send or not
BOOL g_bBufferEmpty = TRUE;	

// Channel Packet Protocol 
PSendReceiveBuffer g_pAppV = NULL;

// Maximum Data Write Size
USHORT g_usMaxDataSize = 0;
USHORT g_usVirtualChannelNum = 0;
BOOL g_bIsHpc = FALSE;

// Buffer to consolidate a write that spans the end and beginning of the ring buffer.
LPBYTE g_pbaConsolidationBuffer = NULL;     

// Memory buffer pointer array
MEMORY_SECTION g_MemorySections[NUMBER_OF_MEMORY_SECTIONS];

// Sample user data for HPC
ULONG g_ulUserData = 0xCAACCAAC;      		

#include <iostream>

DRIVER_API int DriverOpen(IN PVD pVd, IN OUT PVDOPEN pVdOpen, OUT PUINT16 puiSize)
{
    WDSETINFORMATION   wdsi;
    VDWRITEHOOK        vdwh;

	// Struct for getting more engine information, used by HPC
	VDWRITEHOOKEX      vdwhex;
    WDQUERYINFORMATION wdqi;

	// This is filled in with the virtual channel information once we set it up via WDxQUERYINFORMATION
    OPENVIRTUALCHANNEL OpenVirtualChannel;
    int                rc;
    UINT16             uiSize;
	
	DebugWrite("AppV: DriverOpen entered");
    g_bBufferEmpty = TRUE;
		
	    
	//This is REQUIERD to be sent OUT of the function
	*puiSize = sizeof(VDOPEN);
	
	DebugWrite("AppV: Attempting to get open virtual channel '%s'", CTXAPPV_VIRTUAL_CHANNEL_NAME);
	    
	wdqi.WdInformationClass = WdOpenVirtualChannel;
    wdqi.pWdInformation = &OpenVirtualChannel;
    wdqi.WdInformationLength = sizeof(OPENVIRTUALCHANNEL);
    OpenVirtualChannel.pVCName = CTXAPPV_VIRTUAL_CHANNEL_NAME;
    
    OUT uiSize = sizeof(WDQUERYINFORMATION);
	DebugWrite("AppV: Opening channel %s...\n", OpenVirtualChannel.pVCName);
	rc = VdCallWd(pVd, WDxQUERYINFORMATION, &wdqi, &uiSize);	
	
    if(rc != CLIENT_STATUS_SUCCESS)
    {        
		DebugWrite("AppV: Could not open %s. rc=%d.", OpenVirtualChannel.pVCName, rc);
        return(rc);
    }			
    g_usVirtualChannelNum = OpenVirtualChannel.Channel;
	
	/* pointer to private data, if needed */
    pVd->pPrivate   = NULL; 

	DebugWrite("AppV: Channel Opened. Name = %s Channel Number = %lu\n", OpenVirtualChannel.pVCName, g_usVirtualChannelNum);

	DebugWrite("Registering driver's ICADataArrival callack function...");
	/* Get a callback to Send function (send data to server on the channel) */
    vdwh.Type  = g_usVirtualChannelNum;
    vdwh.pVdData = pVd; // Currently send NULL private data
    vdwh.pProc = (PVDWRITEPROCEDURE) ICADataArrival;
    wdsi.WdInformationClass  = WdVirtualWriteHook;
    wdsi.pWdInformation      = &vdwh;
    wdsi.WdInformationLength = sizeof(VDWRITEHOOK);
    uiSize                   = sizeof(WDSETINFORMATION);

    rc = VdCallWd(pVd, WDxSETINFORMATION, &wdsi, &uiSize);	
    if(CLIENT_STATUS_SUCCESS != rc)
    {     
		DebugWrite("AppV: Could not register ICADataArrival callack function. rc %d", rc);
        return(rc);
    }
	DebugWrite("AppV: ICADataArrival callack function channel number=%u vdwh.pVdData=%x rc=%d", vdwh.Type, vdwh.pVdData, rc);

	/* Get the pointer to the WD data */
    g_pWd = vdwh.pWdData;

	DebugWrite("Optaining dynamic Send function address (for sending data to)");
	
	/* Get leacy send function callback here */
    LegacySend = vdwh.pQueueVirtualWriteProc;			

    // Do extra initialization to determine if we are talking to an HPC client

    wdsi.WdInformationClass = WdVirtualWriteHookEx;
    wdsi.pWdInformation = &vdwhex;
    wdsi.WdInformationLength = sizeof(VDWRITEHOOKEX);
	
	// Set version to 0; older clients will do nothing
    vdwhex.usVersion = HPC_VD_API_VERSION_LEGACY;				
    
	rc = VdCallWd(pVd, WDxQUERYINFORMATION, &wdsi, &uiSize);

    DebugWrite("AppV: Optaining dynamic Enhanced HPC Send function address (for sending data to when client HPC) Version=%u p=%lx rc=%d", vdwhex.usVersion, vdwhex.pSendDataProc, rc);
    if(CLIENT_STATUS_SUCCESS != rc)
	{
		DebugWrite("AppV: WD WriteHookEx failed. rc %d", rc);
        return(rc);
    }
	
	// if version returned, this is HPC or later
    g_bIsHpc = (HPC_VD_API_VERSION_LEGACY != vdwhex.usVersion);	
	
	// save HPC SendData API address
	SendData = vdwhex.pSendDataProc;         
   
    // If it is an HPC client, tell it the highest version of the HPC API we support.
    if(g_bIsHpc)
    {
       WDSET_HPC_PROPERITES hpcProperties;

       hpcProperties.usVersion = HPC_VD_API_VERSION_V1;
       hpcProperties.pWdData = g_pWd;
       hpcProperties.ulVdOptions = HPC_VD_OPTIONS_NO_POLLING;

       wdsi.WdInformationClass = WdHpcProperties;
       wdsi.pWdInformation = &hpcProperties;
       wdsi.WdInformationLength = sizeof(WDSET_HPC_PROPERITES);

	   DebugWrite("AppV: WdSet_HPC_Properties Version=%u ulVdOptions=%lx g_pWd=%lx.", hpcProperties.usVersion, hpcProperties.ulVdOptions, hpcProperties.pWdData);
       rc = VdCallWd(pVd, WDxSETINFORMATION, &wdsi, &uiSize);
       
	   if(CLIENT_STATUS_SUCCESS != rc)
       {
		   DebugWrite("AppV: WdSet_HPC_Properties failed. rc %d", rc);
           return(rc);
       }
    }

	DebugWrite("AppV: Registered");

    // ALL MEMORY MUST BE ALLOCATED DURING INITIALIZATION.
    // Allocate a single buffer to respond to the mix request.
    // This example shows use of the MaximumWriteSize returned via
    // the previous call.
    // Subtract one because the first byte is used internally by the
    // WD for the channel number.  We are given a pointer to the
    // second byte.

    g_usMaxDataSize = vdwh.MaximumWriteSize - 1;

    // This is the main buffer which is going to be exchanged
    // between the client and the server. We allocate memory
    // of size g_usMaxDataSize rather than sizeof(PSendReceiveBuffer)
    // as the same buffer is used to send back additional data
    // to the server if required.

    if( NULL == (g_pAppV = (PSendReceiveBuffer)malloc(g_usMaxDataSize)))
	{
		return(CLIENT_ERROR_NO_MEMORY);
	}
	
    return(CLIENT_STATUS_SUCCESS);
}

#pragma warning(disable:4028)
DRIVER_API_CALLBACK static void WFCAPI ICADataArrival(PVD pVD, USHORT uchan, LPBYTE pBuf, USHORT Length)
//DRIVER_API_CALLBACK static void WFCAPI ICADataArrival(PVOID pVd, USHORT uChan, LPBYTE pBuf, USHORT Length)
{
	char* message;
	int rc;
	Prepacket prepacket;
	// Define a pointer pPacket
	WIRE_PTR(SendReceiveBuffer, pPacket);

	// Cast void pointer to SendReceiverBuffer ptr
	WIRE_READ(SendReceiveBuffer, pPacket, pBuf);

	DebugWrite("AppV: ICADataArrival entered. Data has arrived from the server.\n");

	// This protocol is completely synchronous - host should not send
	// another message with a pending response.

	if (!g_bBufferEmpty)
	{
		DebugWrite("AppV: ICADataArrival - Error: not all data was sent, buffer not empty. Will wait for next send. Returning.\n");
		return;
	}

	// Try call managed dll code:
	// https://support.microsoft.com/en-us/help/828736/how-to-call-a-managed-dll-from-native-visual-c-code-in-visual-studio.net-or-in-visual-studio-2005
	try
	{
		HRESULT hr = CoInitialize(NULL);

		InterfacePtr pInterface(__uuidof(Functions));
		long lResult = 0;
		BSTR response;

		_bstr_t message(pPacket->payload);
		DebugWrite("%s: Calling to managed function. Param '%s'\n", "AppV", pPacket->payload);
		pInterface->AddAsd(message, &response, &lResult);
		if (lResult != 0)
		{
			DebugWrite("%s: Call to managed function failed with error %d\n", "AppV", lResult);
		}
		CoUninitialize();
	}
	catch (...)
	{
		DebugWrite("%s: Error trying to call managed code\n", "AppV");
	}
	
	DebugWrite("AppV: Received packet langth of %d from the server: data is = %s.\n", pPacket->length, pPacket->payload);
	
	DebugWrite("AppV: Prepacket #1");
	message = "<Message>Hello from Receiver</Message>";
	g_pAppV = createPacketFromData(message, strlen(message) + 1);
	prepacket.length = g_pAppV->length;
	// Send Prepacket	
    g_bBufferEmpty = FALSE;
	g_MemorySections[0].pSection = (LPBYTE)&prepacket;		// The body of the data to be sent
	g_MemorySections[0].length = (USHORT)sizeof(Prepacket);
	DebugWrite("AppV: Prepacket #2");
	rc = SendData((DWORD)g_pWd, g_usVirtualChannelNum, g_MemorySections[0].pSection, g_MemorySections[0].length, &g_ulUserData, SENDDATA_NOTIFY);	
	DebugWrite("AppV: Prepacket #3");
	if (rc != CLIENT_STATUS_SUCCESS) {
		DebugWrite("AppV: Unable to SendData().\n");
	}
	
	DebugWrite("AppV: About to send response as '%s' with string length of '%d'", message, strlen(message));

	DebugWrite("AppV: #1");
    // Send actual data
    g_MemorySections[0].pSection = (LPBYTE)g_pAppV;		// The body of the data to be sent
    g_MemorySections[0].length = (USHORT)g_pAppV->length;	// Its length
	DebugWrite("AppV: #2");
	DebugWrite("AppV: Entire packet length is '%d'", g_pAppV->length);
	DebugWrite("AppV: #3");
	rc = SendData((DWORD)g_pWd, g_usVirtualChannelNum, g_MemorySections[0].pSection, g_MemorySections[0].length, &g_ulUserData, SENDDATA_NOTIFY);	
	DebugWrite("AppV: #3");
	if (rc != CLIENT_STATUS_SUCCESS) {
		DebugWrite("AppV: Unable to SendData().\n");
	}
	
	DebugWrite("AppV: Send '%d' Bytes", rc);

	g_bBufferEmpty = TRUE;
	// In the HPC case, drive the outbound data that we just put on the queue.  
	// Note that the HPC version of
	// SendData can be executed at any time on any thread.
	
	if(g_bIsHpc)
	{
		PrepareAvailableDataForEnhancedSend();
	}
	// else driverPoll will be used to send later
    return;
}

DRIVER_API INT DriverPoll(PVD pVD, PVOID pVdPoll, PUINT16 puiSize)
{
    int rc = CLIENT_STATUS_NO_DATA;

	// legacy DLLPOLL structure pointer
    PDLLPOLL pVdPollLegacy;

	// DLL_HPC_POLL structure pointer
    PDLL_HPC_POLL pVdPollHpc;        

	// Only print on first invocation
    static BOOL fFirstTimeDebug = TRUE;  

    if(fFirstTimeDebug == TRUE)
	{
		DebugWrite("AppV: DriverPoll entered");
    }

    // Trace the poll information.

	if(g_bIsHpc)
    {
        pVdPollHpc = (PDLL_HPC_POLL)pVdPoll;
        if(fFirstTimeDebug)
        {
			DebugWrite("AppV:DriverPoll. HPC Poll information: ulCurrentTime: %u, nFunction: %d, nSubFunction: %d, pUserData: %x.", pVdPollHpc->ulCurrentTime, pVdPollHpc->nFunction, pVdPollHpc->nSubFunction, pVdPollHpc->pUserData);
        }
    }
    else
    {
        pVdPollLegacy = (PDLLPOLL)pVdPoll;
        if(fFirstTimeDebug)
        {
			DebugWrite("AppV:DriverPoll. Legacy Poll information: ulCurrentTime: %u.", pVdPollLegacy->CurrentTime);
        }
    }
    fFirstTimeDebug = FALSE;                            // No more initial tracing

    // Check for something to write

    if(g_bBufferEmpty)
    {
        rc = CLIENT_STATUS_NO_DATA;
        goto Exit;			
    }

	// Data is available to write.  Send it.  Check for new HPC write API.
	    
	if(g_bIsHpc)
	{
        rc = PrepareAvailableDataForEnhancedSend();                      // send data via HPC client
	}
	else
	{
		// Use the legacy QueueVirtualWrite interface
		// Note that the FLUSH_IMMEDIATELY control will attempt to put the data onto the wire immediately,
		// causing any existing equal or higher priority data in the queue to be flushed as well.
		// This may result in the use of very small wire packets.  Using the value !FLUSH_IMMEDIATELY
		// may result in the data being delayed for a short while (up to 50 ms?) so it can possibly be combined
		// with other subsequent data to result in fewer and larger packets.
		
		// LegacySend(LPVOID, USHORT, LPMEMORY_SECTION, USHORT, USHORT)
		rc = LegacySend(g_pWd, g_usVirtualChannelNum, g_MemorySections, NUMBER_OF_MEMORY_SECTIONS, FLUSH_IMMEDIATELY);

		// Normal status returns are CLIENT_STATUS_SUCCESS (it worked) or CLIENT_ERROR_NO_OUTBUF (no room in output queue)

        if(CLIENT_STATUS_SUCCESS == rc)
        {
			DebugWrite("AppV:DriverPoll. g_fBufferEmpty made TRUE");
            g_bBufferEmpty = TRUE;
        }
        else if(CLIENT_ERROR_NO_OUTBUF == rc)
        {
            rc = CLIENT_STATUS_ERROR_RETRY;            // Try again later
        }
	}
Exit:
    return(rc);
}

DRIVER_API int DriverClose(PVD pVd, PDLLCLOSE pVdClose, PUINT16 puiSize)
{
	DebugWrite("AppV: DriverClose entered");

	// Free up the memory we allocated during DriverOpen()
	if (NULL != g_pAppV)
	{
		free(g_pAppV);
		g_pAppV = NULL;
	}
	return(CLIENT_STATUS_SUCCESS);
}

DRIVER_API int DriverInfo(IN PVD pVd, OUT PDLLINFO pVdInfo, OUT PUINT16 puiSize)
{		
	USHORT ByteCount;
	// Required data that needs to be sent to the host, which tells the host about this module.
	// Note this structure starts with a REQUIRED VD_C2H structure, but then you can take anything else to the back of it
	PVDAPPV_C2H pAboutMeData;
	PMODULE_C2H pHeader;
	PVDFLOW pFlow;
	DebugWrite("VDAPPV: DriverInfo entered");

	ByteCount = sizeof(VDAPPV_C2H);
	OUT *puiSize = sizeof(DLLINFO);
	
	// Check if buffer is big enough
	// If not, the caller is probably trying to determine the required
	// buffer size, so return it in ByteCount.

	if (pVdInfo->ByteCount < ByteCount)
	{
		pVdInfo->ByteCount = ByteCount;
		return(CLIENT_ERROR_BUFFER_TOO_SMALL);
	}

	// Initialize default, mandatory data

	pVdInfo->ByteCount = ByteCount;
	pAboutMeData = (PVDAPPV_C2H)pVdInfo->pBuffer;

	// Initialize module header

	pHeader = &pAboutMeData->Header.Header;
	pHeader->ByteCount = ByteCount;
	pHeader->ModuleClass = Module_VirtualDriver;

	pHeader->VersionL = CTXAPPV_VER_LO;
	pHeader->VersionH = CTXAPPV_VER_HI;
		
	strcpy_s((char*)(pHeader->HostModuleName), sizeof(pHeader->HostModuleName), "ICA"); // max 8 characters

	// Initialize virtual driver header part (Must be done)

	pFlow = &pAboutMeData->Header.Flow;
	pFlow->BandwidthQuota = 0;
	// No pacing of data sent my the server, we just get all of it as quickly as it can get to us
	pFlow->Flow = VirtualFlow_None;

	// add our own channel specific data...

	pAboutMeData->usMaxDataSize = g_usMaxDataSize; //useful way of telling the server the maximum size of data it can send

	// This will be sent OUT by virtue that its a pointer address
	pVdInfo->ByteCount = WIRE_WRITE(VDAPPV_C2H, pAboutMeData, ByteCount);
	
	return(CLIENT_STATUS_SUCCESS);
}

NOT_USED_BUT_REQUIRED DRIVER_API int DriverQueryInformation(IN PVD pVd, IN OUT PVDQUERYINFORMATION pVdQueryInformation, OUT PUINT16 puiSize)
{
	DebugWrite("AppV: DriverQueryInformation entered");
	DebugWrite("pVdQueryInformation->VdInformationClass = %d", pVdQueryInformation->VdInformationClass);

	*puiSize = sizeof(VDQUERYINFORMATION);

	return(CLIENT_STATUS_SUCCESS);
}

NOT_USED_BUT_REQUIRED DRIVER_API int DriverSetInformation(PVD pVd, PVDSETINFORMATION pVdSetInformation, PUINT16 puiSize)
{
	/* This function can receive two information classes:
	VdDisableModule: When the connection is being closed.
    VdFlush: When WFPurgeInput or WFPurgeOutput is called by the server-side virtual channel application. 
	The VdSetInformation structure contains a pointer to a VDFLUSH structure that specifies which purge function was called.*/

	DebugWrite("AppV: DriverSetInformation entered");
	DebugWrite("pVdSetInformation->VdInformationClass = %d", pVdSetInformation->VdInformationClass);

	return(CLIENT_STATUS_SUCCESS);
}

DRIVER_API int DriverGetLastError(PVD pVd, PVDLASTERROR pLastError)
{
	DebugWrite("AppV: DriverGetLastError entered");

	// Interpret last error and pass back code and string data

	pLastError->Error = pVd->LastError;
	pLastError->Message[0] = '\0';
	return(CLIENT_STATUS_SUCCESS);
}

// Prepares the data to be sent using the EnhancedSend() function.
int PrepareAvailableDataForEnhancedSend(void)
{
	int rc = CLIENT_STATUS_NO_DATA;
	if (g_bBufferEmpty)
	{
		rc = CLIENT_STATUS_NO_DATA;
		return(rc);
	}
	DebugWrite("AppV: PrepareAvailableDataForEnhancedSend()");

	// HPC does not support scatter write.  If there are multiple buffers to write
	// to a single packet, you must consolidate the buffers into a single buffer to 
	// send the data to the engine.

	if (NUMBER_OF_MEMORY_SECTIONS == 1)
	{
		// Finally send what is in the Memory_section via the HPC send function we determined earlier...
		rc = SendData((DWORD)g_pWd,	g_usVirtualChannelNum, g_MemorySections[0].pSection, g_MemorySections[0].length, &g_ulUserData, SENDDATA_NOTIFY);
	}
	else
	{
		// Consolidate the buffers into a single buffer.

		LPBYTE pba = g_pbaConsolidationBuffer;              // target buffer
		INT nIndex;
		USHORT usTotalLength = 0;                           // total length to send
		USHORT usSizeOfBufferLeft = SIZEOF_CONSOLIDATION_BUFFER;  // available buffer left to fill

		for (nIndex = 0; nIndex < NUMBER_OF_MEMORY_SECTIONS; ++nIndex)
		{
			memcpy_s(pba, usSizeOfBufferLeft, g_MemorySections[nIndex].pSection, g_MemorySections[nIndex].length);
			pba += g_MemorySections[nIndex].length;           // bump target pointer for next memory section
			usSizeOfBufferLeft -= g_MemorySections[nIndex].length;  // reduce available buffer
			usTotalLength += g_MemorySections[nIndex].length; // accumulate total length sent
		}

		// Now send the single buffer.
		rc = SendData((DWORD)g_pWd, g_usVirtualChannelNum, g_pbaConsolidationBuffer, usTotalLength, &g_ulUserData, SENDDATA_NOTIFY);
	}

	if (CLIENT_STATUS_SUCCESS == rc)
	{
		// Normal status returns are CLIENT_STATUS_SUCCESS (it worked)
		DebugWrite("AppV:PrepareAvailableDataForEnhancedSend. g_bBufferEmpty made TRUE; CLIENT_STATUS_SUCCESS");
		g_bBufferEmpty = TRUE;
	}
	else if (CLIENT_ERROR_NO_OUTBUF == rc)
	{
		// CLIENT_ERROR_NO_OUTBUF (no room in output queue)
		rc = CLIENT_STATUS_ERROR_RETRY;                        // Try again later
		DebugWrite("AppV:PrepareAvailableDataForEnhancedSend - CLIENT_ERROR_NO_OUTBUF (no room in output queue) Try again later");
	}
	else if (CLIENT_STATUS_NO_DATA == rc)
	{
		// There was nothing to do.  Just fall through.
	}
	else
	{
		// We may be waiting on a callback.  This would be indicated by a 
		// CLIENT_ERROR_BUFFER_STILL_BUSY return when we tried
		// to send data.  Just return CLIENT_STATUS_ERROR_RETRY in this case.

		if (CLIENT_ERROR_BUFFER_STILL_BUSY == rc)
		{
			rc = CLIENT_STATUS_ERROR_RETRY;                        // Try again later
		}
	}
	return(rc);
}

int vasprintf(char **strp, const char *fmt, va_list ap)
{
	int r = -1, size = _vscprintf(fmt, ap);
	if ((size >= 0) && (size < INT_MAX)) {
		*strp = (char *)malloc(size + 1); //+1 for null
		if (*strp) {
			r = vsnprintf_s(*strp, size + 1, size + 1, fmt, ap);  //+1 for null
			if ((r < 0) || (r > size)) {
				insane_free(*strp);
				r = -1;
			}
		}
	}
	else { *strp = 0; }
	return(r);
}

// Writes a message to debugwrite
// Accepts variable arguments
void DebugWrite(const char* fmt, ...)
{
	char* buffer = NULL;
	va_list args;

	va_start(args, fmt);
	vasprintf(&buffer, fmt, args);
	va_end(args);

	OutputDebugString(buffer);
	if (buffer)	{		
		insane_free(buffer);
	}	
}


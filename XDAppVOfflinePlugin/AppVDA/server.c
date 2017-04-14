/**********************************************************************
*
* ctxappv.c
*
* AppV Offline Streaming Client
*
**********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <wfapi.h>   // Citrix Server SDK
#include <appv.h>    // Citrix shared header
#include "ctxappv.h" // local header for APPV
#include "host.h"

#ifdef __cplusplus
}
#endif
#include <string>
#include <iostream>

#define EXPORT_ME __declspec(dllexport)

void DebugWrite(const char* fmt, ...);
VOID WINAPI PrintMessage(int nResourceID, ...);

/*
	Writes a string to the virtual channel and waits for an answer.
	returns 0 on success, otherwise the integer represents the error:
	0 - no errorm successfull operation
	1 - Cannot allocate send/recieve buffer
	2 - Unable to open virtual channel
	3 - Could not send message to virtual channel
	4 - Could not receive message from virtual channel. (prepacket)
	5 - Could not receive message from virtual channel. (actual data)
	6 - Unknown exception occured
	7 - Message bigger than protocol supports, send smaller packets
*/
extern "C" EXPORT_ME int SendAndRecieve(const char* message, int messageLength, char* responseBuf, int responseLength)
{	
	BOOLEAN     rc;
	HANDLE      hVC = NULL;
	PSendReceiveBuffer pBuffer = NULL;
	unsigned long ulBytesWritten;
	unsigned long   ulBytesRead;
	Prepacket prepacket;
	int error = 0;

	try 
	{
		// Allocate Read/Write buffer.
		if (pBuffer == NULL){
			if ((pBuffer = (PSendReceiveBuffer)malloc(sizeof(SendReceiveBuffer))) == NULL) {
				DebugWrite("AppV: Could not allocate send buffer structure.\n");
				error = 1;
				goto cleanup;
			}
			DebugWrite("AppV: Allocated buffer.\n");
		}

		// Open Virtual Channel.
		hVC = WFVirtualChannelOpen(WF_CURRENT_SERVER, WF_CURRENT_SESSION, CTXAPPV_VIRTUAL_CHANNEL_NAME);
		if (hVC) {
			DebugWrite("AppV: Virtual Channel successfully opened.\n");
		}
		else {
			DebugWrite("AppV: Unable to open virtual channel\n");
			error = 2;
			goto cleanup;
		}

		// Get the channel header data with attached custom appv info to obtain maximum message length
		PVDAPPV_C2H pAboutMeData = NULL;
		rc = WFVirtualChannelQuery(hVC,	WFVirtualClientData,(PVOID*) &pAboutMeData, &ulBytesRead);
		if (rc != TRUE) {
			OutputDebugString("WFVirtualChannelQuery failed.\n");
		}

		DebugWrite("AppV: Received maximum send size for channel as %d.\n", pAboutMeData->usMaxDataSize);

		// Can't send a message larger than the maximum buffer length the protocol supports
		if (pAboutMeData->usMaxDataSize - 4 < messageLength)
		{
			error = 7;
			goto cleanup;
		}
			
	
		// Write a message to channel	
		pBuffer = createPacketFromData(message, messageLength + 1);		
		rc = WFVirtualChannelWrite(hVC, (PCHAR)pBuffer, pBuffer->length, &ulBytesWritten);
		if (rc != TRUE) {
			DebugWrite("AppV: Could not send message to virtual channel.\n");
			PrintMessage(GetLastError());	
			error = 3;
			goto cleanup;
		}

		DebugWrite("AppV: Sent Message (%s) of (%lu) bytes from virtual channel.\n", pBuffer->payload, ulBytesWritten);

		// Read Response: 
		// A response copnsists of two packets sent from the client.(PrePacket and a SendReceiveBuffer)
		// The Prepacket contains the length of data in the SendReceiveBuffer that is sent next

		// Read prepacket
		rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)&prepacket, sizeof(Prepacket), &ulBytesRead);
		if (rc != TRUE) {
			DebugWrite("AppV: Could not receive prepacket from virtual channel.\n");
			PrintMessage(GetLastError());		
			error = 4;
			goto cleanup;
		}

		DebugWrite("AppV: Prepacket advised length to read is '%d'!", prepacket.length);
		free(pBuffer);
		pBuffer = (PSendReceiveBuffer)malloc(pAboutMeData->usMaxDataSize);
		// Read actual data	
		rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)pBuffer, prepacket.length, &ulBytesRead);
		if (rc != TRUE) {
			DebugWrite("AppV: Could not receive actual message from virtual channel. error=%d\n",rc);
			PrintMessage(GetLastError());
			error = 5;
			goto cleanup;
		}
		DebugWrite("AppV: WFVirtualChannelRead Finished.");
		DebugWrite("AppV: Bytes read is '%d'", ulBytesRead);
		DebugWrite("AppV: Received Message (%s) of (%lu) bytes from virtual channel.\n", pBuffer->payload, ulBytesRead);
		
		// Copy the response to the buffer provided by the caller
		strcpy_s(responseBuf, pBuffer->length, pBuffer->payload);
		return 0;
	}
	catch (...) {
		DebugWrite("AppV: Unknown exception occured\n");
		// this defaults to cleanup:
		error = 6;
	}

cleanup:
	WFVirtualChannelClose(hVC);
	if (pBuffer) {
		free(pBuffer);
		pBuffer = NULL;
	}
	return error;	
}

// for reporting Citrix errors
VOID WINAPI PrintMessage(int nResourceID, ...)
{
	_TCHAR str1[MAX_IDS_LEN + 1], str2[2 * MAX_IDS_LEN + 1];

	va_list args;
	va_start(args, nResourceID);

	if (LoadString(NULL, nResourceID, (LPTSTR)str1, sizeof(str1) / sizeof(*str1))) {

		_vstprintf_s(str2, MAX_IDS_LEN + 1, str1, args);
		_tprintf(str2);
		DebugWrite(str2);

	}
	else {

		_ftprintf(stderr,
			_TEXT("PrintMessage(): LoadString %d failed, Error %ld)\n"),
			nResourceID, GetLastError());
	}

	va_end(args);

}

// Needed for DebugWrite
int vasprintf(char **strp, const char *fmt, va_list ap)
{
#define insane_free(ptr) { free(ptr); ptr = 0; }
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

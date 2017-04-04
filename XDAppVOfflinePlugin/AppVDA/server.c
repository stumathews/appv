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

#define EXPORT_ME __declspec(dllexport)

PSendReceiveBuffer pBuffer = NULL;
HANDLE      hVC = NULL;
BOOLEAN     rc;
PVDAPPV_C2H pAboutMeData = NULL;
unsigned long ulBytesWritten;
unsigned long   ulBytesRead;
Prepacket prepacket;

VOID WINAPI PrintMessage(int nResourceID, ...);
void SafelyExit(PSendReceiveBuffer pBuffer, HANDLE h);
void DoProtocolWork();


extern "C" EXPORT_ME void CloseVirtualChannel() 
{
	SafelyExit(pBuffer, hVC);
}

extern "C"  EXPORT_ME void CreateVirtualChannel()
{	
	int max_try;
	
	
	max_try = 0;

	if ((pBuffer = (PSendReceiveBuffer)malloc(sizeof(SendReceiveBuffer))) == NULL) {
		_tprintf(_T("Could not allocate send buffer structure.\n"));
		SafelyExit(pBuffer, hVC);
		return;
	}

	// Open Virtual Channel.
	hVC = WFVirtualChannelOpen(WF_CURRENT_SERVER, WF_CURRENT_SESSION, CTXAPPV_VIRTUAL_CHANNEL_NAME);
	if (hVC) {
		_tprintf(_T("Virtual Channel successfully opened.\n"));
	}
	else {
		_tprintf(_T("Unable to open virtual channel\n"));
		SafelyExit(pBuffer, hVC);
		return;
	}
	DoProtocolWork();

}



void DoProtocolWork()
{
	
	char* message;
	// Get the channel header data with attached custom appv info
	rc = WFVirtualChannelQuery(hVC,	WFVirtualClientData,(PVOID*) &pAboutMeData, &ulBytesRead);
	if (rc != TRUE) {
		_tprintf(_T("WFVirtualChannelQuery failed.\n"));
		SafelyExit(pBuffer, hVC);
	}	
	_tprintf(_T("Received maximum send size for channel as %d.\n"),pAboutMeData->usMaxDataSize);

	// Write a message to chanel
	message = "<Message>Hello from the server!</Message>";
	pBuffer = createPacketFromData(message, strlen(message) + 1);
	rc = WFVirtualChannelWrite(hVC, (PCHAR)pBuffer, pBuffer->length, &ulBytesWritten);
	if (rc != TRUE) {
		_tprintf(_T("Could not send message to virtual channel.\n"));
		PrintMessage(GetLastError());
		SafelyExit(pBuffer, hVC);
		return;
	}	
	_tprintf(_T("Sent Message (%s) of (%lu) bytes from virtual channel.\n"), pBuffer->payload, ulBytesWritten);
	
	//Read Response 
	// A response copnsists of two packets sent from the client.(PrePacket and a SendReceiveBuffer)
	// The Prepacket contains the length of data in the SendReceiveBuffer that is sent next

	// Read prepacket
	rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)&prepacket, sizeof(Prepacket), &ulBytesRead);
	if (rc != TRUE) {
		_tprintf(_T("Could not receive message from virtual channel.\n"));
		PrintMessage(GetLastError());
		SafelyExit(pBuffer, hVC);
		return;
	}	
	
	// Read actual data	
	rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)pBuffer, prepacket.length, &ulBytesRead);
	if (rc != TRUE) {
		_tprintf(_T("Could not receive message from virtual channel.\n"));			
		PrintMessage(GetLastError());
		SafelyExit(pBuffer, hVC);
		return;			
	}	
	_tprintf(_T("Received Message (%s) of (%lu) bytes from virtual channel.\n"), pBuffer->payload, ulBytesRead);
	SafelyExit(pBuffer, hVC);
}


/* Cleans up before the program exists - for whatever reason*/
void SafelyExit(PSendReceiveBuffer pBuffer, HANDLE h)
{
	if (pBuffer) {
		free(pBuffer);
		pBuffer = NULL;
	}
	WFVirtualChannelClose(h);
}

VOID WINAPI PrintMessage(int nResourceID, ...)
{
	_TCHAR str1[MAX_IDS_LEN + 1], str2[2 * MAX_IDS_LEN + 1];

	va_list args;
	va_start(args, nResourceID);

	if (LoadString(NULL, nResourceID, (LPTSTR)str1, sizeof(str1) / sizeof(*str1))) {

		_vstprintf_s(str2, MAX_IDS_LEN + 1, str1, args);
		_tprintf(str2);

	}
	else {

		_ftprintf(stderr,
			_TEXT("PrintMessage(): LoadString %d failed, Error %ld)\n"),
			nResourceID, GetLastError());
	}

	va_end(args);

}

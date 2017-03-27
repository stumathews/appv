/**********************************************************************
*
* ctxappv.c
*
* AppV Offline Streaming Client
*
**********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <wfapi.h>   // Citrix Server SDK
#include <appv.h>    // Citrix shared header
#include "ctxappv.h" // local header for APPV
#include "host.h"

VOID WINAPI PrintMessage(int nResourceID, ...);
void SafelyExit();

PSendReceiveBuffer pBuffer = NULL;

HANDLE      hVC = NULL;
BOOLEAN     rc;
PVDAPPV_C2H pAboutMeData = NULL;
unsigned long ulBytesWritten;
unsigned long   ulBytesRead;

void print(char* message)
{
	_tprintf(_T(message));
}

#pragma warning(disable:4028)
#pragma warning(disable:4029)
int WriteVirtualChannelMessage(PSendReceiveBuffer buffer, const char *message)
{
	// populate our buffer properly
	buffer = createPacketFromData(message, strlen(message) + 1, print);

	_tprintf(_T("Write a send buffer to chanel...\n"));
	rc = WFVirtualChannelWrite(hVC, (PCHAR)buffer, buffer->length, &ulBytesWritten);

	if (rc != TRUE) {
		_tprintf(_T("Could not send message to virtual channel.\n"));
		PrintMessage(GetLastError());
		SafelyExit();
		return 0;
	}
	_tprintf(_T("Wrote %lu bytes to virtual channel.\n"), ulBytesWritten);
	_tprintf(_T("Wrote payload '%s' to virtual channel.\n"), buffer->payload);
	return ulBytesWritten;
}

char* ReadVirtualChannelMessage(PSendReceiveBuffer buffer)
{
	_tprintf(_T("Read a message form the channel...\n"));

	// Read the length of the payload
	rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)&(pBuffer->length), sizeof(int), &ulBytesRead);
	_tprintf(_T("Received length as %d bytes.\n"), pBuffer->length);
	// Read the payload based on previously received length for the payload
	rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)(pBuffer->payload), rc, &ulBytesRead);
	_tprintf(_T("Received data as '%s'.\n"), pBuffer->payload);

	if (rc != TRUE) {
		_tprintf(_T("Could not receive message from virtual channel.\n"));
		SafelyExit();
		return NULL;
	}
	_tprintf(_T("Received %lu bytes from virtual channel.\n"), ulBytesWritten);
	_tprintf(_T("Message received from receiver: %s of length %d "), pBuffer->length, pBuffer->payload);

	return pBuffer->payload;
}

int __cdecl main(INT argc, CHAR **argv)
{
	char* message;
	// Allocate our reusable send/recieve buffer	
	
	if ((pBuffer = (PSendReceiveBuffer)malloc(sizeof(SendReceiveBuffer))) == NULL) {
		_tprintf(_T("Could not allocate send buffer structure.\n"));
		SafelyExit(); 
		return; 
	}

	// Open Virtual Channel.
	hVC = WFVirtualChannelOpen(WF_CURRENT_SERVER, WF_CURRENT_SESSION, CTXAPPV_VIRTUAL_CHANNEL_NAME);
	if (hVC) {
		_tprintf(_T("Virtual Channel successfully opened.\n"));
	}
	else {
		_tprintf(_T("Unable to open virtual channel\n"));
		SafelyExit();
		return;
	}

	// Get the channel header data with attached custom appv info
	rc = WFVirtualChannelQuery(hVC,	WFVirtualClientData,(PVOID*) &pAboutMeData, &ulBytesRead);
	if (rc != TRUE) {
		_tprintf(_T("WFVirtualChannelQuery failed.\n"));
		SafelyExit();		
	}
	
	_tprintf(_T("Received maximum send size for channel as %d. Using this as maximum send buffer"),pAboutMeData->usMaxDataSize);

	// Write a message to chanel
	
	message = "<Message>Hello</Message>";
	rc = WriteVirtualChannelMessage(pBuffer, (const char*) message);
	ReadVirtualChannelMessage(pBuffer);
	
	SafelyExit();
}



/* Cleans up before the program exists - for whatever reason*/
void SafelyExit()
{
	if (pBuffer) {
		free(pBuffer);
		pBuffer = NULL;
	}
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

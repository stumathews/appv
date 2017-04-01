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

int __cdecl main(INT argc, CHAR **argv)
{
	char* message;	
	char* test;
	int max_try;	
	Prepacket prepacket;

	max_try = 0;

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
	
	_tprintf(_T("Received maximum send size for channel as %d. Using this as maximum send buffer\n"),pAboutMeData->usMaxDataSize);
	test = malloc(sizeof(char)*pAboutMeData->usMaxDataSize);
	// Write a message to chanel	
	message = "<Message>Hello from the server!</Message>";
	pBuffer = createPacketFromData(message, strlen(message) + 1);

	rc = WFVirtualChannelWrite(hVC, (PCHAR)pBuffer, pBuffer->length, &ulBytesWritten);
	if (rc != TRUE) {
		_tprintf(_T("Could not send message to virtual channel.\n"));
		PrintMessage(GetLastError());
		SafelyExit();
		return -1;
	}
	_tprintf(_T("Wrote %lu bytes to virtual channel. Message(%s)\n"), ulBytesWritten, pBuffer->payload);

	// read prepacket	

	rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)&prepacket, sizeof(Prepacket), &ulBytesRead);
	if (rc != TRUE) {
		_tprintf(_T("Could not receive message from virtual channel.\n"));
		PrintMessage(GetLastError());
		SafelyExit();
		return;
	}
	_tprintf(_T("Received %lu bytes from virtual channel. Message(%d)\n"), ulBytesRead, prepacket.length);

	_tprintf(_T("Attemptiong to read actual data of %d bytes...\n"), prepacket.length);
	
	// read actual data
	rc = FALSE;
	while (rc != TRUE && max_try < 3) {
		rc = WFVirtualChannelRead(hVC, VC_TIMEOUT_MILLISECONDS, (PCHAR)pBuffer, prepacket.length, &ulBytesRead);
		if (rc != TRUE) {
			_tprintf(_T("Could not receive message from virtual channel.\n"));			
			max_try++;
			_tprintf(_T("Sleeping....\n"));
			Sleep(2000);
			
		}
	}
	if (max_try == 3) { _tprintf(_T("Finished waiting...\n"), prepacket.length); }
	if (rc != TRUE)
	{
		PrintMessage(GetLastError());
		SafelyExit();
		return;
	}
	_tprintf(_T("Attemptiong to read actual data of %d bytes...\n"), prepacket.length);
	_tprintf(_T("Received %lu bytes from virtual channel.\n"), ulBytesRead);
	_tprintf(_T("Received (%s) bytes from virtual channel.\n"), pBuffer->payload);
	

	_tprintf(_T("Done"));

	SafelyExit();
}



/* Cleans up before the program exists - for whatever reason*/
void SafelyExit()
{
	_tprintf(_T("Entering SafelyExit()"));
	if (pBuffer) {
		free(pBuffer);
		pBuffer = NULL;
	}
	_tprintf(_T("Exiting SafelyExit()"));
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

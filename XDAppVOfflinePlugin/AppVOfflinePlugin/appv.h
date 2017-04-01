#ifndef __VDAPPV_H__
#define __VDAPPV_H__

#include <ica.h>
#include <ica-c2h.h>        /* for VD_C2H structure */

#define CTXAPPV_VIRTUAL_CHANNEL_NAME  "CTXAPPV"
#define VC_TIMEOUT_MILLISECONDS     10000L

/*
 * Lowest and highest compatable version.  See DriverInfo().
 */
#define CTXAPPV_VER_LO      1
#define CTXAPPV_VER_HI      1

#pragma pack(1)

typedef struct Prepacket {
	int length;
} Prepacket;

/* AppV buffer format*/
typedef struct SRB {	
	int length;
	char payload[1];
} SendReceiveBuffer, *PSendReceiveBuffer;


/* vdping driver info (client to host) structure
This structure can be a developer-defined virtual channel-specific structure, but it must begin with a
VD_C2H structure, which in turn begins with a MODULE_C2H structure. All fields of the
VD_C2H structure must be filled in except for the ChannelMask field. See ica-c2h.h (in src\inc\)
for definitions of these structures.
*/
typedef struct _VDAPPV_C2H
{
	VD_C2H  Header;
	USHORT  usMaxDataSize;      /* maximum data block size */
} VDAPPV_C2H, *PVDAPPV_C2H;

#pragma pack()

unsigned packetPayloadLength(PSendReceiveBuffer packet) { return packet->length - sizeof(PSendReceiveBuffer) - 1; }
PSendReceiveBuffer createPacketFromData(const char* data, unsigned dataSize)
{
	unsigned packetSize = sizeof(SendReceiveBuffer) + dataSize - 1;	
	PSendReceiveBuffer packet = (PSendReceiveBuffer)malloc(packetSize);	
	packet->length = packetSize;
	memcpy(packet->payload, data, dataSize);	
	return packet;
}

#endif /* __VDAPPV_H__ */

#ifndef UNIT_H
#define UNIT_H

#define UNIT_SHORT_STRING	10
#define UNIT_MIDDLE_STRING	20
#define UNIT_LONG_STRING		50

#define UNIT_MAX_CAPABILITIES	5

/* Library includes. */
#include "hw_types.h"
#include "uip.h"

typedef enum
{
	UNIT_READY,
	UNIT_BUSY
}eUnitState;

typedef struct
{
	char Software[UNIT_SHORT_STRING];
	char Hardware[UNIT_SHORT_STRING];
	char SoftwareInfo[UNIT_LONG_STRING];
	char HardwareInfo[UNIT_LONG_STRING];
}tUnitVersion;

typedef struct
{
	char Type[UNIT_MIDDLE_STRING];
	void * pxDependancy;
}tUnitCapability;

typedef struct
{
	uip_ipaddr_t xSrcAddr;
	u16_t uSrcPort;
	uip_ipaddr_t xDstAddr;
	u16_t uDstPort;
	tBoolean bSeqNo;
	tBoolean bRelTime;
	tBoolean bAck;
	unsigned int uDs;
	unsigned int uDt;
}tUnitJob;

typedef void (* tcbNewJob) (tUnitJob xNewJob);

typedef struct
{
	char Name[UNIT_MIDDLE_STRING];
	tUnitVersion xVersion;
	eUnitState xState;
	tUnitCapability xCapabilities[UNIT_MAX_CAPABILITIES];
	tcbNewJob vNewJob;
}tUnit;

#endif
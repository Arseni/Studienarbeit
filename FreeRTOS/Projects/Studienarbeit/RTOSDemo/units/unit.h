#ifndef UNIT_H
#define UNIT_H

#define UNIT_SHORT_STRING	10
#define UNIT_MIDDLE_STRING	20
#define UNIT_LONG_STRING		50

#define UNIT_MAX_CAPABILITIES			5
#define UNIT_MAX_GLOBAL_UNITS			5
#define UNIT_MAX_GLOBAL_JOBS_PARALLEL	10
#define UNIT_JOB_QUEUE_LENGTH			10
#define UNIT_MAX_WAITTIME_ON_FULL_QUEUE 1000
#define INITIAL_BROADCAST_SEND_PERIOD	1000

#define INITIAL_ADDR	0xFF,0xFF,0xFF,0xFF
#define INITIAL_PORT	50001
#define COMM_PORT		50001

/* Library includes. */
#include "hw_types.h"
#include "uip.h"
#include <string.h>

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

#define UNIT_CAPABILITY_VALID(X) (strlen(X.Type)>0?true:false)
#define UNIT_CAPABILITIES_CMP(X,Y) (strcmp(X.Type, Y.Type))

typedef struct
{
	char unitName[UNIT_MIDDLE_STRING];
	tUnitCapability xCapability;
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

typedef struct
{
	char Text[UNIT_MIDDLE_STRING];
}tUnitValue;

typedef void (* tcbUnitNewJob) (tUnitJob xNewJob);

typedef struct
{
	char Name[UNIT_MIDDLE_STRING];
	tUnitVersion xVersion;
	eUnitState xState;
	tUnitCapability xCapabilities[UNIT_MAX_CAPABILITIES];
	tcbUnitNewJob vNewJob;
}tUnit;



void vUnitHandlerTask(void * pvParameters);
tUnit * xUnitCreate(char * Name, tcbUnitNewJob JobReceived);
tBoolean xUnitUnlink(tUnit * pUnit);
tBoolean bUnitAddCapability(tUnit * pUnit, tUnitCapability Capability);

#endif

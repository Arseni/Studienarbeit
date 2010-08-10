#ifndef UNIT_H
#define UNIT_H

#define UNIT_TX_BUFFER_LEN			1024
#define UNIT_XML_TREE_BUFFER_LEN	4000

#define UNIT_SHORT_STRING	10
#define UNIT_MIDDLE_STRING	20
#define UNIT_LONG_STRING	50

#define UNIT_MAX_CAPABILITIES			5
#define UNIT_MAX_GLOBAL_UNITS			5
#define UNIT_MAX_GLOBAL_JOBS_PARALLEL	10
#define UNIT_JOB_QUEUE_LENGTH			10
#define UNIT_MAX_WAITTIME_ON_FULL_QUEUE 1000
#define INITIAL_BROADCAST_SEND_PERIOD	1000
#define UNIT_JOB_VALIDATION_INTERVALL	100
#define UNIT_JOB_TIMEOUT				10000

#define INITIAL_ADDR	0xFF,0xFF,0xFF,0xFF
#define INITIAL_PORT	50001
#define COMM_PORT		50001

/* Library includes. */
#include "hw_types.h"
#include "portmacro.h"
#include "uip.h"
#include "muXML/muXMLTree.h"
#include <string.h>

typedef enum
{
	UNIT_READY,
	UNIT_BUSY
}eUnitState;

typedef enum
{
	JOB_ACK = (1<<0),
	JOB_STORE = (1<<1),
	JOB_COMPLETE = (1<<2)
}eUnitJobState;

typedef enum
{
	CAPA_Ready,
	CAPA_Busy,
	CAPA_Restricted,
	CAPA_Failure
}eUnitCapaState;

typedef struct
{
	char Software[UNIT_SHORT_STRING];
	char Hardware[UNIT_SHORT_STRING];
	char SoftwareInfo[UNIT_LONG_STRING];
	char HardwareInfo[UNIT_LONG_STRING];
}tUnitVersion;

typedef struct
{
	char Name[UNIT_MIDDLE_STRING];
	char * StateDescription;
	eUnitCapaState State;
}tUnitCapability;

#define UNIT_CAPABILITY_VALID(X) (strlen(X.Name)>0?true:false)
#define UNIT_CAPABILITIES_CMP(X,Y) (strcmp(X.Name, Y.Name))

#define bUnitIsCapabilityName(X) (strcmp(job->Element.Name, X) == 0) // Name der adressierten Capability abgleichen
#define bUnitHasAttribute(X) (muXMLGetAttributeByName(job, X) != NULL)
#define bUnitIsAttrValue(X,Y) (strcmp(muXMLGetAttributeByName(job, X), Y) == 0)
#define xUnitGetAttrValue(X) (muXMLGetAttributeByName(job, X))



typedef struct
{
	char Text[UNIT_MIDDLE_STRING];
}tUnitValue;

typedef eUnitJobState (* tcbUnitNewJob) (struct muXMLTreeElement * Job, int uid);

typedef struct
{
	char Name[UNIT_MIDDLE_STRING];
	tUnitCapability xCapabilities[UNIT_MAX_CAPABILITIES];
	tUnitVersion xVersion;
	eUnitState xState;
	tcbUnitNewJob vNewJob;
}tUnit;

struct tUnitJobHandler
{
	struct muXMLTreeElement * job;
	int internal_uid;

	tUnit * unit;
	uip_udp_endpoint_t endpoint;

	int uid;
	int seqNo;
	tBoolean relTime;
	tBoolean ack;
	unsigned int ds;
	unsigned int dt;
	char statusFlags;

	portTickType startTime;
	int timeout;
	tBoolean store;
	tBoolean inUse;
};

void vUnitHandlerTask(void * pvParameters);
tUnit * xUnitCreate(char * Name, tcbUnitNewJob JobReceived);
tBoolean xUnitUnlink(tUnit * pUnit);
tUnitCapability * xUnitAddCapability(tUnit * pUnit, char * Name);
tBoolean bUnitUpdateCapability(tUnitCapability * pCapability, char * stateDesc, eUnitCapaState state);
tBoolean bUnitUnlinkCapability(tUnitCapability * pCapability);

tUnit * unitGetUnitByName(char * Name);

tBoolean bUnitSend(struct muXMLTreeElement * xjob, int uid);


#endif

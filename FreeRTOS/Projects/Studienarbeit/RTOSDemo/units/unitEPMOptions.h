#ifdef OBSOLETE
#ifndef UNIT_EPMOPTIONS_H
#define UNIT_EPMOPTIONS_H

#include "unit.h"

#define STATUS_USE_SEQ_NO		(1<<0)
#define STATUS_USE_UID			(1<<1)
#define STATUS_ACK_REQUESTED	(1<<2)
#define STATUS_PERIODIC			(1<<3)
#define STATUS_DELETE			(1<<4)

struct tUnitEPMOption
{
	const char * Name;
	void (* preProcess)(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root);
	void (* postProcess)(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root);
};

// Define functions here
// reply
void replyPreProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root);
void replyPostProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root);
//withseqno
void withseqnoPreProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root);
void withseqnoPostProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root);

static struct tUnitEPMOption unitEPMOption[] = {
		{"reply", replyPreProcess, replyPostProcess},
		{"withseqno", withseqnoPreProcess, withseqnoPostProcess}
};
#endif
#endif

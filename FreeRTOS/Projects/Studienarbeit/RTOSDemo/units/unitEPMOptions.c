#ifdef OBSOLETE
#include "unitEPMOptions.h"

void replyPreProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root)
{
	if(strcmp(value, "never") == 0)
	{
		handle->statusFlags |= STATUS_DELETE;
	}
	if(strcmp(value, "onchange") == 0)
	{
		muXMLUpdateAttribute(root->SubElements->SubElements, "onchange", "yes");
	}
}

void replyPostProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root)
{
}

void withseqnoPreProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root)
{
	if(strcmp(value, "yes") == 0)
	{
		handle->statusFlags |= STATUS_USE_SEQ_NO;
		handle->seqNo = 1;
	}
}

void withseqnoPostProcess(char * value, struct tUnitJobHandler * handle, struct muXMLTreeElement * root)
{
	handle->seqNo++;
}
#endif

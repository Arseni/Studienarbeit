#include <stdlib.h>
#include "muXML/muXMLAccess.h"
#include "unit.h"

#define updateAttr(X,Y,Z) muXMLUpdateAttribute(X,Y,Z)
#define isCapability(X,Y) (strcmp(X->Element.Name, Y) == 0) // Name der adressierten Capability abgleichen
#define hasAttribute(X,Y) (muXMLGetAttributeByName(X, Y) != NULL)
#define isAttrValue(X,Y,Z) (strcmp(muXMLGetAttributeByName(X, Y), Z) == 0)
#define getAttrValue(X,Y) muXMLGetAttributeByName(X,Y)
#define getData(X)	(X->Data)

#define toInt(X) atoi(X)

#define HANDLE_STD_CAPABILITEIS() do{if(isCapability(job, "ack") || isCapability(job, "info") || isCapability(job, "readystate"))return JOB_ACK;}while(0);
#define HANDLE_CAPABILITY(X) if(isCapability(job, X))

typedef struct muXMLTreeElement unitToken;

int strToInt(char * data, char * type);
char * intToStr(char * buffer, int number);
tBoolean stringToStream(char * buffer, char * data, char * type);
tBoolean streamToString(char * buffer, char * data, char * type);
tBoolean updateData(struct muXMLTreeElement * subtree, char * buffer, char * data, char * type);
void createBareByRequest(struct muXMLTreeElement * dest, struct muXMLTreeElement * src);

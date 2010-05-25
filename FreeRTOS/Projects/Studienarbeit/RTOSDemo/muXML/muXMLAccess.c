
#include <stdlib.h>
#include <string.h>
#include "muXMLTree.h"

struct muXMLTreeElement * muXML_GetElementByName(struct muXMLTreeElement * pRoot, char * name)
{
	struct muXMLTreeElement * hopper = pRoot->SubElements;
	
	while(hopper != NULL && strstr(name, hopper->Element.Name) != NULL)
	{
		hopper = hopper->Next;
	}
	
	return hopper;
}

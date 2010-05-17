
#include <stdlib.h>
#include <string.h>
#include "muXMLTree.h"

/*----------------------------      Reading      -----------------------------*/

/**
 * Sucht nach einem Knoten eine Hierarchieebene unter dem übergebenen Knoten und
 * gibt den zurück
 * @ pRoot : Knoten, der durchsucht werden soll
 * @ pName : String, nachdem gesucht werden soll
 * + return: Pointer auf den gefundenen Knoten bei Erfolg, sonst NULL
 */
struct muXMLTreeElement* muXML_GetElementByName(struct muXMLTreeElement * pRoot,
												char * Name)
{
	struct muXMLTreeElement * hopper = pRoot->SubElements;

	// Suche alle Unterknoten durch
	while(hopper != NULL && strcmp(Name, hopper->Element.Name) != 0)
	{
		hopper = hopper->Next;	
	}
	
	return hopper;
}



/*----------------------------      Writing      -----------------------------*/


int muXMLUpdateData(struct muXMLTree * pTree,
					struct muXMLTreeElement * pElement,
					char * newData)
{
	int i=0, dataLen = strlen(newData);
	if(dataLen > pElement->Data.DataSize)
	{
		//for(i = pTree->StorageInfo.
	}
	return 0;
}

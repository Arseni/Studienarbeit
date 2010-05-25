
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
struct muXMLTreeElement* muXMLGetElementByName(struct muXMLTreeElement * pRoot,
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

/**
 * Ersetzt das Datum eines Elements durch ein neues. Passt dabei die Länge des
 * Baumes an.
 * @ pTree   : Pointer auf den Gesamtbaum, der bearbeitet wird
 * @ pElement: Pointer auf das Element, indem das Datum verädert werden soll
 * @ newData : Nullterminiterter String des neuen Datums
 * + return  : Passt nicht in den Buffer:1 , sonst 0
 */
int muXMLUpdateData(struct muXMLTree * pTree,
					struct muXMLTreeElement * pElement,
					char * newData)
{
	int i=0, dataLen = strlen(newData),
		diff = dataLen - strlen(pElement->Data.Data);
	char * pData;
	
	if(pTree->StorageInfo.SpaceInUse + diff > pTree->StorageInfo.SpaceTotal)
		return 1;
	
	// Vorgang der Datenverschiebung je nach diff unterschiedlich
	if(diff > 0) // neue Daten sind größer
	{
		pData = (char*)pTree + pTree->StorageInfo.SpaceInUse;
		while(pData != pElement->Data.Data)
		{
			*(pData + diff) = *pData;
			pData--;
		}
	}
	if(diff < 0) // neue Daten sind kleiner
	{
		pData = pElement->Data.Data;
		while(pData != (char*)pTree + pTree->StorageInfo.SpaceInUse)
		{
			*(pData - diff) = *pData;
			pData++;
		}
	}

	// neue Daten übernehmen
	memcpy(pElement->Data.Data, newData, dataLen);
	pElement->Data.DataSize = dataLen;
	pTree->StorageInfo.SpaceInUse += diff;

	// Ketteninformationen updaten
	if(pElement->Next)
		pElement->Next = (char*)pElement->Next + diff;
	return 0;
}


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

void * muXMLCreateTree(void * Buffer, int BufferLength, char * rootName)
{
	memset(Buffer, 0, sizeof(struct muXMLTree));
	strcpy(((struct muXMLTree*)Buffer)->Root.Element.Name, rootName);
#ifdef FILE_INFO
	((struct muXMLTree*)Buffer)->StorageInfo.SpaceTotal = BufferLength;
#endif
	return (struct muXMLTree*)Buffer + 1;
}

void * muXMLCreateElement(void * Buffer, char * name)
{
	memset(Buffer, 0, sizeof(struct muXMLTreeElement));
	strcpy(((struct muXMLTreeElement*)Buffer)->Element.Name, name);
	return ((struct muXMLTreeElement*)Buffer) + 1;
}

/**
 * Ersetzt das Datum eines Elements durch ein neues. Passt dabei die Länge des
 * Baumes an.
 * @ pTree   : Pointer auf den Gesamtbaum, der bearbeitet wird
 * @ pElement: Pointer auf das Element, indem das Datum verädert werden soll
 * @ newData : Nullterminiterter String des neuen Datums
 * + return  : Pointer auf ersten freien Platz im newData Buffer
 */
void * muXMLUpdateData(struct muXMLTreeElement * pElement,
					   char * newData)
{
	pElement->Data.Data = newData;
	pElement->Data.DataSize = strlen(newData);
	return newData + strlen(newData) + 1;
}

/**
 * Ändert den Wert eines Attributs. Ist ein Attribut nicht vorhanden
 * wird es eingerichtet
 */
int muXMLUpdateAttribute(struct muXMLTreeElement * pElement,
						 char * AttrName,
						 char * AttrValue)
{
	int i;

	//validate parameter (strlen etc)
	if( (strlen(AttrName) > muXML_MAX_ATTRIBUTE) ||
		(strlen(AttrValue) > muXML_MAX_VALUE) )
		return 1;

	
	for(i=0; i<muXML_MAX_ATTRIBUTE_CNT; i++)
	{
		if(strcmp(pElement->Element.Attribute[i].Name, AttrName) == 0)
		{
			strcpy(pElement->Element.Attribute[i].Value, AttrValue);
			return 0;
		}
	}
	if(pElement->Element.nAttributes < muXML_MAX_ATTRIBUTE_CNT)
	{
		strcpy(pElement->Element.Attribute[pElement->Element.nAttributes].Name, AttrName);
		strcpy(pElement->Element.Attribute[pElement->Element.nAttributes].Value, AttrValue);
		pElement->Element.nAttributes++;
		return 0;
	}
	return 2;
}

void * muXMLAddElement(struct muXMLTreeElement * pElement,
					   struct muXMLTreeElement * pNewElement)
{
	struct muXMLTreeElement * temp = pElement->SubElements;
	// Das letzte Unterelement suchen
	while(temp != NULL && temp->Next != NULL)
		temp = temp->Next;
	
	if(temp == NULL)
		pElement->SubElements = pNewElement;
	else
		temp->Next = pNewElement;
	pNewElement->Next = NULL; // Notwendig?

	return pNewElement + 1;
}

char * muXMLGetAttributeByName(struct muXMLTreeElement * pElement, char * Name)
{
	int i;
	for(i=0; i<pElement->Element.nAttributes;i++)
	{
		if(strcmp(pElement->Element.Attribute[i].Name, Name) == 0)
		{
			return pElement->Element.Attribute[i].Value;
		}
	}
	return NULL;
}



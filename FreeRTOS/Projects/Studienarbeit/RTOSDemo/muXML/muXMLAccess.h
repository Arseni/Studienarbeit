#ifndef MUXMLACCESS_H
#define MUXMLACCESS_H

#include "muXMLTree.h"
struct muXMLTreeElement* muXMLGetElementByName(struct muXMLTreeElement * pRoot,
											   char * Name);

void * muXMLAddElement(struct muXMLTreeElement * pElement,
					   struct muXMLTreeElement * pNewElement);

int muXMLUpdateAttribute(struct muXMLTreeElement * pElement,
						 char * AttrName,
						 char * AttrValue);

void * muXMLUpdateData(struct muXMLTreeElement * pElement,
					   char * newData);

void * muXMLCreateTree(void * Buffer, int BufferLength, char * rootName);
void * muXMLCreateElement(void * Buffer, char * name);

char * muXMLGetAttributeByName(struct muXMLTreeElement * pElement, char * Name);
#endif /* MUXMLACCESS_H */

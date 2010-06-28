#ifndef MUXML_H
#define MUXML_H

#include "muXMLTree.h"
struct muXMLTreeElement* muXMLGetElementByName(struct muXMLTreeElement * pRoot,
											   char * Name);

void * muXMLAddElement(struct muXMLTree * pTree,
					   struct muXMLTreeElement * pElement,
					   struct muXMLTreeElement * pNewElement);

int muXMLUpdateAttribute(struct muXMLTree * pTree,
						 struct muXMLTreeElement * pElement,
						 char * AttrName,
						 char * AttrValue);

void * muXMLUpdateData(struct muXMLTree * pTree,
					   struct muXMLTreeElement * pElement,
					   char * newData);

void * muXMLCreateTree(void * Buffer, int BufferLength, char * rootName);
void muXMLCreateElement(void * Buffer, char * name);

char * muXMLGetAttributeByName(struct muXMLTreeElement * pElement, char * Name);
#endif /* MUXML_H */

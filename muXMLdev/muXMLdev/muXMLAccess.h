#ifndef MUXML_H
#define MUXML_H

#include "muXMLTree.h"

int muXMLUpdateData(struct muXMLTree * pTree,
					struct muXMLTreeElement * pElement,
					char * newData);

#endif /* MUXML_H */

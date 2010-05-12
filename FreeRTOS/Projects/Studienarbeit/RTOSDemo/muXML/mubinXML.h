/*******************************************************************
 *
 * XML-Datenstruktur in binäre XML-Daten serialisieren/deserialisieren
 * -------------------------------------------------------------------
 *
 * Copyright (c) 2008 Krauth Technology.
 * Alle Rechte vorbehalten.
 *
 * $Id: mubinXML.h 156 2008-09-04 08:36:13Z knueppel $
 *
 *******************************************************************/

#ifndef __MUBINXML__
#define __MUBINXML__

#include "binXML.h"
#include "muXMLTree.h"

int mubinXMLEncode (unsigned char *Stream,
                    int StreamSize,
                    struct muXMLTree *p);

int mubinXMLDecode (char *XMLText,
                    int MaxSize,
                    struct binXMLTree *p);

#endif /* __MUBINXML__ */

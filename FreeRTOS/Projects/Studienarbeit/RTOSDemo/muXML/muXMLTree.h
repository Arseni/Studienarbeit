/*******************************************************************
 *
 * XML-Datenstruktur serialisieren/deserialisieren
 * -----------------------------------------------
 *
 * Copyright (c) 2008 Krauth Technology.
 * Alle Rechte vorbehalten.
 *
 * $Id: muXMLTree.h 147 2008-08-19 13:10:33Z knueppel $
 *
 *******************************************************************/

#ifndef __MUXMLTREE__
#define __MUXMLTREE__

#include "muXML.h"

#undef MUXMLTREE_HEADER

struct muXMLTreeData {
  char *Data;                           /* eigentliche Daten */
  int DataSize;                         /* aktuelle Größe der Daten */
};

struct muXMLTreeElement {
  struct muXMLTreeElement *Next;        /* Weitere Elemente auf
                                           gleicher Ebene */
  struct muXMLTreeElement *SubElements; /* Untergeordnete Elemente */
  struct muXMLTreeData Data;            /* Daten innerhalb dieses Elements */
  struct muXML_State Element;           /* Elementinformationen */
};

struct muXMLTree {
#ifdef MUXMLTREE_HEADER
  struct muXML_State Header;            /* Headerinformationen
                                           (Version, Zeichenkodierung, ...) */
#endif
  struct muXMLTreeElement Root;         /* Wurzelelement */
};

struct muXMLTree *muXMLTreeDecode (const char *XMLText,
                                   unsigned char *Data, int MaxSize,
                                   int SkipHeader,
                                   int *pUsage);

int muXMLTreeEncode (char *XMLText,
                     int MaxSize,
                     struct muXMLTree *pTree);

#endif /* __MUXMLTREE__ */

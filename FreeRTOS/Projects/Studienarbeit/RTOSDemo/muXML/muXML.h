/*******************************************************************
 *
 * Auswertung und Behandlung von XML-Daten (muXML = Micro Unit XML)
 * ----------------------------------------------------------------
 *
 * Copyright (c) 2008 Krauth Technology.
 * Alle Rechte vorbehalten.
 *
 * $Id: muXML.h 147 2008-08-19 13:10:33Z knueppel $
 *
 *******************************************************************/

#ifndef __MUXML__
#define __MUXML__

/*
 * Scan-Statuscodes
 */
#define muXML_ERROR             (-1)
#define muXML_IN_PROGRESS       (-2)
#define muXML_FINISHED          (-3)
#define muXML_ENDTAG            (-4)
#define muXML_IS_CHAR(n)        ((n) >= 0)

/*
 * Größenbeschränkungen
 */
#define muXML_MAX_NAME          16      /* max. Länge eines muXML-Tag-Namens */
#define muXML_MAX_ATTRIBUTE     16      /* max. Länge eines Attributnamens */
#define muXML_MAX_VALUE         24      /* max. Länge eines Attributwertes */
#define muXML_MAX_ENTITY        4       /* max. Länge einer Entity */
#define muXML_MAX_ATTRIBUTE_CNT 8       /* max. Anzahl an Attributen */
#define muXML_MAX_EXTID         8       /* max. Länge ExternalID */
#define muXML_MAX_ID            64      /* max. Länge System-/Public-
                                           identifier in DOCTYPE-Deklaration */

enum muXML_ScanState {
  muXML_WaitingForTag,
  muXML_InTagName,
  muXML_WaitingForAttribute,
  muXML_InAttributeName,
  muXML_WaitingForAttributeValue,
  muXML_InAttributeValue,
  muXML_WaitingForTagEnd,
  muXML_TagCompleted,
  muXML_InEntity1,
  muXML_InEntity2,
  muXML_InEntityName,
  muXML_InEntityDecimal,
  muXML_InEntityHex,
  muXML_InSpecialStart,
  muXML_InCommentStart,
  muXML_InComment,
  muXML_InCommentEnd,
  muXML_InCommentEnd2,
  muXML_InDocType,
  muXML_InDocTypeName1,
  muXML_InDocTypeName2,
  muXML_InDocTypeExtID1,
  muXML_InDocTypeExtID2,
  muXML_InDocTypePubSysID1,
  muXML_InDocTypePubSysID2,
  muXML_InEndTagName,
  muXML_InHexTag,
  muXML_InHexEndTag,
  muXML_EndTagCompleted,
  muXML_ErrorState
};

struct muXML_Attribute {
  char Name[muXML_MAX_ATTRIBUTE];         /* Attributname */
  char Value[muXML_MAX_VALUE];            /* Attributwert */
};

struct muXML_State {
  enum muXML_ScanState State;           /* Aktueller Auswertezustand */
  enum muXML_ScanState PreCommentState; /* Auswertezustand vor Kommentar */
  char Name[muXML_MAX_NAME];            /* Tag-Name */
  unsigned char nAttributes;            /* Anzahl an Attributen */
  unsigned char CharIndex;              /* Index für nächstes Zeichen */
  unsigned char TagIsFinished;          /* Kennzeichnung für "/>" */
  unsigned char Value;                  /* Zwischenwert für Zeichen */
  char ValueDelimiter;                  /* Begrenzungszeichen für
                                           Attributwerte ("'" oder '"') */
  char _Reserved[3];
  char EntityName[muXML_MAX_ENTITY];    /* Hilfsspeicher für Entitynamen */
  struct muXML_Attribute Attribute[muXML_MAX_ATTRIBUTE_CNT]; /* Attribute */
};

struct muXML_DocType {
  char RootElement[muXML_MAX_NAME];     /* Wurzelelementname */
  char ExternalID[muXML_MAX_EXTID];     /* "SYSTEM" oder "PUBLIC" */
  char SystemID[muXML_MAX_ID];          /* Systemidentifier */
  char PublicID[muXML_MAX_ID];          /* Publicidentifier */
};

void muXML_SetDocType (struct muXML_DocType *p);

void muXML_InitState (struct muXML_State *p);

void muXML_SetEndState (struct muXML_State *p);

int muXML_ScanTag (char c, struct muXML_State *p);

int muXML_ScanHeader (char c, struct muXML_State *p);

int muXML_ScanData (char c, struct muXML_State *p);

int muXML_ScanDataBlock (char c, struct muXML_State *p,
                         char *Data, int *pSize, int MaxSize);

int muXML_ScanEndTag (char c, struct muXML_State *p);

const char *muXML_GetAttributeValue (struct muXML_State *p,
                                     const char *Attribute,
                                     const char *Default);

int muXML_GetHeader (int (*GetChar) (void), struct muXML_State *p);

int muXML_GetTag (int (*GetChar) (void), struct muXML_State *p);

int muXML_GetEndTag (int (*GetChar) (void), struct muXML_State *p);

int muXML_GetData (int (*GetChar) (void), struct muXML_State *p,
                   char *Data, int *pSize, int MaxSize);

#endif /* __MUXML__ */

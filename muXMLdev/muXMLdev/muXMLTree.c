/*******************************************************************
 *
 * XML-Datenstruktur serialisieren/deserialisieren
 * -----------------------------------------------
 *
 * Copyright (c) 2008 Krauth Technology.
 * Alle Rechte vorbehalten.
 *
 * $Id: muXMLTree.c 147 2008-08-19 13:10:33Z knueppel $
 *
 *******************************************************************/

#include "muXMLTree.h"
#include <stdlib.h>
#include <string.h>

#define ALLOCATE(obj,type,pool,poolend,err)        \
  do {                                             \
    (obj) = (type *) (pool);                       \
    (pool) += sizeof (type);                       \
    (err) = ((pool) > (poolend));                  \
    if (!(err)) memset ((obj), 0, sizeof (type));  \
  } while (0)

#define DEALLOCATE(obj,pool) \
  (pool) = (unsigned char *) (obj)

/* Nur Speicher für die benötigten Attribute belegen */
#define SHRINKELEMENT(p,pool) \
  (pool) = (unsigned char *) & ((p)->Element.Attribute[(p)->Element.nAttributes])

static int ReadSubElements (struct muXMLTreeElement **ppElement,
                            const char **pXMLText,
                            unsigned char **pData, unsigned char *DataEnd)
/*
 * Unterelemente einer kompletten XML-Datenstruktur
 * nach "*ppElement" lesen (deserialisieren). Dabei
 * wird benötigter Speicher ab "*pData" belegt, wobei
 * Speicher bis maximal "DataEnd" zur Verfügung steht.
 * Der XML-Text wird von "*pXMLText" gelesen.
 * Rückgabewert: 1 im Fehlerfall; 0 sonst.
 */
{
  int c, rc, Error;
  const char *PrelimXMLText;
  struct muXMLTreeElement *p, *First, *Previous;

  First = NULL;
  Previous = NULL;
  while (1) {
    ALLOCATE (p, struct muXMLTreeElement, *pData, DataEnd, Error);
    if (Error) return 1;
    muXML_InitState (& p->Element);
    do {
      c = *(*pXMLText)++;
      if (c == '\0') return 1;
      rc = muXML_ScanTag (c, & p->Element);
      if (rc == muXML_ERROR) return 1;
    } while (rc == muXML_IN_PROGRESS);

    if (rc == muXML_ENDTAG) { DEALLOCATE (p, *pData); break; }

    SHRINKELEMENT (p, *pData);

    if (First) Previous->Next = p;
    else First = p;
    Previous = p;

    if (!p->Element.TagIsFinished) {
      PrelimXMLText = *pXMLText;
      do {
        c = *PrelimXMLText++;
        if (c == '\0') return 1;
        rc = muXML_ScanData (c, & p->Element);
        if (rc == muXML_ERROR) break;
        if (muXML_IS_CHAR (rc)) {
          if (p->Data.DataSize == 0) p->Data.Data = (char *) *pData;
          *(*pData)++ = rc;
          if (*pData >= DataEnd) return 1;
          p->Data.DataSize++;
        }
      } while (rc != muXML_FINISHED);

      if (rc == muXML_ERROR) {
        if (ReadSubElements (& p->SubElements,
                             pXMLText, pData, DataEnd)) return 1;

        /* End-Tag einlesen */
        muXML_SetEndState (& p->Element);
        do {
          c = *(*pXMLText)++;
          if (c == '\0') return 1;
          rc = muXML_ScanEndTag (c, & p->Element);
          if (rc == muXML_ERROR) return 1;
        } while (rc != muXML_FINISHED);
      }
      else {
        *pXMLText = PrelimXMLText;
        if (p->Data.DataSize) {
          /* Nicht in die Datenlänge eingehendes Nullbyte an das Ende anfügen */
          *(*pData)++ = '\0';
          if (*pData > DataEnd) return 1;
          /* 4-Byte-Alignment für nächste Datenstruktur durchführen */
          *pData = (unsigned char *) (((unsigned long) (*pData) + 3) & ~3);
        }
      }
    }
  }

  *ppElement = First;
  return 0;
}

struct muXMLTree * muXMLTreeDecode (const char *XMLText,
                                   unsigned char *Data, int MaxSize,
                                   int SkipHeader,
                                   int *pUsage)
/*
 * Einlesen (Deserialisierung) einer kompletten XML-
 * Datenstruktur von "XMLText" nach "Data", wobei dort
 * maximal "MaxSize" Bytes zur Verfügung stehen.
 * Falls "SkipHeader" ungleich Null ist, wird kein
 * XML-Header erwartet und eingelesen.
 * Ist "pUsage" nicht der Nullzeiger, wird in "*pUsage"
 * die Gesamtgröße des belegten Speichers ab "Data" abgelegt.
 * Rückgabewert: Zeiger auf die XML-Datenstruktur oder
 *               Nullzeiger im Fehlerfall.
 */
{
  int c, rc, Error;
  struct muXMLTree *p;
  unsigned char *DataEnd;

  DataEnd = Data + MaxSize;

  ALLOCATE (p, struct muXMLTree, Data, DataEnd, Error);
  if (Error) return NULL;

#ifdef MUXMLTREE_HEADER
  if (!SkipHeader) {
    /* Header einlesen */
    muXML_InitState (& p->Header);
    do {
      c = *XMLText++;
      if (c == '\0') goto Error;
      rc = muXML_ScanHeader (c, & p->Header);
      if (rc == muXML_ERROR) goto Error;
    } while (rc != muXML_FINISHED);
  }
#endif

  /* Wurzelelement einlesen */
  muXML_InitState (& p->Root.Element);
  do {
    c = *XMLText++;
    if (c == '\0') goto Error;
    rc = muXML_ScanTag (c, & p->Root.Element);
    if (rc == muXML_ERROR) goto Error;
  } while (rc != muXML_FINISHED);

  SHRINKELEMENT (& p->Root, Data);

  /* Unterelemente einlesen */
  if (ReadSubElements (& p->Root.SubElements,
                       & XMLText, & Data, DataEnd)) goto Error;

  /* End-Tag einlesen */
  muXML_SetEndState (& p->Root.Element);
  do {
    c = *XMLText++;
    if (c == '\0') goto Error;
    rc = muXML_ScanEndTag (c, & p->Root.Element);
    if (rc == muXML_ERROR) goto Error;
  } while (rc != muXML_FINISHED);

  #ifdef FILE_INFO
  p->StorageInfo.SpaceTotal = MaxSize;
  p->StorageInfo.SpaceInUse = MaxSize - (DataEnd - Data);

  if (pUsage) *pUsage =  p->StorageInfo.SpaceInUse;
  #else
  if (pUsage) *pUsage =  MaxSize - (DataEnd - Data);
  #endif
  return p;

  Error:
  return NULL;
}

static int CopyString (char **pXMLText, char *TextEnd, const char *s)
/*
 * Zeichenkette "s" nach "*pXMLText" kopieren, wobei die
 * Grenze "TextEnd" nicht überschritten werden darf.
 * Rückgabewert: 1 im Fehlerfall; 0 sonst.
 */
{
  int Len;
  char *p;

  Len = strlen (s);
  p = *pXMLText;
  *pXMLText += Len;
  if (*pXMLText > TextEnd) return 1;
  memcpy (p, s, Len);
  return 0;
}

static int CopyData (char **pXMLText, char *TextEnd,
                     const char *s, int Size)
/*
 * Daten "s" (mit insgesamt "Size" Bytes nach "*pXMLText"
 * kopieren, wobei die Grenze "TextEnd" nicht überschritten
 * werden darf.
 * Rückgabewert: 1 im Fehlerfall; 0 sonst.
 */
{
  int i, InHexMode;
  char c, Digit;

  InHexMode = 0;
  for (i = 0; i < Size; i++) {
    c = *s++;
    if ((c & 0x7f) < 0x20) {
      if (!InHexMode) {
        if (CopyString (pXMLText, TextEnd, "<hex>")) return 1;
        InHexMode = 1;
      }
      if (*pXMLText + 2 > TextEnd) return 1;
      Digit = (c >> 4);
      Digit += ((Digit > 9) ? 'A' : '0');
      *(*pXMLText)++ = Digit;
      Digit = (c & 0x0f);
      Digit += ((Digit > 9) ? 'A' : '0');
      *(*pXMLText)++ = Digit;
    }
    else {
      if (InHexMode) {
        if (CopyString (pXMLText, TextEnd, "</hex>")) return 1;
        InHexMode = 0;
      }
      if (*pXMLText >= TextEnd) return 1;
      *(*pXMLText)++ = c;
    }
  }

  return InHexMode && CopyString (pXMLText, TextEnd, "</hex>");
}

static int ElementEncode (struct muXMLTreeElement *p,
                          char **pXMLText, char *TextEnd)
/*
 * Serialisierung des XML-Elementes "p" und seiner
 * Unter- und Nachfolgeelemente nach "*pXMLText", wobei
 * dort bis hin zu "TextEnd" Speicher zur Verfügung
 * steht.
 * Rückgabewert: 1 im Fehlerfall; 0 sonst.
 */
{
  int i;

  for (; p; p = p->Next) {
    if (CopyString (pXMLText, TextEnd, "<") ||
        CopyString (pXMLText, TextEnd, p->Element.Name)) return 1;
    for (i = 0; i < p->Element.nAttributes; i++)
      if (CopyString (pXMLText, TextEnd, " ") ||
          CopyString (pXMLText, TextEnd, p->Element.Attribute[i].Name) ||
          CopyString (pXMLText, TextEnd, "=\"") ||
          CopyString (pXMLText, TextEnd, p->Element.Attribute[i].Value) ||
          CopyString (pXMLText, TextEnd, "\"")) return 1;
    if (!p->Data.DataSize && !p->SubElements) {
      if (CopyString (pXMLText, TextEnd, "/>")) return 1;
    }
    else {
      if (CopyString (pXMLText, TextEnd, ">")) return 1;
      if (p->Data.DataSize &&
          CopyData (pXMLText, TextEnd,
                    p->Data.Data, p->Data.DataSize)) return 1;
      if (ElementEncode (p->SubElements, pXMLText, TextEnd)) return 1;
      if (CopyString (pXMLText, TextEnd, "</") ||
          CopyString (pXMLText, TextEnd, p->Element.Name) ||
          CopyString (pXMLText, TextEnd, ">")) return 1;
    }
  }
  return 0;
}

int muXMLTreeEncode (char *XMLText,
                     int MaxSize,
                     struct muXMLTree *pTree)
/*
 * Serialisierung einer XML-Struktur "pTree" in
 * einen transportablen XML-Datenstrom ab "XMLText",
 * wobei dort maximal "MaxSize" Bytes zur Verfügung
 * stehen.
 * Rückgabewert: 1 im Fehlerfall; 0 sonst.
 */
{
  char *TextEnd;

  TextEnd = XMLText + MaxSize;

  if (!ElementEncode (& pTree->Root, & XMLText, TextEnd)) {
    if (XMLText < TextEnd) *XMLText = '\0';
    return 0;
  }
  return 1;
}

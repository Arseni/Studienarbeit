/*******************************************************************
 *
 * XML-Datenstruktur in binäre XML-Daten serialisieren/deserialisieren
 * -------------------------------------------------------------------
 *
 * Copyright (c) 2008 Krauth Technology.
 * Alle Rechte vorbehalten.
 *
 * $Id: mubinXML.c 156 2008-09-04 08:36:13Z knueppel $
 *
 *******************************************************************/

#include <string.h>
#include "mubinXML.h"

static int Encode (unsigned char *Stream,
                   unsigned char *StreamEnd,
                   struct muXMLTreeElement *p,
                   int *pError)
/*
 * XML-Baum "p" in binäre XML-Daten ab "Stream", wobei dort
 * bis "StreamEnd" Platz zur Verfügung steht, kodieren.
 * Falls ein Fehler auftritt, wird "*pError" gesetzt.
 * Rückgabewert: Länge der binären XML-Daten.
 */
{
#define COPYSTRING(s,Len)                       \
  do {                                          \
    if (Stream + Len > StreamEnd) goto Error;   \
    memcpy (Stream, (s), (Len));                \
    Stream += (Len);                            \
  } while (0)
#define WRITE16(s,n)                            \
  do {                                          \
    *s++ = ((n) >> 8);                          \
    *s++ = (n);                                 \
  } while (0)

  int i, Len;
  unsigned char *StreamStart, *SubTagLen, *AttrLen;

  StreamStart = Stream;

  for (; p; p = p->Next) {
    if (Stream + 3 + 2 * p->Element.nAttributes > StreamEnd) goto Error;

    /* 2 Bytes für Länge der Unter-Tags können erst nach Abarbeitung
       der Unter-Tags gesetzt werden. */
    SubTagLen = Stream;

    /* Attributdaten schreiben */
    Stream[2] = p->Element.nAttributes;
    Stream += 3;

    AttrLen = Stream;
    Stream += 2 * p->Element.nAttributes;
    for (i = 0; i < p->Element.nAttributes; i++) {
      Len = strlen (p->Element.Attribute[i].Name);
      *AttrLen++ = Len;
      COPYSTRING (p->Element.Attribute[i].Name, Len);
      Len = strlen (p->Element.Attribute[i].Value);
      *AttrLen++ = Len;
      COPYSTRING (p->Element.Attribute[i].Value, Len);
    }

    /* Tag-Namen schreiben */
    if (Stream >= StreamEnd) goto Error;
    Len = strlen (p->Element.Name);
    *Stream++ = Len;
    COPYSTRING (p->Element.Name, Len);

    /* Elementdaten schreiben */
    if (Stream + 2 > StreamEnd) goto Error;
    WRITE16 (Stream, p->Data.DataSize);
    COPYSTRING (p->Data.Data, p->Data.DataSize);

    /* Unter-Tags schreiben */
    Len = Encode (Stream, StreamEnd, p->SubElements, pError);
    if (*pError) return 0;
    Stream += Len;
    WRITE16 (SubTagLen, Len);
  }

  return Stream - StreamStart;

  Error:
  *pError = 1;
  return 0;

#undef WRITE16
#undef COPYSTRING
}

int mubinXMLEncode (unsigned char *Stream,
                    int StreamSize,
                    struct muXMLTree *p)
/*
 * XML-Baum "p" in binäre XML-Daten ab "Stream", wobei dort
 * maximal "StreamSize" Bytes zur Verfügung stehen, kodieren.
 * Rückgabewert: Länge der binären XML-Daten oder
 *               0 im Fehlerfall.
 */
{
  int Len, Error;

  Error = 0;
  Len = Encode (Stream, Stream + StreamSize, & p->Root, & Error);
  return Error ? 0 : Len;
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
  unsigned char c, Digit;

  InHexMode = 0;
  for (i = 0; i < Size; i++) {
    c = *s++;
    if (((c & 0x7f) < 0x20) || (c == '<') || (c == '>')) {
      if (!InHexMode) {
        if (CopyString (pXMLText, TextEnd, "<hex>")) return 1;
        InHexMode = 1;
      }
      if (*pXMLText + 2 > TextEnd) return 1;
      Digit = (c >> 4);
      Digit += ((Digit > 9) ? ('A' - 10) : '0');
      *(*pXMLText)++ = Digit;
      Digit = (c & 0x0f);
      Digit += ((Digit > 9) ? ('A' - 10) : '0');
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

static int ElementEncode (struct binXMLTree *p,
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
        CopyString (pXMLText, TextEnd, p->Name)) return 1;
    for (i = 0; i < p->nAttributes; i++)
      if (CopyString (pXMLText, TextEnd, " ") ||
          CopyString (pXMLText, TextEnd, p->Attribute[i].Name) ||
          CopyString (pXMLText, TextEnd, "=\"") ||
          CopyString (pXMLText, TextEnd, p->Attribute[i].Value) ||
          CopyString (pXMLText, TextEnd, "\"")) return 1;
    if (!p->DataSize && !p->SubElements) {
      if (CopyString (pXMLText, TextEnd, "/>")) return 1;
    }
    else {
      if (CopyString (pXMLText, TextEnd, ">")) return 1;
      if (p->DataSize &&
          CopyData (pXMLText, TextEnd,
                    p->Data, p->DataSize)) return 1;
      if (ElementEncode (p->SubElements, pXMLText, TextEnd)) return 1;
      if (CopyString (pXMLText, TextEnd, "</") ||
          CopyString (pXMLText, TextEnd, p->Name) ||
          CopyString (pXMLText, TextEnd, ">")) return 1;
    }
  }
  return 0;
}

int mubinXMLDecode (char *XMLText,
                    int MaxSize,
                    struct binXMLTree *p)
/*
 * Serialisierung einer XML-Struktur "p" in
 * einen transportablen XML-Datenstrom ab "XMLText",
 * wobei dort maximal "MaxSize" Bytes zur Verfügung
 * stehen.
 * Rückgabewert: 1 im Fehlerfall; 0 sonst.
 */
{
  char *TextEnd;

  TextEnd = XMLText + MaxSize;

  if (!ElementEncode (p, & XMLText, TextEnd)) {
    if (XMLText < TextEnd) *XMLText = '\0';
    return 0;
  }
  return 1;
}

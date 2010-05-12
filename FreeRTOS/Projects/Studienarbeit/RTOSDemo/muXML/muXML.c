/*******************************************************************
 *
 * Auswertung und Behandlung von XML-Daten (muXML = Micro Unit XML)
 * ----------------------------------------------------------------
 *
 * Copyright (c) 2008 Krauth Technology.
 * Alle Rechte vorbehalten.
 *
 * $Id: muXML.c 147 2008-08-19 13:10:33Z knueppel $
 *
 *******************************************************************/

#include "muXML.h"
#include <string.h>

static struct muXML_DocType *DocType = NULL;

static int IsNameFirstChar (char c)
/*
 * Prüfung, ob "c" ein zulässiges Zeichen für das
 * erste Zeichen eines Bezeichners ist.
 * Rückgabewert: 1, falls ja; 0 sonst.
 */
{
  return (((c >= 'A') && (c <= 'Z')) ||
          ((c >= 'a') && (c <= 'z')) ||
          (c == ':') || (c == '_'));
}

static int IsNameChar (char c)
/*
 * Prüfung, ob "c" ein zulässiges Zeichen für
 * weitere Zeichen eines Bezeichners ist.
 * Rückgabewert: 1, falls ja; 0 sonst.
 */
{
  return (((c >= 'A') && (c <= 'Z')) ||
          ((c >= 'a') && (c <= 'z')) ||
          ((c >= '0') && (c <= '9')) ||
          (c == ':') || (c == '_') || (c == '-') || (c == '.'));
}

static int IsWhiteSpaceChar (char c)
/*
 * Prüfung, ob "c" ein White-Space-Zeichen ist.
 * Rückgabewert: 1, falls ja; 0 sonst.
 */
{
  return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

static int IsAttributeValueChar (char c)
/*
 * Prüfung, ob "c" ein zulässiges Zeichen innerhalb
 * eines Attributwertes ist.
 * Rückgabewert: 1, falls ja; 0 sonst.
 */
{
  return (c >= ' ') && (c != '<');
}

static int IsDecimalChar (char c)
/*
 * Prüfung, ob "c" ein gültiges Zeichen
 * innerhalb einer Dezimalzahl ist.
 * Rückgabewert: 1, falls ja; 0 sonst.
 */
{
  return (c >= '0') && (c <= '9');
}

static int IsHexChar (char c)
/*
 * Prüfung, ob "c" ein gültiges Zeichen
 * innerhalb einer Hexadezimalzahl ist.
 * Rückgabewert: 1, falls ja; 0 sonst.
 */
{
  return (((c >= '0') && (c <= '9')) ||
          ((c >= 'A') && (c <= 'F')) ||
          ((c >= 'a') && (c <= 'f')));
}

static int GetCharValue (char c)
/*
 * Zahlenwert des Zeichens "c" zurückgeben.
 * Annahme: "c" ist innerhalb [0-9a-fA-F] und "c" ist ein ASCII-Zeichen.
 */
{
  return (c <= '9') ? (c & 0x0f) : ((c & 0x0f) + 9);
}

void muXML_SetDocType (struct muXML_DocType *p)
/*
 * Daten für DOCTYPE-Deklaration setzen.
 */
{
  DocType = p;
  if (DocType) memset (DocType, 0, sizeof (struct muXML_DocType));
}

void muXML_InitState (struct muXML_State *p)
/*
 * Initialisierung der Auswertung eines Tags.
 */
{
  memset (p, 0, sizeof (struct muXML_State));
  p->State = muXML_WaitingForTag;
}

void muXML_SetEndState (struct muXML_State *p)
/*
 * Vorbereitung auf Auswertung des zu "p" gehörenden End-Tags.
 */
{
  p->CharIndex = 0;
  p->State = muXML_InEndTagName;
}

static void StartSpecial (struct muXML_State *p, enum muXML_ScanState State)
/*
 * Auswertung eines XML-Kommentars starten/kennzeichnen.
 * Nach Auswertung des Kommentars wird der Scanzustand
 * auf "State" gesetzt.
 */
{
  p->PreCommentState = State;
  p->State = muXML_InSpecialStart;
}

static int ScanCommentAndSpecials (char c, struct muXML_State *p)
/*
 * Auswertung eines XML-Kommentars, wobei als
 * nächstes Zeichen im XML-Stream "c" geliefert wird.
 * Informationen über die Auswertung werden in "p" abgelegt.
 * Rückgabewert: muXML_ERROR im Fehlerfall;
 *               muXML_IN_PROGRESS sonst.
 */ 
{
  static const char DocTypeTag[] = "DOCTYPE";

  switch (p->State) {
    case muXML_InSpecialStart:
      if (c == '-') p->State = muXML_InCommentStart;
      else if (c == DocTypeTag[0]) {
        if (DocType && DocType->RootElement[0]) goto Error;
        p->CharIndex = 1;
        p->State = muXML_InDocType;
      }
      else goto Error;
      break;
    case muXML_InCommentStart:
      if (c == '-') p->State = muXML_InComment;
      else goto Error;
      break;
    case muXML_InComment:
      if (c == '-') p->State = muXML_InCommentEnd;
      break;
    case muXML_InCommentEnd:
      p->State = ((c == '-') ? muXML_InCommentEnd2 : muXML_InComment);
      break;
    case muXML_InCommentEnd2:
      if (c == '>') p->State = p->PreCommentState;
      else p->State = muXML_InComment; /* "goto Error" für SGML-Kompat. */
      break;
    case muXML_InDocType:
      if (DocTypeTag[p->CharIndex] == '\0') {
        if (IsWhiteSpaceChar (c)) p->State = muXML_InDocTypeName1;
        else goto Error;
      }
      else if (c != DocTypeTag[p->CharIndex++]) goto Error;
      break;
    case muXML_InDocTypeName1:
      if (IsNameFirstChar (c)) {
        if (DocType) { DocType->RootElement[0] = c; p->CharIndex = 1; }
        p->State = muXML_InDocTypeName2;
      }
      else if (!IsWhiteSpaceChar (c)) goto Error;
      break;
    case muXML_InDocTypeName2:
      if (IsNameChar (c)) {
        if (DocType && (p->CharIndex < muXML_MAX_NAME - 1))
          DocType->RootElement[p->CharIndex++] = c;
      }
      else if (IsWhiteSpaceChar (c)) p->State = muXML_InDocTypeExtID1;
      else goto Error;
      break;
    case muXML_InDocTypeExtID1:
      if (IsNameFirstChar (c)) {
        if (DocType) { DocType->ExternalID[0] = c; p->CharIndex = 1; }
        p->State = muXML_InDocTypeExtID2;
      }
      else if (!IsWhiteSpaceChar (c)) goto Error;
      break;
    case muXML_InDocTypeExtID2:
      if (IsNameChar (c)) {
        if (DocType && (p->CharIndex < muXML_MAX_EXTID - 1))
          DocType->ExternalID[p->CharIndex++] = c;
      }
      else if (IsWhiteSpaceChar (c)) {
        if (DocType) {
          if (!strcmp (DocType->ExternalID, "SYSTEM")) p->Value = 1;
          else if (!strcmp (DocType->ExternalID, "PUBLIC")) p->Value = 2;
          else goto Error;
        }
        else p->Value = 2;
        p->State = muXML_InDocTypePubSysID1;
      }
      else goto Error;
      break;
    case muXML_InDocTypePubSysID1:
      if ((c == '"') || (c == '\'')) {
        if (p->Value-- == 0) goto Error;
        p->ValueDelimiter = c;
        p->CharIndex = 0;
        p->State = muXML_InDocTypePubSysID2;
      }
      else if (c == '>') {
        if (DocType && p->Value) goto Error;
        p->CharIndex = 0;
        p->State = p->PreCommentState;
      }
      else if (!IsWhiteSpaceChar (c)) goto Error;
      break;
    case muXML_InDocTypePubSysID2:
      if (c == p->ValueDelimiter) p->State = muXML_InDocTypePubSysID1;
      else if (DocType && (p->CharIndex < muXML_MAX_ID - 1)) {
        if (p->Value) DocType->PublicID[p->CharIndex++] = c;
        else DocType->SystemID[p->CharIndex++] = c;
      }
      break;
    default:
      return muXML_ERROR;
  }
  return muXML_IN_PROGRESS;

  Error:
  p->State = muXML_ErrorState;
  return muXML_ERROR;
}

int muXML_ScanTag (char c, struct muXML_State *p)
/*
 * Auswertung eines Tags, wobei als
 * nächstes Zeichen im XML-Stream "c" geliefert wird.
 * Informationen über die Auswertung werden in "p" abgelegt.
 * Nicht implementiert sind: Kommentare und andere,
 * mit "<!" beginnende Konstrukte.
 * Rückgabewert: muXML_FINISHED, falls die Auswertung beendet ist;
 *               muXML_ERROR im Fehlerfall;
 *               muXML_ENDTAG, falls ein End-Tag erkannt wurde;
 *               muXML_IN_PROGRESS sonst.
 */
{
  switch (p->State) {
    case muXML_WaitingForTag:
      if (c == '<') { p->State = muXML_InTagName; p->CharIndex = 0; }
      break;
    case muXML_InTagName:
      if (p->CharIndex == 0) {
        if (c == '!') StartSpecial (p, muXML_WaitingForTag);
        else if (c == '/') {
          p->State = muXML_InEndTagName;
          return muXML_ENDTAG;
        }
        else if (IsNameFirstChar (c)) { p->Name[0] = c; p->CharIndex = 1; }
        else goto Error;
      }
      else {
        if (IsNameChar (c)) {
          if (p->CharIndex < muXML_MAX_NAME - 1) p->Name[p->CharIndex++] = c;
        }
        else {
          if (IsWhiteSpaceChar (c)) p->State = muXML_WaitingForAttribute;
          else if (c == '/') {
            p->TagIsFinished = 1;
            p->State = muXML_WaitingForTagEnd;
          }
          else if (c == '>') {
            p->State = muXML_TagCompleted;
            return muXML_FINISHED;
          }
          else goto Error;
        }
      }
      break;
    case muXML_WaitingForAttribute:
      if (IsNameFirstChar (c)) {
        if (p->nAttributes < muXML_MAX_ATTRIBUTE_CNT)
          p->Attribute[p->nAttributes].Name[0] = c;
        p->CharIndex = 1;
        p->State = muXML_InAttributeName;
      }
      else if (c == '/') {
        p->TagIsFinished = 1;
        p->State = muXML_WaitingForTagEnd;
      }
      else if (c == '>') {
        p->State = muXML_TagCompleted;
        return muXML_FINISHED;
      }
      else if (!IsWhiteSpaceChar (c)) goto Error;
      break;
    case muXML_InAttributeName:
      if (IsNameChar (c)) {
        if ((p->nAttributes < muXML_MAX_ATTRIBUTE_CNT) &&
            (p->CharIndex < muXML_MAX_ATTRIBUTE - 1))
          p->Attribute[p->nAttributes].Name[p->CharIndex++] = c;
      }
      else if (c == '=') p->State = muXML_WaitingForAttributeValue;
      else goto Error;
      break;
    case muXML_WaitingForAttributeValue:
      if ((c == '"') || (c == '\'')) {
        p->ValueDelimiter = c;
        p->CharIndex = 0;
        p->State = muXML_InAttributeValue;
      }
      else goto Error;
      break;
    case muXML_InAttributeValue:
      if (c == p->ValueDelimiter) {
        if (p->nAttributes < muXML_MAX_ATTRIBUTE_CNT) p->nAttributes++;
        p->State = muXML_WaitingForAttribute;
      }
      else if (IsAttributeValueChar (c)) {
        if ((p->nAttributes < muXML_MAX_ATTRIBUTE_CNT) &&
            (p->CharIndex < muXML_MAX_VALUE - 1))
          p->Attribute[p->nAttributes].Value[p->CharIndex++] = c;
      }
      else goto Error;
      break;
    case muXML_WaitingForTagEnd:
      if (c == '>') { p->State = muXML_TagCompleted; return muXML_FINISHED; }
      else goto Error;
    case muXML_InSpecialStart:
    case muXML_InCommentStart:
    case muXML_InComment:
    case muXML_InCommentEnd:
    case muXML_InCommentEnd2:
    case muXML_InDocType:
    case muXML_InDocTypeName1:
    case muXML_InDocTypeName2:
    case muXML_InDocTypeExtID1:
    case muXML_InDocTypeExtID2:
    case muXML_InDocTypePubSysID1:
    case muXML_InDocTypePubSysID2:
      return ScanCommentAndSpecials (c, p);
    case muXML_TagCompleted:
      return muXML_FINISHED;
    case muXML_InEndTagName:
      return muXML_ENDTAG;
    default:
      return muXML_ERROR;
  }
  return muXML_IN_PROGRESS;

  Error:
  p->State = muXML_ErrorState;
  return muXML_ERROR;
}

int muXML_ScanHeader (char c, struct muXML_State *p)
/*
 * Überspringen des Headers, wobei als nächstes
 * Zeichen des XML-Streams "c" geliefert wird.
 * Rückgabewert: muXML_FINISHED, falls die Auswertung beendet ist;
 *               muXML_ERROR im Fehlerfall;
 *               muXML_IN_PROGRESS sonst.
 */
{
  static const char HeaderTag[] = "?xml";

  switch (p->State) {
    case muXML_WaitingForTag:
      if (c == '<') { p->State = muXML_InTagName; p->CharIndex = 0; }
      break;
    case muXML_InTagName:
      if (HeaderTag[p->CharIndex] == '\0') {
        if (IsWhiteSpaceChar (c)) p->State = muXML_WaitingForAttribute;
        else goto Error;
      }
      else if (c != HeaderTag[p->CharIndex++]) goto Error;
      break;
    case muXML_WaitingForAttribute:
      if ((c == '/') || (c == '>')) goto Error;
      else if (c == '?') p->State = muXML_WaitingForTagEnd;
      else return muXML_ScanTag (c, p);
      break;
    default:
      return muXML_ScanTag (c, p);
  }
  return muXML_IN_PROGRESS;

  Error:
  p->State = muXML_ErrorState;
  return muXML_ERROR;
}

static int SetValueFromEntityName (const char *Name, unsigned char *pValue)
/*
 * Zeichencode "*pValue" für Entity "Name" setzen.
 * Rückgabewert: 1 im Fehlerfall; 0 sonst.
 */
{
  struct muXMLentity {
    const char *Name;
    unsigned char Value;
  };
  int i;
  const struct muXMLentity *p;
  static const struct muXMLentity Entities[] = {
    { "lt", '<' }, { "gt", '>' }, { "amp", '&' }
  };

  p = Entities;
  for (i = 0; i < sizeof (Entities) / sizeof (Entities[0]); i++, p++)
    if (!strcmp (Name, p->Name)) { *pValue = p->Value; return 0; }
  return 1;
}

int muXML_ScanData (char c, struct muXML_State *p)
/*
 * Auswertung von Zeichen innerhalb eines Elements "p",
 * wobei als nächstes Zeichen des XML-Streams "c"
 * geliefert wird.
 * Informationen über den Status der Auswertung werden
 * in "p" abgelegt.
 * Annahme: Innerhalb des Elements sind nur Zeichen
 * ("PCDATA") oder Elemente vom Typ "<hex>" zulässig.
 * Rückgabewert: >= 0: Datenzeichen;
 *               muXML_FINISHED, falls das Element beendet ist;
 *               muXML_ERROR im Fehlerfall;
 *               muXML_IN_PROGRESS bei offenen Zuständen.
 */
{
  static const char HexEndTag[] = "/hex>";

  switch (p->State) {
    case muXML_TagCompleted:
      p->State = muXML_WaitingForTag;
      /* fall through */
    case muXML_WaitingForTag:
      if (c == '<') { p->CharIndex = 0; p->State = muXML_InTagName; }
      else if (c == '&') p->State = muXML_InEntity1;
      else return c & 0xff;
      break;
    case muXML_InTagName:
      if (c == '!') StartSpecial (p, muXML_WaitingForTag);
      else if (c == '/') { p->CharIndex = 0; p->State = muXML_InEndTagName; }
      else {
        if (c != HexEndTag[1 + p->CharIndex++]) goto Error;
        if (c == '>') { p->CharIndex = 0; p->State = muXML_InHexTag; }
      }
      break;
    case muXML_InEndTagName:
      if (p->Name[p->CharIndex] == '\0') {
        if (c == '>') {
          p->TagIsFinished = 1;
          p->State = muXML_EndTagCompleted;
          return muXML_FINISHED;
        }
        else goto Error;
      }
      if (c != p->Name[p->CharIndex++]) goto Error;
      break;
    case muXML_InHexTag:
      if (c == '<') {
        if (p->CharIndex) goto Error;
        p->State = muXML_InHexEndTag;
      }
      else if (IsHexChar (c)) {
        if (p->CharIndex == 0) p->Value = 0;
        p->Value = 16 * p->Value + GetCharValue (c);
        p->CharIndex = (p->CharIndex + 1) & 1;
        if (p->CharIndex == 0) return p->Value;
      }
      else if (!IsWhiteSpaceChar (c)) goto Error;
      break;
    case muXML_InHexEndTag:
      if ((c == '!') && (p->CharIndex == 0)) StartSpecial (p, muXML_InHexTag);
      else if (c != HexEndTag[p->CharIndex++]) goto Error;
      if (c == '>') p->State = muXML_TagCompleted;
      break;
    case muXML_InEntity1:
      if (c == '#') p->State = muXML_InEntity2;
      else if (IsNameFirstChar (c)) {
        p->CharIndex = 1;
        p->EntityName[0] = c;
        p->State = muXML_InEntityName;
      }
      else goto Error;
      break;
    case muXML_InEntity2:
      if (c == 'x') { p->Value = 0; p->State = muXML_InEntityHex; }
      else if (IsDecimalChar (c)) {
        p->Value = GetCharValue (c);
        p->State = muXML_InEntityDecimal;
      }
      else goto Error;
      break;
    case muXML_InEntityName:
      if (IsNameChar (c)) {
        if (p->CharIndex < muXML_MAX_ENTITY - 1)
          p->EntityName[p->CharIndex++] = c;
      }
      else if (c == ';') {
        p->EntityName [p->CharIndex] = '\0';
        if (SetValueFromEntityName (p->EntityName, & p->Value)) goto Error;
        p->State = muXML_TagCompleted;
        return p->Value;
      }
      break;
    case muXML_InEntityDecimal:
      if (IsDecimalChar (c)) p->Value = 10 * p->Value + GetCharValue (c);
      else if (c == ';') { p->State = muXML_TagCompleted; return p->Value; }
      break;
    case muXML_InEntityHex:
      if (IsHexChar (c)) p->Value = 16 * p->Value + GetCharValue (c);
      else if (c == ';') { p->State = muXML_TagCompleted; return p->Value; }
      break;
    case muXML_InSpecialStart:
    case muXML_InCommentStart:
    case muXML_InComment:
    case muXML_InCommentEnd:
    case muXML_InCommentEnd2:
    case muXML_InDocType:
    case muXML_InDocTypeName1:
    case muXML_InDocTypeName2:
    case muXML_InDocTypeExtID1:
    case muXML_InDocTypeExtID2:
    case muXML_InDocTypePubSysID1:
    case muXML_InDocTypePubSysID2:
      return ScanCommentAndSpecials (c, p);
    case muXML_EndTagCompleted:
      return muXML_FINISHED;
    default:
      return muXML_ERROR;
  }
  return muXML_IN_PROGRESS;

  Error:
  p->State = muXML_ErrorState;
  return muXML_ERROR;
}

int muXML_ScanDataBlock (char c, struct muXML_State *p,
                         char *Data, int *pSize, int MaxSize)
/*
 * Lesen von Zeichen innerhalb des Elementes "p" nach "Data",
 * wobei die Länge der gelesenen Daten durch "*pSize"
 * und die maximal zur Verfügung stehende Länge durch
 * "MaxSize" angegeben und als nächstes Zeichen
 * des XML-Streams "c" geliefert wird.
 * Die Zeichen werden in "Data" als nullterminierte
 * Zeichenkette abgelegt.
 * Rückgabewert: muXML_FINISHED, falls das Element beendet ist;
 *               muXML_ERROR im Fehlerfall;
 *               muXML_IN_PROGRESS bei offenen Zuständen.
 */
{
  int rc, Len;

  rc = muXML_ScanData (c, p);
  if (muXML_IS_CHAR (rc)) {
    Len = *pSize;
    if (Len >= MaxSize - 1) return muXML_ERROR;
    Data += Len;
    (*pSize)++;
    *Data++ = rc;
    *Data = '\0';

    rc = muXML_IN_PROGRESS;    
  }

  return rc;
}

int muXML_ScanEndTag (char c, struct muXML_State *p)
/*
 * Überspringen von Elementen und Daten bis zum
 * zu "p" gehörigen End-Tag, wobei als nächstes
 * Zeichen des XML-Streams "c" geliefert wird.
 * Annahme: Es sind keine Elemente außer "<hex>" zulässig.
 * Rückgabewert: muXML_FINISHED, falls die Auswertung beendet ist;
 *               muXML_ERROR im Fehlerfall;
 *               muXML_IN_PROGRESS sonst.
 */
{
  int rc;

  if (p->TagIsFinished) return muXML_FINISHED;
  rc = muXML_ScanData (c, p);
  return muXML_IS_CHAR (rc) ? muXML_IN_PROGRESS : rc;
}

const char *muXML_GetAttributeValue (struct muXML_State *p,
                                     const char *Attribute,
                                     const char *Default)
/*
 * Den zum Attribut "Attribute" gehörenden Wert aufgrund der
 * Elementdaten "p" zurückgeben. Falls das Attribut nicht
 * angegeben wurde, wird "Default" zurückgegeben.
 * Rückgabewert: Zeiger auf den Attributwert.
 */
{
  int i;

  for (i = 0; i < p->nAttributes; i++)
    if (!strcmp (p->Attribute[i].Name, Attribute))
      return p->Attribute[i].Value;
  return Default;
}

int muXML_GetHeader (int (*GetChar) (void), struct muXML_State *p)
/*
 * Einlesen des Headers "p", wobei die Zeichen über
 * "GetChar" gelesen werden.
 * Rückgabewert: muXML_FINISHED, falls die Auswertung beendet ist;
 *               muXML_ERROR im Fehlerfall.
 */
{
  int rc, c;

  muXML_InitState (p);

  do {
    c = GetChar ();
    if (c < 0) return muXML_ERROR;
    rc = muXML_ScanHeader (c, p);
  } while (rc == muXML_IN_PROGRESS);
  return rc;
}

int muXML_GetTag (int (*GetChar) (void), struct muXML_State *p)
/*
 * Einlesen des Tags "p", wobei die Zeichen über
 * "GetChar" gelesen werden.
 * Rückgabewert: muXML_FINISHED, falls die Auswertung beendet ist;
 *               muXML_ENDTAG, falls ein End-Tag erkannt wurde;
 *               muXML_ERROR im Fehlerfall.
 */
{
  int rc, c;

  muXML_InitState (p);

  do {
    c = GetChar ();
    if (c < 0) return muXML_ERROR;
    rc = muXML_ScanTag (c, p);
  } while (rc == muXML_IN_PROGRESS);
  return rc;
}

int muXML_GetEndTag (int (*GetChar) (void), struct muXML_State *p)
/*
 * Einlesen des (erkannten) End-Tags zu "p", wobei die Zeichen über
 * "GetChar" gelesen werden.
 * Rückgabewert: muXML_FINISHED, falls die Auswertung beendet ist;
 *               muXML_ERROR im Fehlerfall.
 */
{
  int rc, c;

  muXML_SetEndState (p);

  do {
    c = GetChar ();
    if (c < 0) return muXML_ERROR;
    rc = muXML_ScanEndTag (c, p);
  } while (rc == muXML_IN_PROGRESS);
  return rc;
}

int muXML_GetData (int (*GetChar) (void), struct muXML_State *p,
                   char *Data, int *pSize, int MaxSize)
/*
 * Lesen von Zeichen innerhalb des Elementes "p" nach "Data",
 * wobei die maximal zur Verfügung stehende Länge durch
 * "MaxSize" angegeben wird und die Zeichen über "GetChar"
 * gelesen werden.
 * Die Zeichen werden in "Data" als nullterminierte
 * Zeichenkette und die Länge der Daten in "*pSize"
 * abgelegt.
 * Rückgabewert: muXML_FINISHED, falls das Element beendet ist;
 *               muXML_ERROR im Fehlerfall.
 */
{
  int c, rc;

  *pSize = 0;

  do {
    c = GetChar ();
    if (c < 0) return muXML_ERROR;
    rc = muXML_ScanDataBlock (c, p, Data, pSize, MaxSize);
  } while (rc == muXML_IN_PROGRESS);
  return rc;
}

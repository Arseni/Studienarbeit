#include "muXMLTree.h"
#include "muXMLAccess.h"
#include <stdio.h>
#include <string.h>

static void Dump (struct muXMLTreeElement *p, int Indent)
{
  int i;

  for (; p; p = p->Next) {
    printf ("%*s<%s", Indent, "", p->Element.Name);
    for (i = 0; i < p->Element.nAttributes; i++)
      printf (" %s=\"%s\"",
              p->Element.Attribute[i].Name,
              p->Element.Attribute[i].Value);
    printf (">");
    for (i = 0; i < p->Data.DataSize; i++)
      putchar (p->Data.Data[i]);
    if (p->SubElements) {
      printf ("\n");
      Dump (p->SubElements, Indent + 2);
      printf ("%*s", Indent, "");
    }
    printf ("</%s>\n", p->Element.Name);
  }
}

typedef struct
{
enum eType
{
	NONE			= (0),
	BUTTON_DRIVER 	= (1<<0),
	JOB_HANDLER 	= (1<<1)
}eType;
union xValue
{
	char xButton[50];
	int xCapability[20];
}xValue;
}tBtnUnitQueueItem;

int main ()
{

	tBtnUnitQueueItem test;

  int Usage;
  struct muXMLTree *p;
  unsigned char Data[2048];
  char Stream[1024];
  static const char XML[] = "<epm><unit name=\"superpower\"><info><versions><software>1234</software><hardware>5678</hardware></versions><capabilities><capability type=\"download\" /><capability type=\"power\" /><capability type=\"serial\"><subtype id=\"0\">rs232</subtype><subtype id=\"1\">rs232 rs485</subtype></capability><description>Netzteil mit serieller Schnittstelle</description></capabilities></info></unit></epm>";
  static const char XML2[] = "<epm dest=\"192.168.64.98:12345\" uid=\"5566\" withseqno=\"yes\" withreltime=\"yes\" ds=\"10m\" dt=\"2s\" ack=\"yes\"><unit><get>wheel:wheelstate</get></unit></epm>";
  static const char XML3[] = "<test><test2>test</test2><test2></test2></test>";
  
  void * pp;
  pp = muXMLCreateTree(Data, sizeof(Data), "epm");
  muXMLUpdateAttribute(Data, &(((struct muXMLTree*)Data)->Root), "uid", "0");
  muXMLUpdateAttribute(Data, &(((struct muXMLTree*)Data)->Root), "ack", "yes");
  muXMLUpdateAttribute(Data, &(((struct muXMLTree*)Data)->Root), "home", "192.168.123.111:50001");
  muXMLCreateElement(pp, "unit");
  muXMLUpdateAttribute(Data, pp, "name", "power");
  pp = muXMLAddElement(Data, &(((struct muXMLTree*)Data)->Root), pp);

  muXMLTreeEncode(Stream, sizeof(Stream), Data);
  
  p = muXMLTreeDecode(XML3, Data, sizeof(Data), 1, &Usage);
  if(p)
  {
	  int i;
	  FILE * fp = fopen("test.txt", "w+");

	  struct muXMLTreeElement * tmp = muXMLGetElementByName(&(p->Root), "test2");
	  printf ("Memory Usage: %d of %d\n\n", Usage, sizeof (Data));
	  //muXMLUpdateData(p, tmp, "nu");
	  Dump (& p->Root, 0);
	  printf("Memory Usage: %d of %d\n\n", p->StorageInfo.SpaceInUse, p->StorageInfo.SpaceTotal);
	  for(i=0; i<p->StorageInfo.SpaceInUse; i++)
	  {
		  fprintf(fp, "%02X\t%c\r\n", Data[i], Data[i]);
	  }
	  fclose(fp);
  }
/*
  p = muXMLTreeDecode (XML, Data, sizeof (Data), 1, & Usage);
  if (p) {
    printf ("Memory Usage: %d of %d\n\n", Usage, sizeof (Data));
	
	struct muXMLTreeElement * tmp = muXML_GetElementByName(&(p->Root), "unit");
	tmp = muXML_GetElementByName(tmp, "info");
	tmp = muXML_GetElementByName(tmp, "versions");
	tmp = muXML_GetElementByName(tmp, "hardware");
	if(tmp != NULL)
		printf("name: %s\n\n", tmp->Data.Data);
	else
		printf("Knoten existiert nicht");
	
	Dump (& p->Root, 0);
    if (!muXMLTreeEncode (Stream, sizeof (Stream), p))
      printf ("%d:[%s]\n", strlen (Stream), Stream);
  }
/*
  printf ("\n\n\n");

  p = muXMLTreeDecode (XML2, Data, sizeof (Data), 1, & Usage);
  if (p) {
    printf ("Memory Usage: %d of %d\n\n", Usage, sizeof (Data));
    Dump (& p->Root, 0);
    printf ("\n");
    if (!muXMLTreeEncode (Stream, sizeof (Stream), p))
      printf ("%d:[%s]\n", strlen (Stream), Stream);
  }
/**/
  getchar();
  return 0;
}

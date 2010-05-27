#define UNIT_SHORT_STRING	10
#define UNIT_MIDDLE_STRING	20
#define UNIT_LONG_STRING		50

#define UNIT_MAX_CAPABILITIES	5

typedef enum
{
	UNIT_READY,
	UNIT_BUSY
}eUnitState;

typedef struct
{
	char Software[UNIT_SHORT_STRING];
	char Software[UNIT_SHORT_STRING];
	char SoftwareInfo[UNIT_LONG_STRING];
	char HardwareInfo[UNIT_LONG_STRING];
}tUnitVersion;

typedef struct
{
	char Type[UNIT_MIDDLE_STRING];
}tUnitCapability;

typedef struct
{
	char Name[UNIT_MIDDLE_STRING];
	tUnitVersion Version;
	eUnitState State;
	tUnitCapability Capabilities[UNIT_MAX_CAPABILITIES];
}tUnit;
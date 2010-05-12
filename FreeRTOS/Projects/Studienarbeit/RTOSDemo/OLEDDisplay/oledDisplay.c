#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "grlib.h"
#include "OLEDDisplay/rit128x96x4.h"
#include "OLEDDisplay/osram128x64x4.h"
#include "OLEDDisplay/formike128x128x16.h"
#include "OLEDDisplay/oledDisplay.h"
#include "OLEDDisplay/bitmap.h"

/* driver includes. */
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_sysctl.h"
#include "sysctl.h"

/* The maximum number of message that can be waiting for display at any one time. */
#define mainOLED_QUEUE_SIZE					( 3 )

/* Dimensions the buffer into which the jitter time is written. */
#define mainMAX_MSG_LEN						LCD_MESSAGE_MAX_LENGTH

/* The queue used to send messages to the OLED task. */
xQueueHandle xOLEDQueue = NULL;

/* The welcome text. */
const portCHAR * const pcWelcomeMessage = "   Better than ever";

/*
 * The display is written two by more than one task so is controlled by a
 * 'gatekeeper' task.  This is the only task that is actually permitted to
 * access the display directly.  Other tasks wanting to display a message send
 * the message to the gatekeeper.
 */
void vOLEDTask( void *pvParameters )
{
xOLEDMessage xMessage;
unsigned portLONG ulY, ulMaxY;
static portCHAR cMessage[ mainMAX_MSG_LEN ];
extern volatile unsigned portLONG ulMaxJitter;
unsigned portBASE_TYPE uxUnusedStackOnEntry, uxUnusedStackNow;
const unsigned portCHAR *pucImage;

/* Create the queue used by the OLED task.  Messages for display on the OLED
are received via this queue. */
xOLEDQueue = xQueueCreate( mainOLED_QUEUE_SIZE, sizeof( xOLEDMessage ) );

/* Functions to access the OLED.  The one used depends on the dev kit
being used. */
void ( *vOLEDInit )( unsigned portLONG ) = NULL;
void ( *vOLEDStringDraw )( const portCHAR *, unsigned portLONG, unsigned portLONG, unsigned portCHAR ) = NULL;
void ( *vOLEDImageDraw )( const unsigned portCHAR *, unsigned portLONG, unsigned portLONG, unsigned portLONG, unsigned portLONG ) = NULL;
void ( *vOLEDClear )( void ) = NULL;

	/* Just for demo purposes. */
	uxUnusedStackOnEntry = uxTaskGetStackHighWaterMark( NULL );

	/* Map the OLED access functions to the driver functions that are appropriate
	for the evaluation kit being used. */
	switch( HWREG( SYSCTL_DID1 ) & SYSCTL_DID1_PRTNO_MASK )
	{
		case SYSCTL_DID1_PRTNO_6965	:
		case SYSCTL_DID1_PRTNO_2965	:	vOLEDInit = OSRAM128x64x4Init;
										vOLEDStringDraw = OSRAM128x64x4StringDraw;
										vOLEDImageDraw = OSRAM128x64x4ImageDraw;
										vOLEDClear = OSRAM128x64x4Clear;
										ulMaxY = mainMAX_ROWS_64;
										pucImage = pucBasicBitmap;
										break;

		case SYSCTL_DID1_PRTNO_1968	:
		case SYSCTL_DID1_PRTNO_8962 :	vOLEDInit = RIT128x96x4Init;
										vOLEDStringDraw = RIT128x96x4StringDraw;
										vOLEDImageDraw = RIT128x96x4ImageDraw;
										vOLEDClear = RIT128x96x4Clear;
										ulMaxY = mainMAX_ROWS_96;
										pucImage = pucBasicBitmap;
										break;

		default						:	vOLEDInit = vFormike128x128x16Init;
										vOLEDStringDraw = vFormike128x128x16StringDraw;
										vOLEDImageDraw = vFormike128x128x16ImageDraw;
										vOLEDClear = vFormike128x128x16Clear;
										ulMaxY = mainMAX_ROWS_128;
										pucImage = pucGrLibBitmap;
										break;
	}

	ulY = ulMaxY;

	/* Initialise the OLED and display a startup message. */
	vOLEDInit( ulSSI_FREQUENCY );
	vOLEDStringDraw( "POWERED BY FreeRTOS", 0, 0, mainFULL_SCALE );
	vOLEDImageDraw( pucImage, 0, mainCHARACTER_HEIGHT + 1, bmpBITMAP_WIDTH, bmpBITMAP_HEIGHT );

	for( ;; )
	{
		/* Wait for a message to arrive that requires displaying. */
		xQueueReceive( xOLEDQueue, &xMessage, portMAX_DELAY );

		/* Write the message on the next available row. */
		ulY += mainCHARACTER_HEIGHT;
		if( ulY >= ulMaxY )
		{
			ulY = mainCHARACTER_HEIGHT;
			vOLEDClear();
			vOLEDStringDraw( pcWelcomeMessage, 0, 0, mainFULL_SCALE );
		}

		/* Display the message along with the maximum jitter time from the
		high priority time test. */
		sprintf( cMessage, "%s", xMessage.pcMessage );
		vOLEDStringDraw( cMessage, 0, ulY, mainFULL_SCALE );
	}
}
/*-----------------------------------------------------------*/

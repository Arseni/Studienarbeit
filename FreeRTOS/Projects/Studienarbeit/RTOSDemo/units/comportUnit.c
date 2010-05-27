#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "unit.h"
#include "comportUnit.h"

/**
 *  Semapthore, die die interkommunikation synchronisiert
 *  zwischen Jobdispatcher und comport
 */
xSemaphoreHandle xComportUnitTaskSemaphore;

/**
 * Description !!!
 */
void vComportUnitTask( void *pvParameters )
{
	const portTickType xDelay = 1000 / portTICK_RATE_MS;
	int c;

	// Setup com port
	xComOpen(1,2,3,4,5,6);

	// Setup Unit
	vSemaphoreCreateBinary( xComportUnitTaskSemaphore );

	// Periodic
	for(;;)
	{
		c = xComGetChar(1, 0);
		vComPutChar(1, c, 0);
	}
}
/*-----------------------------------------------------------*/

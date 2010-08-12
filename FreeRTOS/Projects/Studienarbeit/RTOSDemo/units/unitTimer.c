#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Library includes. */
#include "hw_types.h"
#include "sysctl.h"
#include "gpio.h"
#include "hw_memmap.h"

#include "unit.h"
#include "udpHandler.h"

#include "drivers/timer.h"

#define TIMER_MAX_TIMERS			10
#define TIMER_MAX_EVENTS_PENDING 	10

struct timerHandler
{
	 void (*callback)(struct tUnitJobHandler * xjob, uip_udp_endpoint_t sender);
	struct tUnitJobHandler * xjob;
	uip_udp_endpoint_t sender;
	int delay;
};

static struct timerHandler timers[TIMER_MAX_TIMERS];
static xQueueHandle xEventQueue;
static xSemaphoreHandle xTimerHandlerMutex;

static void onTimeElapsed(void)
{
	int i;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	for(i=0; i<TIMER_MAX_TIMERS; i++)
	{
		if(timers[i].delay-- == 0 && timers[i].callback != NULL)
		{
			xQueueSendFromISR(xEventQueue, &(timers[i]), &xHigherPriorityTaskWoken);
			timers[i].callback = NULL;
		}
	}

	// Now the buffer is empty we can switch context if necessary.
	if( xHigherPriorityTaskWoken )
	{
		taskYIELD();
	}
}

void vUnitTimerTask(void * pvParameters)
{
	struct timerHandler event;

	xEventQueue = xQueueCreate(TIMER_MAX_EVENTS_PENDING, sizeof(struct timerHandler));
	xTimerHandlerMutex = xSemaphoreCreateMutex();
	// init timer
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
	TimerControlStall(TIMER0_BASE, TIMER_A, true);
	TimerPrescaleSet(TIMER0_BASE, TIMER_A, 0);
	TimerLoadSet(TIMER0_BASE, TIMER_A, configCPU_CLOCK_HZ/1000);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerIntRegister(TIMER0_BASE, TIMER_A, onTimeElapsed);
	TimerEnable(TIMER0_BASE, TIMER_A);
	for(;;)
	{
		xQueueReceive(xEventQueue, &event, portMAX_DELAY);
		event.callback(event.xjob, event.sender);
	}
}


void vUnitTimerStart(int msDelay,
					 void callback(struct tUnitJobHandler * xjob, uip_udp_endpoint_t sender),
					 struct tUnitJobHandler * xjob,
					 uip_udp_endpoint_t sender)
{
	int i;
	xSemaphoreTake(xTimerHandlerMutex, portMAX_DELAY);
	for(i=0; i<TIMER_MAX_TIMERS; i++)
	{
		if(timers[i].callback == NULL)
		{
			timers[i].callback = callback;
			timers[i].xjob = xjob;
			timers[i].sender = sender;
			timers[i].delay = msDelay;
			break;
		}
	}
	xSemaphoreGive(xTimerHandlerMutex);
}


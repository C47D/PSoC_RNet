#include "project.h"

#include "FreeRTOS.h"
#include "task.h"

#include "RNet_App.h"

void prvHardwareSetup(void);

int main(void)
{    
    prvHardwareSetup();

    RNETApp_Init();
    
    vTaskStartScheduler();

    while(1) {
    }
}

void prvHardwareSetup(void)
{
    // Port layer functions that need to be copied into the vector table.
    extern void xPortPendSVHandler(void);
    extern void xPortSysTickHandler(void);
    extern void vPortSVCHandler(void );

    const uint8_t CORTEX_INTERRUPT_BASE = 16;
    // Handler for Cortex Supervisor Call (SVC) - Address 11 (SVCall_IRQn = -5)
    CyIntSetSysVector(CORTEX_INTERRUPT_BASE + SVCall_IRQn,
                    (cyisraddress) vPortSVCHandler);
    // Handler for Cortex PendSV Call - Address 14 (PendSV_IRQn = -2)
    CyIntSetSysVector(CORTEX_INTERRUPT_BASE + PendSV_IRQn,
                    (cyisraddress) xPortPendSVHandler);
    // Handler for Cortex SYSTICK - Address 15 (SysTick_IRQn = -1)
    CyIntSetSysVector(CORTEX_INTERRUPT_BASE + SysTick_IRQn,
                    (cyisraddress) xPortSysTickHandler);
    
    // Start-up the peripherals.
    CyGlobalIntEnable;
    
    UART_Start();
    // The SPI is started on the rf1_start function
    RF1_start();
}

// The heap space has been execeeded.
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    while(1) {
    }
}

// The stack space has been overflowed
void vApplicationStackOverflowHook(void)
{
    taskDISABLE_INTERRUPTS();
    while(1) {
    }
}

/* [] END OF FILE */

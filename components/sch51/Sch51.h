/*------------------------------------------------------------------*-

   SCH51.H (v1.00)

  ------------------------------------------------------------------

   - see SCH51.C for details


   COPYRIGHT
   ---------

   This code is from the book:

   PATTERNS FOR TIME-TRIGGERED EMBEDDED SYSTEMS by Michael J. Pont
   [Pearson Education, 2001; ISBN: 0-201-33138-1].

   This code is copyright (c) 2001 by Michael J. Pont.

   See book for copyright details and other information.

-*------------------------------------------------------------------*/

#ifndef _SCH51_H
#define _SCH51_H

#include "stdint.h"
#include "Sch51_config.h"

// ------ Public data type declarations ----------------------------
// Store in DATA area, if possible, for rapid access
// Total memory per task is 7 bytes

typedef struct
{
   // Pointer to the task (must be a 'void (void)' function)
   void (*pTask)(void);

   // Delay (ticks) until the function will (next) be run
   // - see SCH_Add_Task() for further details
   uint16_t Delay;

   // Interval (ticks) between subsequent runs.
   // - see SCH_Add_Task() for further details
   uint16_t Period;

   // Incremented (by scheduler) when task is due to execute
   uint8_t RunMe;
} sTask;

// ------ Public function prototypes -------------------------------
// Core scheduler functions
void SCH_Report_Status(void);
uint8_t SCH_Delete_Task(const uint8_t);                                        // Delete a task from the scheduler
uint8_t SCH_Add_Task(void (*pFunction)(void), const uint16_t, const uint16_t); // Add a new task to the scheduler
void SCH_Dispatch_Tasks(void);                                                 // Run a task (if one is ready) put it into main loop
void SCH_Dispatch_IT(void);                                                    // Put this into Timer ISR, with the period set by the user

// ------ Public constants -----------------------------------------
// The maximum number of tasks required at any one time
// during the execution of the program
//
// MUST BE ADJUSTED FOR EACH NEW PROJECT
#define SCH_MAX_TASKS (16)

#endif

/*------------------------------------------------------------------*-
  ---- END OF FILE -------------------------------------------------
-*------------------------------------------------------------------*/

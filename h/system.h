#ifndef _system_h_
#define _system_h_

// lock and unlock functions are used to secure critical sections by preventing context switch from happening
// It ensures that object's state stays valid during the time the system is in the critical section
// lock will increment lockFlag, meaning that the lockFlag's value represents how many nested critical sections have been entered
#define THREADSAFE_LOCK lockFlag++;
// unlock function will decrement lockFlag, which means that one (non-nested or nested) critical section has been exited
// during the time context switch was not allowed there may have been requests for context switch;
// Therefore, system checks for context_switch_on_demand flag; if it is set, dispatch function is called, BUT only if the lockFlag is 1, which means that the system has exited
// all of the nested (if there were any) critical sections
#define THREADSAFE_UNLOCK lockFlag--;\
	if ((lockFlag == 1) && (context_switch_on_demand == 1)) {\
		dispatch();\
	}
#define THREADSAFE_SOFT_UNLOCK lockFlag--;

#include <dos.h>
#include <iostream.h>
// pInterrupt typedef is used for compatibility reasons (getvect and setvect functions from <dos.h> take void interrupt interruptRoutine(...) functions as parameters)
typedef void interrupt (*pInterrupt)(...);

// initialize and restore functions are used to manipulate system timer's interrupt routine
// initialize function puts redefined system timer's interrupt routine into entry number 8 of the IVT (where original system timer's interrupt routine was originally), whilst saving it
// restore function gets original system timer's interrupt routine back into entry number 8 of the IVT
// these functions also dynamically allocate all of the data structures needed for the kernel... for example: list of all threads, main thread, etc...
void initialize();
void restore();

// redefined system timer's interrupt routine
void interrupt timer(...);

#endif

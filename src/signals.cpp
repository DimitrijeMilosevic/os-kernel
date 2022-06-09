#include "idlethrd.h"
#include "pcb.h"
#include "schedule.h"
#include "signals.h"
#include "system.h"

/*
Signal 0 needs to 'kill' the running thread, with having all of its resources allocated - deallocated.
resources consist of every dynamically allocated data structures in the class PCB (the stack, the blocked threads list, a Node in the PCB::allThreads list,
signal handlers lists of all of the signals and signal wait list).
Note: Before deleting blockedThreads list, all of the eventual threads need to be deblocked!
Once the resources have been deallocted, the thread does not exist - threfore, before returning back to timer's interrupt routine,
the next runnable thread needs to be chosen as the running thread (the idle thread if there is no such thread)
Important note: signal0 'kills' the thread by having all of its resources deallocated, but the destructor is never called, which means the object still DOES exist.
Therefore, stack, blockedThreads, all of the signalHandlers, signalWaitList are 'hanging' pointers, and they need to be updated to 0, because, once the destructor has been called,
it will attempt deleting all of the above
*/
void signal0() {
	// Deallocation
	PCB::running->threadState = PCB::THREAD_FINISHED;
	PCB* deblockedThread = PCB::running->blockedThreads->removeFirst();
	while (deblockedThread != 0) {
		deblockedThread->threadState = PCB::THREAD_READY;
		Scheduler::put(deblockedThread);
		deblockedThread = PCB::running->blockedThreads->removeFirst();
	}
	delete PCB::running->blockedThreads;
	PCB::running->blockedThreads = 0;
	delete PCB::running->stack;
	PCB::running->stack = 0;
	for (int i = 0; i < 16; i++)
		PCB::running->signalHandlers[i] = 0;
	delete PCB::running->signalWaitList;
	PCB::running->signalWaitList = 0;
	PCB::allThreads->remove((PCB*)PCB::running);
	// Update
	PCB::running = Scheduler::get();
	if (PCB::running == 0) PCB::running = IdleThread::idleThread->myPCB;
}

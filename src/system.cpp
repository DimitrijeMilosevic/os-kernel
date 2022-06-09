#include "idlethrd.h"
#include "kernsem.h"
#include "pcb.h"
#include "schedule.h"
#include "system.h"

//extern void tick();

// lockFlag is used to forbid context switch from happening while teh system is in the critical section
// Its value represents how many nested critical sections have been entered (lockFlag = number_of_nested_critical_sections_entered - 1),
// meaning that only when the lockFlag equals 1, a context switch can happen
volatile unsigned int lockFlag = 1;
// counter will be used in order to count how much more system timer's interrupt needs to proc before context switch needs to happen
// Each thread has its quantum, which represents maximum amount of time (x 55ms) that that very thread can be running before context switch needs to happen
volatile int counter = 2;
// context_switch_on_demand flag will be used to store whether or not some thread has requested for a context switch to happen or not
// If this flag is set, context switch can happen, BUT ONLY if no other thread is in its critical section
// Therefore, once a thread exits its critical section it will (after it sets lockFlag to 1) check if there was a requested context switch and, if so, call dispatch function
volatile unsigned int context_switch_on_demand = 0;
// unlimitedQuantumThread flag will indicate whether or not the current running thread has unlimited quantum
volatile unsigned unlimitedQuantumThread = 0;
// tss, tsp and tbp temporary variables will be used for accessing Stack Segment, Stack Pointer and Base Pointer registers (load/store)
volatile unsigned tss, tsp, tbp;
pInterrupt oldTimerRoutine;

void initialize() {
#ifndef BCC_BLOCK_IGNORE
	oldTimerRoutine = getvect(0x08);
	setvect(0x60, oldTimerRoutine);
	setvect(0x08, timer);
#endif
	Thread::createMainThread();
	IdleThread::createIdleThread();
	IdleThread::idleThread->start();
	PCB::running = PCB::mainThread->myPCB;
}

void restore() {
	delete KernelSem::allSemaphores;
	delete IdleThread::idleThread;
	delete PCB::mainThread;
	delete PCB::allThreads;
#ifndef BCC_BLOCK_IGNORE
	setvect(0x08, oldTimerRoutine);
#endif
}

void interrupt timer(...) {
	if (context_switch_on_demand == 0) {
			KernelSem::updateTimeBlocked();
	#ifndef BCC_BLOCK_IGNORE
			asm int 0x60;
	#endif
			//tick();
	}
	if ((unlimitedQuantumThread == 0) && (context_switch_on_demand == 0)) counter--;
	if ((context_switch_on_demand == 1) || ((unlimitedQuantumThread == 0) && (counter == 0))) {
		if (lockFlag == 1) {
			context_switch_on_demand = 0;
#ifndef BCC_BLOCK_IGNORE
			asm {
				mov tsp, sp
				mov tss, ss
				mov tbp, bp
			}
#endif
			PCB::running->sp = tsp;
			PCB::running->ss = tss;
			PCB::running->bp = tbp;
			if (PCB::running->threadState == PCB::THREAD_RUNNING) {
				// If the running thread enters timer's interrupt routine without having its state set to THREAD_BLOCKED or THREAD_FINISHED,
				// it can be put back into Scheduler, with having its state updated to THREAD_READY
				PCB::running->threadState = PCB::THREAD_READY;
				if (PCB::running != IdleThread::idleThread->myPCB)
					Scheduler::put((PCB*)PCB::running);
			}
			PCB::running = Scheduler::get();
			if (PCB::running == 0) PCB::running = IdleThread::idleThread->myPCB;
			/* === SIGNALS === */
			if (PCB::running != IdleThread::idleThread->myPCB) {// Idle thread has no signal functionalities
				List<SignalID>::Node *currentSignal = PCB::running->signalWaitList->first, *previousSignal = 0;
				while (currentSignal != 0) {
					SignalID nextSignalToBeHandled = currentSignal->data;
					if ((PCB::globalSignalBlockFlags[nextSignalToBeHandled] == 0) && (PCB::running->signalBlockFlags[nextSignalToBeHandled] == 0)) {// Signal can be handled, since it is not blocked (non-globally or globally)
						List<SignalID>::Node* oldSignal = currentSignal;
						if (previousSignal == 0) {
							PCB::running->signalWaitList->first = PCB::running->signalWaitList->first->next;
							if (PCB::running->signalWaitList->first == 0)
								PCB::running->signalWaitList->last = 0;
						}
						else {
							previousSignal->next = currentSignal->next;
							if (currentSignal->next == 0)
								PCB::running->signalWaitList->last = previousSignal;
						}
					currentSignal = currentSignal->next;
					delete oldSignal;
					// Handling all of the signals from the wait list
					THREADSAFE_LOCK
					for(List<SignalHandler>::Node* nextHandler = PCB::running->signalHandlers[nextSignalToBeHandled]->first; nextHandler != 0; nextHandler = nextHandler->next)
						nextHandler->data();
					THREADSAFE_SOFT_UNLOCK
					// If the signal 0 has been handled - break - the thread does not exist anymore
					if (nextSignalToBeHandled == 0) break;
					}
					else {
						previousSignal = currentSignal;
						currentSignal = currentSignal->next;
					}
				}
			}
			/* === SIGNALS === */
			counter = PCB::running->timeSlice;
			unlimitedQuantumThread = PCB::running->hasUnlimitedQuantum;
			PCB::running->threadState = PCB::THREAD_RUNNING;
			tsp = PCB::running->sp;
			tss = PCB::running->ss;
			tbp = PCB::running->bp;
#ifndef BCC_BLOCK_IGNORE
			asm {
				mov sp, tsp
				mov ss, tss
				mov bp, tbp
			}
#endif
		}
		else context_switch_on_demand = 1;
	}
}

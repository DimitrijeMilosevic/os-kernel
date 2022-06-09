#include "ivtentry.h"
#include "kernev.h"
#include "schedule.h"

extern volatile unsigned int context_switch_on_demand;

KernelEv::KernelEv(Event* myEv, IVTNo _ivtNo) {
	if (IVTEntry::allEvents[ivtNo] == 0) {// There must be only one Event object registered in a certain entry of the IVT
		myEvent = myEv;
		ivtNo = _ivtNo;
		threadHolder = (PCB*)PCB::running;
		isThreadBlocked = 0;
		eventValue = 0;
#ifndef BCC_BLOCK_IGNORE
		oldRoutine = getvect(ivtNo);
		setvect(ivtNo, IVTEntry::allRoutines[ivtNo]);
#endif
		IVTEntry::allEvents[ivtNo] = this;
	}
}

KernelEv::~KernelEv() {
	if (IVTEntry::allEvents[ivtNo] != 0) {
		if (isThreadBlocked == 1) {
			threadHolder->threadState = PCB::THREAD_READY;
			isThreadBlocked = 0;
			Scheduler::put(threadHolder);
		}
#ifndef BCC_BLOCK_IGNORE
		setvect(ivtNo, oldRoutine);
#endif
		IVTEntry::allEvents[ivtNo] = 0;
	}
}

void KernelEv::wait() {
	if (PCB::running == threadHolder) {
		if (eventValue == 1) eventValue = 0;
		else {
			threadHolder->threadState = PCB::THREAD_BLOCKED;
			isThreadBlocked = 1;
			context_switch_on_demand = 1;
		}
	}
}

void KernelEv::signal() {
	if (isThreadBlocked == 0) eventValue = 1;
	else {
		threadHolder->threadState = PCB::THREAD_READY;
		isThreadBlocked = 0;
		Scheduler::put(threadHolder);
	}
}


#ifndef _kernev_h_
#define _kernev_h_

#include "event.h"
#include "pcb.h"

class KernelEv {
public:

	KernelEv(Event* myEv, IVTNo _ivtNo);
	~KernelEv();

	/*
	wait method blocks the thread who called it, but only if that thread is the thread holder of the KernelEv object
	if it is not - wait method has no effect
	*/
	void wait();
	/*
	signal method deblocks the blocked thread holder. signal method isn't called manually, but rather from the interrupt routine, automatically, once the interrupt occurs
	*/
	void signal();

	friend class IVTEntry;
private:
	// Each KernelEv object is related to one Event object; no two KernelEv objects are related to the same Event object; same goes vice-versa
	Event* myEvent;
	// Each event object is related to one entry in the IVT. Event happens whenever the interrupt from that very entry of the IVT happens; besides the 'normal' things
	// interrupt routine also signals on the KernelEv object related to it
	IVTNo ivtNo;
	// Each KernelEv object has its thread holder, and ONLY THAT thread can call wait on the KernelEv object. If any other thread (which isn't the holder thread)
	// tries calling wait on that KernelEv object, those methods are effectless
	// Thread holder is the thread running at the time the KernelEv object is created
	PCB* threadHolder;
	unsigned int isThreadBlocked;
	// Event is a binary semaphore, so its value can be either 0 or 1 (0 means that if the thread holder tries calling wait on its event, it will get blocked, 1 means the opposite)
	unsigned int eventValue;
	pInterrupt oldRoutine;

};

#endif

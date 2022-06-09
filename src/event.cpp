#include "event.h"
#include "kernev.h"

extern volatile unsigned int lockFlag;
extern volatile unsigned int context_switch_on_demand;

Event::Event(IVTNo ivtNo) {
	THREADSAFE_LOCK
	myImpl = new KernelEv(this, ivtNo);
	THREADSAFE_UNLOCK
}

Event::~Event() {
	THREADSAFE_LOCK
	delete myImpl;
	THREADSAFE_UNLOCK
}

void Event::wait() {
	THREADSAFE_LOCK
	myImpl->wait();
	THREADSAFE_UNLOCK
}

void Event::signal() {
	THREADSAFE_LOCK
	myImpl->signal();
	THREADSAFE_UNLOCK
}

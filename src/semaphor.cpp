#include "kernsem.h"
#include "semaphor.h"

extern volatile unsigned int lockFlag;
extern volatile unsigned int context_switch_on_demand;
extern void dispatch();

Semaphore::Semaphore(int init) {
	THREADSAFE_LOCK
	myImpl = new KernelSem(this, init);
	THREADSAFE_UNLOCK
}

Semaphore::~Semaphore() {
	THREADSAFE_LOCK
	delete myImpl;
	THREADSAFE_UNLOCK
}

int Semaphore::wait(Time maxTimeToWait) {
	return myImpl->wait(maxTimeToWait);
}

int Semaphore::signal(int n) {
	THREADSAFE_LOCK
	int result = myImpl->signal(n);
	THREADSAFE_UNLOCK
	return result;
}

int Semaphore::val() const { return myImpl->val(); }

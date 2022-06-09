#include "pcb.h"
#include "thread.h"

extern volatile unsigned int lockFlag;
extern volatile unsigned int context_switch_on_demand;

Thread::Thread(StackSize stackSize, Time timeSlice) {
	THREADSAFE_LOCK
	myPCB = new PCB(this, stackSize, timeSlice);
	THREADSAFE_UNLOCK
}

Thread::Thread(char create_main_thread) {
	myPCB = new PCB(this, create_main_thread);
}

void Thread::createMainThread() {
	if (PCB::mainThread == 0)
		PCB::mainThread = new Thread('M');// Thread(char)'s parameter is not used anywhere. It makes sure that there are no two default constructors in the class Thread
}

void Thread::start() {
	THREADSAFE_LOCK
	myPCB->start();
	THREADSAFE_UNLOCK
}

void Thread::waitToComplete() {
	THREADSAFE_LOCK
	myPCB->waitToComplete();
	THREADSAFE_UNLOCK
}

Thread::~Thread() {
	THREADSAFE_LOCK
	delete myPCB;
	THREADSAFE_UNLOCK
}

ID Thread::getId() { return myPCB->getId(); }
ID Thread::getRunningId() {	return PCB::getRunningId(); }
Thread* Thread::getThreadById(ID id) { return PCB::getThreadById(id); }

void dispatch() {
#ifndef BCC_BLOCK_IGNORE
	asm cli;
#endif
	context_switch_on_demand = 1;
	timer();
#ifndef BCC_BLOCK_IGNORE
	asm sti;
#endif
}

/* === SIGNALS === */
void Thread::signal(SignalID signal) {
	THREADSAFE_LOCK
	myPCB->signal(signal);
	THREADSAFE_UNLOCK
}

void Thread::registerHandler(SignalID signal, SignalHandler handler) {
	THREADSAFE_LOCK
	myPCB->registerHandler(signal, handler);
	THREADSAFE_UNLOCK
}
void Thread::unregisterAllHandlers(SignalID id) {
	THREADSAFE_LOCK
	myPCB->unregisterAllHandlers(id);
	THREADSAFE_UNLOCK
}
void Thread::swap(SignalID id, SignalHandler hand1, SignalHandler hand2) {
	THREADSAFE_LOCK
	myPCB->swap(id, hand1, hand2);
	THREADSAFE_UNLOCK
}

void Thread::blockSignal(SignalID signal) {
	THREADSAFE_LOCK
	myPCB->blockSignal(signal);
	THREADSAFE_UNLOCK
}
void Thread::blockSignalGlobally(SignalID signal) {
	THREADSAFE_LOCK
	PCB::blockSignalGlobally(signal);
	THREADSAFE_UNLOCK
}
void Thread::unblockSignal(SignalID signal) {
	THREADSAFE_LOCK
	myPCB->blockSignal(signal);
	THREADSAFE_UNLOCK
}
void Thread::unblockSignalGlobally(SignalID signal) {
	THREADSAFE_LOCK
	PCB::unblockSignalGlobally(signal);
	THREADSAFE_UNLOCK
}
/* === SIGNALS === */

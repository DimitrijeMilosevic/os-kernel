#ifndef _idlethrd_h_
#define _idlethrd_h_

#include "pcb.h"

// Idle thread is a thread which will be chosen as the running thread if the Scheduler does not have any thread ready to be the running thread
// Idle thread's run method is just while (1); - idle thread just needs to buy time for another thread to become ready to be the running thread
class IdleThread : public Thread {
public:

	static void createIdleThread() {
		if (IdleThread::idleThread == 0) {
			idleThread = new IdleThread();
		}
	}
	void run() { while (1); }
	// Idle thread should never finish in Scheduler!
	void start() {
		if (IdleThread::idleThread != 0)
			IdleThread::idleThread->myPCB->threadState = PCB::THREAD_READY;
	}

	friend void initialize();
	friend void interrupt timer(...);
	friend void restore();
	/* === SIGNALS === */
	friend void signal0();
	/* === SIGNALS === */
private:
	// There should only be one running thread, and therefore constructor is private, and idle thread is being allocated through the static method createIdleThread
	IdleThread() : Thread(1024, 1) {}
	static IdleThread* idleThread;
};

#endif

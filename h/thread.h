#ifndef _thread_h_
#define _thread_h_

#include "system.h"

typedef unsigned long StackSize;
const StackSize defaultStackSize = 4096;
typedef unsigned int Time; // time, x 55ms
const Time defaultTimeSlice = 2; // default = 2*55ms
typedef int ID;

/* === SIGNALS === */
typedef void (*SignalHandler)();
typedef unsigned short SignalID;
/* === SIGNALS === */

class PCB; // Kernel's implementation of a user's thread

class Thread {
public:

	void start();
	void waitToComplete();
	virtual ~Thread();

	ID getId();
	static ID getRunningId();
	static Thread* getThreadById(ID id);
	// createMainThread method creates a Thread object which will represent initial system thread created by the OS when the program starts
	static void createMainThread();

	friend void interrupt timer(...);
	/* === SIGNALS === */
	friend void signal0();
	void signal(SignalID signal);

	void registerHandler(SignalID signal, SignalHandler handler);
	void unregisterAllHandlers(SignalID id);
	void swap(SignalID id, SignalHandler hand1, SignalHandler hand2);

	void blockSignal(SignalID signal);
	static void blockSignalGlobally(SignalID signal);
	void unblockSignal(SignalID signal);
	static void unblockSignalGlobally(SignalID signal);
	/* === SIGNALS === */

	friend void initialize();

	friend class IdleThread;
protected:
	friend class PCB;
	Thread (StackSize stackSize = defaultStackSize, Time timeSlice = defaultTimeSlice);
	virtual void run() {}

private:
	PCB* myPCB;
	// Thread(char) is private because the Thread(char) constructor is used for creating the 'main' thread... There should only be one 'main' thread;
	// therefore, the Thread(char) is private, and it is being called from the createMainThread static method, which ensures that the 'main' thread is created only ONCE
	Thread(char create_main_thread);
};

void dispatch();

#endif

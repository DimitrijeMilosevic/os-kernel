#ifndef _pcb_h_
#define _pcb_h_

#include "list.h"
#include "thread.h"

class PCB {
public:
	PCB(Thread* myThrd, StackSize stackSz, Time timeSlc);
	void start();
	void waitToComplete();
	~PCB();

	ID getId();
	static ID getRunningId();
	static Thread* getThreadById(ID id);
	// Threads initial context will contain thread's run method's address. Since the run method is not a static method of the class Thread, it needs an object on which it can be called
	// To make things easier, there is a static wrapper method which calls the run method of the running thread. Since it is a static method of the class PCB, it does not need any object
	// to be called and therefore simplifies creating thread's initial context
	// This mathod also does final things, once the run method completes - for example: deblocking all of the threads waiting for the running thread to finish its 'work'
	static void wrapper();
	// Thread's possible states:
	enum ThreadState { THREAD_CREATED = 1, THREAD_READY, THREAD_RUNNING, THREAD_BLOCKED, THREAD_FINISHED };

	friend void initialize();
	friend void interrupt timer(...);
	friend void restore();
	/* === SIGNALS === */
	friend void signal0();
	/* === SIGNALS === */

	friend class IdleThread;
	friend class KernelEv;
	friend class KernelSem;
	friend class Thread;
	/* === SIGNALS === */
	void signal(SignalID signal);

	void registerHandler(SignalID signal, SignalHandler handler);
	void unregisterAllHandlers(SignalID id);
	void swap(SignalID id, SignalHandler hand1, SignalHandler hand2);

	void blockSignal(SignalID signal);
	static void blockSignalGlobally(SignalID signal);
	void unblockSignal(SignalID signal);
	static void unblockSignalGlobally(SignalID signal);
	/* === SIGNALS === */
private:
	static Thread* mainThread;
	// running pointer will point towards the currently running thread. It will be updated whenever a context switch happens
	static volatile PCB* running;
	// allThreads list will store all of the thread (PCB) objects ever created, and it will be used for methods such as getThreadById
	// Each thread (PCB) object will be put into this list in the constructor, and removed from the same in the destructor
	static List<PCB*>* allThreads;
	// Each Thread (PCB) object has its unique identifier which is generated automatically by the idGenerator
	static unsigned idGenerator;
	unsigned id;
	// Each PCB object is related to one Thread object, and no two PCB objects can be related to the same Thread object; same goes vice-versa
	Thread* myThread;
	// Each PCB object has its own stack used to save/restore its context each time it needs to be saved/restored(for example: when calling a function/method and returning from it
	// , when a context switch happens, when a thread gets chosen from the Scheduler, and it needs to continue its 'work', etc...)
	StackSize stackSize;
	unsigned* stack;
	// Each thread has its quantum - the amount of time (x 55ms) that the thread can be running, before context switch needs to happen
	// Note: That does not mean that the context switch only happens when the quantum runs out, it can also happen on demand
	// If the timeSlice parameter of the class Thread's constructor is 0, that means that the thread has an unlimited quantum - it can lose processor only if there was a context switch request
	// and it isn't in its critical section
	// Instead of checking if the timeSlice field is 0, each PCB object has an unsigned flag - hasUnlimitedQuantum, for this purpose
	Time timeSlice;
	unsigned hasUnlimitedQuantum;
	// ss, sp and bp registers are not saved/restored automatically as the part of the thread's context once an interrupt happens/once kernel returns from the interrupt routine
	// Therefore, they need to be manually saved/restored - so each PCB object has 3 unsigned fields which will store these register's values
	unsigned ss, sp, bp;
	/* Each thread has its state. It has one of the 5 possible states:
	1) CREATED: Thread is created, and it yet needs to be put into Scheduler in order to start its 'work'
	2) READY: Thread has been started, and it is ready to be the running thread - it is up to Scheduler to choose it to be the running thread
	3) RUNNING: Thread is currently running
	4) BLOCKED: Thread is blocked and it is not able to be the running thread. Once it gets deblocked, it can continue its 'work'
	5) FINISHED: Thread has finished its 'work'
	*/
	ThreadState threadState;
	/*
	Since any thread can call another thread's destructor (it is public), kernel MUST NOT allow a thread object to be destroyed if that thread has not yet finished its work...
	Therefore, a MUST DO is to call waitToComplete method in the destructor of a class which extends class Thread. waitToComplete method will block the thread who calls it, if that thread
	tries to destroy a thread which hasn't finished its 'work'.
	blockedThreads list will store all of the threads blocked, which wait for this thread to finish its 'work'; once it does all of the threads from this list will be deblocked
	*/
	List<PCB*>* blockedThreads;
	/* maxTimeToWait field has different meanings, depending on its value:
	   	   -1: thread is not currently blocked (for no longer than a certain amount of time) on any semaphore
	   	   0: thread was blocked on a semaphore for no longer than a certain amount of time, and that time ran out, which means that the thread gets deblocked
	   	   >0: thread is currently blocked on a semaphore for no longer than a certain amount of time, and maxTimeToWait field represents how much more time does the thread
	   	   need to be blocked (x 55ms)
	*/
	int maxTimeToWait;
	// A helper method called from the constructor which will put thread's initial context onto the stack
	void createInitialContext();
	// PCB(Thread*, char) constructor is private because it is used for creating a PCB object for the 'main' thread
	PCB(Thread* mainThread, char create_main_thread);
	/* === SIGNALS === */
	// signalHandlers is an array with 16 entries, with each one of the entries pointing towards the SignalHandler(s) for the signal with an id the same as the index of that entry
	List<SignalHandler>* signalHandlers[16];
	// signalWaitList will store all of the signals that need to be handled
	List<SignalID>* signalWaitList;
	// signalBlockFlags and globalSignalBlockFlags are arrays with 16 entries (one non-static, and one static, respectively); each one of the entries are flags which tell whether or not the signal
	// , with an id the same as the index of that entry, has been blocked (non-globally or globally, respectively)
	// If a signal is blocked (non-globally or globally), it cannot be handled; not untill it gets unblocked
	static unsigned globalSignalBlockFlags[16];
	unsigned signalBlockFlags[16];
	// Each thread created (except for the main thread), inherits all of the signal options (all registered handles, non-global block flags, signal wait list) from the 'parent' thread
	// Also, each thread created (except for the main thread), signals 1 towards its 'parent' thread when it (not the 'parent' thread) finishes its 'work'; therefore, a PCB* towards the 'parent' thread is needed
	PCB* parentThread;
	// Helper methods called from the constructors, used to initialize all of the data structures used for the signals
	void initializeSignalDataStructures();
	void initializeMainSignalDataStructures();
	// Helper method called from the destructor, used to deallocate all of the data structures allocated for the signals
	void deallocateSignalDataStructures();
	/* === SIGNALS === */
};

#endif

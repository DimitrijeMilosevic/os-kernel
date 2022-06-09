#include "pcb.h"
#include "schedule.h"

Thread* PCB::mainThread = 0;
volatile PCB* PCB::running = 0;
List<PCB*>* PCB::allThreads = new List<PCB*>();
unsigned PCB::idGenerator = 1;
/* === SIGNALS === */
unsigned PCB::globalSignalBlockFlags[16] = { 0 };
/* === SIGNALS === */

extern volatile unsigned int lockFlag;
extern volatile unsigned int context_switch_on_demand;

/* === SIGNALS === */
extern void signal0();
/* === SIGNALS === */

void PCB::createInitialContext() {
	// On the bottom of the stack is PSW's initial value - 0x200 which means that only the bit I is active (interrupts are allowed)
	stack[stackSize - 1] = 0x200;
#ifndef BCC_BLOCK_IGNORE
	// Next up is the PC's initial value - static method wrapper
	stack[stackSize - 2] = FP_SEG(&(PCB::wrapper));
	stack[stackSize - 3] = FP_OFF(&(PCB::wrapper));
	// Next up are the initial values of the SP (its segment and offset part) and BP
	// In between stackSize - 3 and stackSize - 12 are all of the registers from the x86 architecture - ax, bx, cx, dx, es, ds, si, di, bp
	// Their initial values are random, since their initial values aren't important
	sp = FP_OFF(stack + stackSize - 12);
	ss = FP_SEG(stack + stackSize - 12);
	bp = FP_OFF(stack + stackSize - 12);
#endif
}

PCB::PCB(Thread* myThrd, StackSize stackSz, Time timeSlc) : myThread(myThrd), timeSlice(timeSlc), threadState(PCB::THREAD_CREATED), maxTimeToWait(-1) {
	id = idGenerator++;
	if (timeSlice == 0) hasUnlimitedQuantum = 1;
	else hasUnlimitedQuantum = 0;
	if (stackSz > 65536) stackSz = defaultStackSize;
	// unsigned data type is 2B 'long', therefore 64KB equals 32K of unsigned data type variables - so stackSz argument needs to be divided by sizeof(unsigned) (which equals 2)
	stackSize = stackSz / sizeof(unsigned);
	stack = new unsigned[stackSize];
	createInitialContext();
	blockedThreads = new List<PCB*>();
	/* === SIGNALS === */
	initializeSignalDataStructures();
	/* === SIGNALS === */
	PCB::allThreads->add(this);
}

PCB::PCB(Thread* mainThread, char create_main_thread)
: myThread(mainThread), stackSize(defaultStackSize / sizeof(unsigned)), timeSlice(defaultTimeSlice), threadState(PCB::THREAD_RUNNING), id(0), blockedThreads(0), hasUnlimitedQuantum(0),
		maxTimeToWait(-1) {
	stack = new unsigned[stackSize];
#ifndef BCC_BLOCK_IGNORE
	sp = FP_OFF(stack + stackSize - 1);
	ss = FP_SEG(stack + stackSize - 1);
	bp = FP_OFF(stack + stackSize - 1);
#endif
	/* === SIGNALS === */
	initializeMainSignalDataStructures();
	/* === SIGNALS === */
	PCB::allThreads->add(this);
}

// start method will only be called once - when a thread is created and needs to be put into Scheduler in order to start its 'work'
void PCB::start() {
	if (threadState == PCB::THREAD_CREATED) {
		threadState = PCB::THREAD_READY;
		Scheduler::put(this);
	}
}

void PCB::waitToComplete() {
	if ((threadState != PCB::THREAD_CREATED) && (threadState != PCB::THREAD_FINISHED)) {// waitToComplete method will block the running thread only if this thread has been started AND has
		// not yet finished its 'work'
		PCB::running->threadState = PCB::THREAD_BLOCKED;
		blockedThreads->add((PCB*)PCB::running);
		context_switch_on_demand = 1;
	}
}

PCB::~PCB() {
	delete stack;
	delete blockedThreads;
	/* === SIGNALS === */
	deallocateSignalDataStructures();
	/* === SIGNALS === */
	PCB::allThreads->remove(this);
}

ID PCB::getId() { return id; }

ID PCB::getRunningId() { return PCB::running->id; }

Thread* PCB::getThreadById(ID id) {
	if (id == 1) return 0;// idle thread (id = 1) cannot be accessed by the user, therefore getThreadById returns 0
	List<PCB*>::Node* currentPCB = PCB::allThreads->first;
	while (currentPCB != 0) {
		if (currentPCB->data->id == id) return currentPCB->data->myThread;
		currentPCB = currentPCB->next;
	}
	// If there was no return by this point, there is no thread with an id equal with the id - the argument of the getThreadById static method
	return 0;
}

void PCB::wrapper() {
	PCB::running->myThread->run();
	THREADSAFE_LOCK
	/* === SIGNALS === */
	// Once the thread finishes its run method it needs to signal 1 to the parent thread
	if (PCB::running->parentThread != 0)
		PCB::running->parentThread->signal(1);
	// Once the thread finishes its run method it needs to 'handle' signal 2
	List<SignalHandler>::Node* currentHandler = PCB::running->signalHandlers[2]->first;
	while (currentHandler != 0) {
		currentHandler->data();
		currentHandler = currentHandler->next;
	}
	/* === SIGNALS === */
	PCB::running->threadState = PCB::THREAD_FINISHED;
	// Once the thread finishes its run method, all of the threads blocked, waiting for it to finish its 'work' need to be deblocked
	PCB* deblockedThread = PCB::running->blockedThreads->removeFirst();
	while (deblockedThread != 0) {
		deblockedThread->threadState = PCB::THREAD_READY;
		Scheduler::put(deblockedThread);
		deblockedThread = PCB::running->blockedThreads->removeFirst();
	}
	context_switch_on_demand = 1;
	THREADSAFE_UNLOCK
}

/* === SIGNALS === */
// Important note: Idle thread has no signal functionalities; therefore, most of the methods below have a check if the thread being signaled is not the idle thread
void PCB::initializeSignalDataStructures() {
	if (id != 1) {// Idle thread has no signal functionalities
		parentThread = (PCB*)PCB::running;
		for (int i = 0; i < 16; i++) {
			signalHandlers[i] = new List<SignalHandler>();
			List<SignalHandler>::Node* currentNode = parentThread->signalHandlers[i]->first;
			while (currentNode != 0) {
				signalHandlers[i]->add(currentNode->data);
				currentNode = currentNode->next;
			}
		}
		signalWaitList = new List<SignalID>();
		List<SignalID>::Node* currentNode1 = parentThread->signalWaitList->first;
		while (currentNode1 != 0) {
			signalWaitList->add(currentNode1->data);
			currentNode1 = currentNode1->next;
		}
		for (int j = 0; j < 16; j++)
			signalBlockFlags[j] = parentThread->signalBlockFlags[j];
	}
	else {
		parentThread = 0;
		for (int i = 0; i < 16; i++) signalHandlers[i] = 0;
		signalWaitList = 0;
		for (int j = 0; j < 16; j++) signalBlockFlags[j] = 0;
	}
}

void PCB::initializeMainSignalDataStructures() {
	parentThread = 0;
	for (int i = 0; i < 16; i++)
		signalHandlers[i] = new List<SignalHandler>();
	// Registering the default signal 0 handler
	signalHandlers[0]->add(signal0);
	signalWaitList = new List<SignalID>();
	for (int j = 0; j < 16; j++)
		signalBlockFlags[j] = 0;
}

void PCB::deallocateSignalDataStructures() {
		delete signalWaitList;
		for (int i = 0; i < 16; i++)
			delete signalHandlers[i];
}

void PCB::signal(SignalID signal) {
	if (signal > 15) return;
	// Important note: Signal with an ID of 2 is automatically 'handled' in the wrapper static method, once the thread finishes its run method
	if ((id == 1) || (signal == 2)) return;
	if ((signal == 0) && (id == 0)) return;// Main thread cannot be destroyed
	if ((signal != 1) || (PCB::getRunningId() == 0)) // Signal 1 must be sent only from the system - the main thread
		signalWaitList->add(signal);
}

void PCB::registerHandler(SignalID signal, SignalHandler handler) {
	if ((signal > 15) || (handler == 0)) return;
	// Handler for the signal 0 is registered in the PCB::mainThread's constructor
	if ((signal == 0) || (id == 1)) return;
	signalHandlers[signal]->add(handler);
}

void PCB::unregisterAllHandlers(SignalID id) {
	if ((id > 15) || (PCB::running->id == 1)) return;
	SignalHandler removedHandler = signalHandlers[id]->removeFirst();
	while (removedHandler != 0)
		removedHandler = signalHandlers[id]->removeFirst();
}

void PCB::swap(SignalID id, SignalHandler hand1, SignalHandler hand2) {
	if ((id > 15) || (hand1 == 0) || (hand2 == 0) || (PCB::running->id == 1)) return;
	unsigned foundFirstHandler = 0, foundSecondHandler = 0;
	List<SignalHandler>::Node *firstHandlerNode = signalHandlers[id]->first, *secondHandlerNode = signalHandlers[id]->first;
	while ((firstHandlerNode != 0) && (secondHandlerNode != 0)) {
		if (firstHandlerNode->data == hand1) { foundFirstHandler = 1; }
		if (secondHandlerNode->data == hand2) { foundSecondHandler = 1; }
		if ((foundFirstHandler == 1) && (foundSecondHandler == 1)) break;
		if (foundFirstHandler == 0) firstHandlerNode = firstHandlerNode->next;
		if (foundSecondHandler == 0) secondHandlerNode = secondHandlerNode->next;
	}
	if ((firstHandlerNode == 0) || (secondHandlerNode == 0)) return;
	else {
		SignalHandler tempHandler = firstHandlerNode->data;
		firstHandlerNode->data = secondHandlerNode->data;
		secondHandlerNode->data = tempHandler;
	}
}

void PCB::blockSignal(SignalID signal) {
	if ((signal > 15) || (id == 1)) return;
	signalBlockFlags[signal] = 1;
}

void PCB::blockSignalGlobally(SignalID signal) {
	if (signal > 15) return;
	PCB::globalSignalBlockFlags[signal] = 1;
}

void PCB::unblockSignal(SignalID signal) {
	if ((signal > 15) || (id == 1)) return;
	signalBlockFlags[signal] = 0;
}

void PCB::unblockSignalGlobally(SignalID signal) {
	if (signal > 15) return;
	PCB::globalSignalBlockFlags[signal] = 0;
}
/* === SIGNALS === */

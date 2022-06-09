# Overview
Kernel supports:
- Creating and running a thread for the given task.
- Context switching, which can happen synchronously (explicit yield) or asynchronously (an innterrupt or due to time sharing).
- Semaphores, for securing critical sections and conditional synchronization.
- Events (essentially, binary semaphores).
- Asynchronous thread signaling mechanism, where one thread can signal another to do something (i.e. kill itself, compute something, etc.).

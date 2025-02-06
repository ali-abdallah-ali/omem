OMEM C++ Lib

Author: ALI ABDALLAH

Description:
OMEM is a C++ library designed to enhance memory safety and management.
It provides robust mechanisms to detect out-of-bound memory reads and writes, ensuring safer and more reliable code execution.
Additionally, OMEM introduces an ownership model with configurable read and write permissions, offering fine-grained control over memory access.
Currently, the library supports Windows OS.

Code integration:
To integrate OMEM into an existing codebase, you can use the _OMEM_OP_OVERRIDE macro to override the new operator, enabling minimal changes to your code.
Alternatively, you can directly use omem_alloc and omem_free for memory allocation and deallocation. 
This flexibility allows you to choose the approach that best suits your project's requirements.

Out-of-Bounds Reads/Writes:
Memory allocated using OMEM is protected against out-of-bounds reads and writes.
Any attempt to access memory outside the allocated boundaries will trigger an exception, ensuring robust error detection and prevention of undefined behavior.

Lock and Unlock:
OMEM enhances memory safety with an ownership model that supports read-only and shared memory configurations.
Shared memory can be restricted to read operations unless explicitly unlocked.
For write operations, the memory must be unlocked using the provided API.
OMEM also includes a scoped unlock mechanism, which temporarily unlocks the memory and automatically re-locks it when the corresponding object goes out of scope, ensuring safe and controlled access.

Performance Pointers:
Unlike standard memory allocation, Performance Pointers take advantage of Windows’ ability to lock physical pages in memory,
ensuring critical data stays in RAM and never gets paged out. This leads to dramatically faster and more consistent access speeds,
perfect for high-performance computing, gaming, and real-time applications.

	Prerequisite: you’ll need to enable the "Lock Pages in Memory" policy in your Windows OS settings. 
	You can find detailed instructions on how to enable this setting here: 
	https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-10/security/threat-protection/security-policy-settings/lock-pages-in-memory

 OMEM © 2024 by ALI ABDALLAH is licensed under CC BY-NC-SA 4.0.
 To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
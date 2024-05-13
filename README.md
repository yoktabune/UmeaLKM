Linux Kernel Module Key-Value Store
This project is an implementation of a key-value store as a loadable kernel module (LKM) in the Linux kernel. The project was completed as part of a course to gain hands-on experience with different aspects of kernel programming.

Project Description
The project aimed to build a key-value store using kernel modules, divided into several modules, each focusing on specific functionalities:

Module 1: Key-Value
Implemented a basic key-value store functionality without considering synchronization for multiple threads. Demonstrated functionality by building multiple programs that read and write to the store.

Module 2: Add Synchronization
Enhanced the implementation to support synchronization for multiple processes, ensuring consistency while avoiding unnecessary blocking.

Module 3: Storing Data
Explored ways to store data securely, considering filesystem and memory management techniques.

Module 4: Remote Access
Extended the key-value store interface to allow access from multiple machines, evaluating options for implementing remote access from user or kernel space.

Module 5: Authentication
Implemented an authentication mechanism to control access to the key-value store, ensuring only authorized users can read and write data.

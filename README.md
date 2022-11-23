Name: Dobrica Nicoleta Adriana

321CA

# Homework nr 1

Organization
-
* This homework implements an executable loader for the ELF binary format files. This implementation, overall, loads the file into memory using a demand-paging mechanism. The structure of this program consists of a segment which holds valuable information to be processed and later used by the page fault handler, and to further know which page to load into memory. 

Implementation
-

* When a file is loaded into memory, a file descriptor is needed for later use. Since we use a demand-paging mechanism, we load the file page-by-page.
* When a page hasn't been mapped into memory yet, we check the part of the segment it exists in; if we have an invalid memory access (if it does not belong to any segment), then we raise the SIGSEGV signal. 
* In the handler function, we use the address that generated the page fault to search these pages. If the page has already been mapped and it still raised a page fault, then the segment it belongs to has invalid permissions. 
* If a page does belong to a segment but it has not been mapped yet, then we use the `mmap()` system call to create a memory mapping in the virtual address space of the process. To know which pages have been mapped into memory, we modify the data part of the segment structure, creating a semaphore-like array. 
Example: 
```cpp
// marking the mapped page
((int *)segm->data)[page_index] = 1;
```
* After a page has been mapped, we need to copy our data from the executable to the new mapped page with the help of the `lseek()` and `read()` system calls.
* At the end, we use the `munmap()` system call to remove the mapped pages from the process' virtual address space.



Bibliography
-

* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-04
* https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-06
* The Linux Programming Interface - chapters 21, 49, 50
* https://man7.org/linux/man-pages/


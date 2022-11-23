// SPDX-License-Identifier: ACS
/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "exec_parser.h"
#include <unistd.h>

static so_exec_t *exec;

static struct sigaction old_action;
static int fd;

// function that uses the old handler

void exec_old_action(int signum, siginfo_t *info, void *context) {
    old_action.sa_sigaction(signum, info, context);
    return;
}

// function that signals if a mapping has failed and uses the old handler

void map_failed_err(int signum, siginfo_t *info, void *context) {
    perror("Map failed");
    exec_old_action(signum, info, context);
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	// initializing a semaphore variable
	
	int flag = 0;
	int seg_size = exec->segments_no;

	for (int i = 0; i < seg_size; i++) {
		so_seg_t *segm = &((exec->segments)[i]);
		uintptr_t page_index = ((uintptr_t)info->si_addr - segm->vaddr) / getpagesize();
		uintptr_t si_addr_copy = (uintptr_t)info->si_addr;
		uintptr_t s_vaddr = (uintptr_t)segm->vaddr;
		uintptr_t seg_memsize = (uintptr_t)segm->mem_size;

		if (si_addr_copy >= s_vaddr && si_addr_copy <  seg_memsize + s_vaddr) {
			flag = 1;

			if (segm->data == NULL)
				segm->data = calloc(seg_memsize / getpagesize() + 1, sizeof(int));
			
			// if a page has already been mapped, we use the old handler

			if (((int *)segm->data)[page_index] == 1) {
				exec_old_action(signum, info, context);
			} else {
				
				// mapping the address of the page into memory

				char *addr = mmap((void *)(s_vaddr + page_index * getpagesize()), 
						getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
		 		if (addr == MAP_FAILED) {
					map_failed_err(signum, info, context);
				}
				
				// marking the mapped page

				((int *)segm->data)[page_index] = 1;
			 
				uintptr_t file_size = segm->file_size;

				uintptr_t seg_offset = segm->offset;

				uintptr_t last_addr = s_vaddr + (page_index + 1) * getpagesize() - 1;
				
				lseek(fd, getpagesize() * page_index + seg_offset, SEEK_SET);

				// adding the contents of the file from the mapped address
				
				if (last_addr < s_vaddr + file_size) 
					read(fd, (void *)addr, getpagesize());
				else if (last_addr >= s_vaddr + file_size && getpagesize() * page_index < file_size){
					read(fd, (void *)addr, file_size - getpagesize() * page_index);
				}
				
				mprotect(addr, getpagesize(), segm->perm);
			}
		}
	}
	if (flag == 0) {
		exec_old_action(signum, info, context);
	}
}

// function that signals whether sigaction has failed

int sigaction_err() {
    perror("sigaction");
    return -1;
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, &old_action);
	if (rc < 0) {
		sigaction_err();
	}
	return 0;
}

// function that signals that the path to file is invalid

int open_err() {
    perror("Could not open path to file.\n");
    return -1;
}

// function that unmaps the pages mapped from the segments

void munmap_exec() {
    for (int i = 0; i < exec->segments_no; i++) {
        so_seg_t *segm = &((exec->segments)[i]);
        uintptr_t no_pages = (uintptr_t)segm->mem_size / getpagesize();
        for (int j = 1; j < no_pages; j++) {
            if (((int *)segm->data)[j] == 1) {
                munmap((void *)segm->vaddr + j * getpagesize(), getpagesize());
            }
        }
    }
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	fd = open(path, O_RDONLY);
	if (!fd) {
		open_err();
	}

	if (!exec)
		return -1;

	so_start_exec(exec, argv);
	munmap_exec();

	return -1;
}
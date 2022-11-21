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
				old_action.sa_sigaction(signum, info, context);
				return;
			} else {
				
				// mapping the address of the page into memory

				char *addr = mmap((void *)(s_vaddr + page_index * getpagesize()), 
						getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
		 		if (addr == MAP_FAILED) {
					perror("Map failed");
					old_action.sa_sigaction(signum, info, context);
					return;
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
		old_action.sa_sigaction(signum, info, context);
		return;
	}
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
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	fd = open(path, O_RDONLY);
	if (!fd) {
		perror("Could not open path to file.\n");
		return -1;
	}

	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}

/* 
 * checkpoint.c
 * by WN @ Ap6. 06, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <interp/checkpoint.h>
#include <interp/mm.h>
#include <interp/logger.h>

#include <xasm/syscall.h>
#include <xasm/string.h>
#include <xasm/vsprintf.h>
#include <xasm/tls.h>
#include <xasm/types_helper.h>

static void
build_head(struct thread_private_data * tpd,
		struct checkpoint_head * head, struct pusha_regs * regs, void * eip)
{
	/* fill magic */
	memset(head->magic, '\0', CKPT_MAGIC_SZ);
	memcpy(head->magic, CKPT_MAGIC, sizeof(CKPT_MAGIC));

	/* argp_start */
	head->argp_first = tpd->argp_first;
	head->argp_last = tpd->argp_last;

	/* brk */
	head->brk = INTERNAL_SYSCALL_int80(brk, 1, 0);

	/* pid and tid */
	head->pid = tpd->pid;
	head->tid = tpd->tid;
	head->tnr = tpd->tnr;

	/* thread areas */
	for (int i = 0; i < GDT_ENTRY_TLS_ENTRIES; i++) {
		int err;
		head->thread_area[i].entry_number =
			GDT_ENTRY_TLS_MIN + i;
		err = INTERNAL_SYSCALL_int80(get_thread_area,
				1, &(head->thread_area[i]));
		assert(err == 0);
	}

	/* reg state */
	build_reg_state(&head->reg_state, regs, eip);
}

/* 
 * return value: start address of the next line.
 * if this is the last line, return NULL.
 */
static char *
read_procmem_line(char * line, struct mem_region * region, char ** p_fn)
{
	int err;
	int l = 0;
	char str_prot[5];
	err = sscanf(line, "%lx-%lx %4s %llx %*2x:%*2x %*d %n",
			(long unsigned int *)&region->start,
			(long unsigned int *)&region->end,
			str_prot,
			&region->offset,
			&l);

	/* set the prot */
	region->prot = 0;
	if (str_prot[0] == 'r')
		region->prot |= PROT_READ;
	if (str_prot[1] == 'w')
		region->prot |= PROT_WRITE;
	if (str_prot[2] == 'x')
		region->prot |= PROT_EXEC;
	/* str_prot[3] is 's' or 'p', we don't care 's'. when replay,
	 * all pages are 'p'. */

	/* find the mapped file name */
	*p_fn = line + l;
	if (*(*p_fn - 1) == '\n')
		*p_fn -= 1;

	/* I don't know why err == 4, but err is 4. */
	if (err != 4) {
		assert(err == 0);
		/* we meet the end of the file */
		return NULL;
	}

	/* if err != 0, then *p_fn must contain '\n' */
	/* eat the last '\n' in file name */
	char * eol = strchr(*p_fn, '\n');
	assert(eol != NULL);
	assert(*eol == '\n');
	*eol = '\0';
	/* plus a '\0' */
	region->fn_sz = strlen(*p_fn) + 1;
	return eol + 1;
}

static uint32_t
get_aval_size(const char * fn, uint64_t offset, uint32_t desire_size)
{
	if ((fn == NULL) || (fn[0] == '\0'))
		return desire_size;
	if (fn[0] == '[')
		return desire_size;

	/* stat the file */
	struct stat64 st;
	int err = INTERNAL_SYSCALL_int80(stat64, 2,
			fn, &st);
	if (err != 0) {
		WARNING(CKPT, "failed (%d) stat %s\n", err, fn);
		return desire_size;
	}

	/* check size */
	return (offset + desire_size > (uint64_t)(st.st_size)) ?
		(st.st_size - offset) :
		(desire_size);
}

static void
append_region(int fd, struct mem_region * region, const char * fn)
{
	int err;

	TRACE(CKPT, "append region: 0x%08x-0x%08x:0x%llx:%01x:%s\n", region->start,
			region->end, region->offset, region->prot, fn);

	/* write the region structure */
	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, region, sizeof(*region));
	assert(err == sizeof(*region));

	/* append file name */
	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, fn, region->fn_sz);
	assert(err == (int)region->fn_sz);

	/* write the memory image */
	/* if the memory is mapped from /dev/xxx, don't write it */
	if (strncmp("/dev", fn, 4) == 0) {
		/* append a '0' */
		int region_sz = 0;
		err = INTERNAL_SYSCALL_int80(write, 3,
				fd, &region_sz, sizeof(region_sz));
		assert(err == sizeof(region_sz));
		return;
	}

	/* stat target file */
	int region_sz = region->end - region->start;
	region_sz = get_aval_size(fn, region->offset, region_sz);
	uintptr_t tmp = region->start + region_sz + PAGE_SIZE - 1;
	tmp = tmp - (tmp % PAGE_SIZE);
	region_sz = tmp - region->start;

	TRACE(CKPT, "aval region_sz=%d\n", region_sz);

	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, &region_sz, sizeof(region_sz));
	assert(err == sizeof(region_sz));

	/* if the memory is unreadable, mark it readable then remark it back */
	if (!(region->prot & PROT_READ)) {
		err = INTERNAL_SYSCALL_int80(mprotect, 3,
				region->start, region_sz,
				region->prot | PROT_READ);
		assert(err == 0);
	}

	/* flush this memory region */
	assert(region->start % PAGE_SIZE == 0);
	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, region->start, region_sz);
	assert(err == region_sz);

	/* remark unreadable pages back */
	if (!(region->prot & PROT_READ)) {
		err = INTERNAL_SYSCALL_int80(mprotect, 3,
				region->start, region_sz,
				region->prot);
		assert(err == 0);
	}

	/* do some extra work for dump_init_stack */
	if (strncmp("[stack]", fn, sizeof("[stack]")) == 0) {
		struct thread_private_data * tpd = get_tpd();
		tpd->stack_top_addr = (void*)(region->end);
	}
}

static void
flush_mem_regions(int fd)
{
	int err;

	/* first, we alloc MAX_PROC_MAPS_FILE_SZ memory space to load
	 * /proc/self/maps.
	 * this file shouldn't larger than that */
	void * proc_map = NULL;
	proc_map = alloc_cont_space2(MAX_PROC_MAPS_FILE_SZ);
	assert(proc_map != NULL);
	memset(proc_map, '\0', MAX_PROC_MAPS_FILE_SZ);

	/* then we load /proc/self/maps */
	int proc_fd = INTERNAL_SYSCALL_int80(open, 2,
			"/proc/self/maps", O_RDONLY);
	assert(proc_fd > 0);

	err = INTERNAL_SYSCALL_int80(read, 3, proc_fd,
			proc_map, MAX_PROC_MAPS_FILE_SZ);
	if (err < 0)
		FATAL(CKPT, "read /proc/self/maps failed\n");
	if (!(err < MAX_PROC_MAPS_FILE_SZ))
		FATAL(CKPT, "memory map too complex, "
				"increase MAX_PROC_MAPS_FILE_SZ!\n");

#if 0
	if (err + 64 < FDPRINTF_MAX)
		TRACE(CKPT, "%s\n", (char*)proc_map);
#endif

	err = INTERNAL_SYSCALL_int80(close, 1, proc_fd);
	assert(err == 0);

	/* begin to process proc_map */
	struct mem_region region;
	char * mapped_fn;
	char * line = read_procmem_line(proc_map, &region, &mapped_fn);
	while (line != NULL) {
		/* append region */
		if ((region.start <= (uintptr_t)proc_map) &&
				(region.end > (uintptr_t)proc_map))
		{
			/* split this region:
			 * if the region can be merged with proc_map, it MUST be
			 * a anonymouse map. */
			assert(*mapped_fn == '\0');
			struct mem_region r1 = region;
			struct mem_region r2 = region;
			r1.start = region.start;	/* useless */
			r1.end = (uintptr_t)proc_map;
			if (r1.end - r1.start > 0)
				append_region(fd, &r1, mapped_fn);
			r2.start = (uintptr_t)(proc_map + MAX_PROC_MAPS_FILE_SZ);
			r2.end = region.end;	/* useless */
			/* needn't consider offser field because proc_map is in
			 * anon section*/
			if (r2.end - r2.start > 0)
				append_region(fd, &r2, mapped_fn);
		} else {
			append_region(fd, &region, mapped_fn);
		}
		line = read_procmem_line(line, &region, &mapped_fn);
	}

	/* write a struct mem_region which mark the end of regions */
	struct mem_region end_region = {
		.start = MEM_REGIONS_END_MARK,
		.end = MEM_REGIONS_END_MARK,
		.prot = 0,
		.offset = 0,
		.fn_sz = 0,
	};
	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, &end_region, sizeof(end_region));
	assert(err == sizeof(end_region));

	/* write the ckpt end mark */
	uint32_t end_mark = CKPT_END_MARK;
	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, &end_mark, sizeof(uint32_t));
	assert(err == sizeof(uint32_t));

	/* free the mmap storages */
	free_cont_space2(proc_map, MAX_PROC_MAPS_FILE_SZ);
}


/* the basic checkpointing, no fork */
/* the fork based checkpoint should unmap all logger pages
 * and codecache pages, if not, 
 * size of checkpoint will become very large */
/* do_make_checkpoint unmap all unneeded pages */
static void
do_make_checkpoint(struct pusha_regs * regs, void * eip)
{

#if 0
	WARNING(COMPILER, "ckpt: *(void**)(0xb7fd0799)=%p\n",
			*(void**)(0xb7fd0799));
	WARNING(COMPILER, "ckpt: *(void**)(0xb7fc7914)=%p\n",
			*(void**)(0xb7fc7914));
#endif

	assert(sizeof(struct checkpoint_head) < 0x2000);
	struct thread_private_data * tpd = get_tpd();
	struct tls_logger * logger = &tpd->logger;

	unmap_tpds_pages();

	/* first, create the checkpoint file */
	int err;
	int fd = INTERNAL_SYSCALL_int80(open, 3,
			logger->ckpt_fn, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	if (fd <= 0)
		FATAL(CKPT, "create ckeckpoint file %s failed: %d\n",
				logger->ckpt_fn, fd);

	struct checkpoint_head head;
	build_head(tpd, &head, regs, eip);

	/* flush head */
	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, &head, sizeof(head));
	assert(err == sizeof(head));

	/* flush mem region */
	flush_mem_regions(fd);

	err = INTERNAL_SYSCALL_int80(close, 1, fd);
	assert(err == 0);


	if (strcmp(logger->ckpt_fn +
				strlen(logger->ckpt_fn) +
				- 4, "dead") == 0)
		return;

	/* create init stack file */
	fd = INTERNAL_SYSCALL_int80(open, 3,
			logger->init_stack_fn, O_WRONLY|O_CREAT|O_TRUNC, 0664);
	if (fd <= 0)
		FATAL(CKPT, "create init stack file %s failed: %d\n",
				logger->init_stack_fn, fd);
	/* it should have beed inited when we append_region(stack) */
	assert(tpd->stack_top_addr != NULL);

	size_t init_stack_size =
		(uintptr_t)(tpd->stack_top_addr) -
		(uintptr_t)(tpd->argp_first);
	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, &init_stack_size, sizeof(init_stack_size));
	assert(err == sizeof(init_stack_size));

	err = INTERNAL_SYSCALL_int80(write, 3,
			fd, tpd->argp_first, init_stack_size);
	assert(err == (int)init_stack_size);

	err = INTERNAL_SYSCALL_int80(close, 1, fd);
	assert(err == 0);
}

void
fork_make_checkpoint(struct pusha_regs * regs, void * eip)
{

	/* don't use normal fork: child become defunct process after exit */
	int pid;
#define CLONE_PARENT 0x00008000
	pid = INTERNAL_SYSCALL_int80(clone, 5,
			CLONE_PARENT, 0, NULL, NULL, NULL);
	assert(pid >= 0);

	if (pid == 0) {
		unset_share_context();
		do_make_checkpoint(regs, eip);
		__exit(0);
	}
}

void
make_checkpoint(struct pusha_regs * regs, void * eip)
{
	/* cleanup all thread's tls pages, then
	 * call do_make_checkpoint */
	do_make_checkpoint(regs, eip);
}

void
make_dead_checkpoint(struct pusha_regs * regs, void * eip)
{
	reset_ckpt_log_names(TRUE);
	fork_make_checkpoint(regs, eip);
}


// vim:ts=4:sw=4


/* 
 * configure.c
 * by WN @ Aug. 03, 2010
 */

#include <interp/configure.h>

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/assert.h>

#include <xasm/tls.h>
#include <xasm/string.h>

static int
_atoi(const char * x)
{
	int s = 0;
	if (x == NULL)
		return s;

	while ((*x <= '9') && (*x >= '0')) {
		s *= 10;
		s += *x++ - '0';
	}

	return s;
}

static void
reset_all_conf(struct thread_private_data * tpd)
{
#define bool_conf(a, b, def)		tpd->b = def;
#define int_conf(a, b, def)		tpd->b = def;
#include <interp/confentries.h>
#undef bool_conf
#undef int_conf
}

#define cmp_get(___key, ___str) ({const char * ___res;	\
		if (strncmp(___key "=", ___str, sizeof(___key "=") - 1) == 0) { \
			___res = ___str + sizeof(___key "=") - 1;		\
		} else {	\
			___res = NULL;	\
		}			\
		___res;		\
})

static void
print_all_conf(struct thread_private_data * tpd)
{
#define bool_conf(a, b, c)	VERBOSE(LOADER, a " = %s\n", (tpd->b) ? "true" : "false");
#define int_conf(a, b, c)	VERBOSE(LOADER, a " = %d\n", tpd->b);
#include <interp/confentries.h>
#undef bool_conf
#undef int_conf
}

static void
check_env(struct thread_private_data * tpd, const char * entry)
{
	assert(entry != NULL);
	assert(tpd != NULL);
#define bool_conf(___key, ___item, __def) do {\
	const char * val;\
	if ((val = cmp_get(___key, entry))) {\
		if (strncmp(val, "1", 1) == 0)\
			tpd->___item = TRUE;\
	}\
} while(0);

#define int_conf(___key, ___item, __def) do {\
	const char * val;\
	if ((val = cmp_get(___key, entry))) {\
		tpd->___item = _atoi(val);\
	}\
} while(0);



#include <interp/confentries.h>
#undef bool_conf
#undef int_conf
}

void
read_conf(void * esp)
{
	struct thread_private_data * tpd = get_tpd();

	reset_all_conf(tpd);

	/* get env */
	uintptr_t * p = esp;
	p ++;
	while (*p != 0) {
		p ++;
	}
	p ++;
	while (*p != 0) {
		const char * entry = (const char *)(*p);
		TRACE(LOADER, "env: %s\n", (const char *)(entry));

		check_env(tpd, entry);

		p ++;
	}
	print_all_conf(tpd);
	return;
}

// vim:ts=4:sw=4


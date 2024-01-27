#ifndef _BACKPORT_LINUX_STRING_HELPERS_H_
#define _BACKPORT_LINUX_STRING_HELPERS_H_
#include <linux/string.h>
#include_next <linux/string_helpers.h>

#ifdef BPM_CONSOLIDATE_STRING_HELPERS_NOT_PRESENT
static inline const char *str_yes_no(bool v)
{
	return v ? "yes" : "no";
}

static inline const char *str_on_off(bool v)
{
	return v ? "on" : "off";
}

static inline const char *str_enable_disable(bool v)
{
	return v ? "enable" : "disable";
}

static inline const char *str_enabled_disabled(bool v)
{
	return v ? "enabled" : "disabled";
}
#endif

#endif

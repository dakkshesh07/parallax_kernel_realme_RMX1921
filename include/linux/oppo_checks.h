#ifndef __INCLUDE_LINUX_OPPO_CHECKS_H
#define __INCLUDE_LINUX_OPPO_CHECKS_H
static __maybe_unused bool on_ambient_screen;
static __maybe_unused bool call_status;
bool q6_call_status(void);
bool device_is_dozing(void);
#endif 

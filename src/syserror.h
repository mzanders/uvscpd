#ifndef _SYSERROR_H_
#define _SYSERROR_H_


void SysError(const char * Module, const char * Fct);
#define SysMError(a)  SysError(ModuleName, a)
void NonSysError(const char * Module, const char * Fct);

#endif /* _SYSERROR_H_ */

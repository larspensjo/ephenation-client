#pragma once

#ifdef DEBUG
extern void LPLog(const char *func, const char *file, int line, const char *fmt, ...);
#define LPLOG(args...) LPLog(__FUNCTION__, __FILE__, __LINE__, args)
#else
#define LPLOG(...)
#endif

#include "common.h"
#include <cctype>
#include <stdio.h>
#include <cstdarg>


// Compatibility: snprintf is not implemented before VC2015
#if defined(_MSC_VER)
#if _MSC_VER < 1800

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#elif _MSC_VER < 1900

#define snprintf _snprintf

#endif // _MSC_VER < 1800
#endif // _MSC_VER


std::string VersionString(int nVer)
{
    int nMajor = nVer / 1000000;
    int nMinor = (nVer / 1000) % 1000;
    int nPatch = nVer % 1000;

    char szBuff[128];
    snprintf(szBuff, sizeof(szBuff), "%d.%d.%d", nMajor, nMinor, nPatch);
    szBuff[sizeof(szBuff) - 1] = '\0';

    return szBuff;
}


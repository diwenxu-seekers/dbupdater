#ifndef COMMON_H
#define COMMON_H

#include <list>
#include <string>

// Application name
#define APPLICATION_NAME "DBUpdater"

// Version number in the form of major * 10^6 + minor * 10^3 + patch

#if defined(DBUPDATER_VERSION_1)
#define APPLICATION_VERSION 1007004
#else
#define APPLICATION_VERSION 2000001
#endif

// Organization name
#define ORGANIZATION_NAME "Win Wind Securities Ltd."

// Copyright
#define COPYRIGHTS_YEAR "2015"
#define COPYRIGHTS_STATEMENT "ALL RIGHTS RESERVED."

// Helper functions

std::string VersionString(int nVer);

#endif // COMMON_H

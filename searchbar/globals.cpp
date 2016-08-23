#include "globals.h"

QMutex Globals::_mutex;
std::unordered_map<std::string, int> Globals::database_lastrowid;
std::unordered_map<std::string, std::string> Globals::database_lastdatetime;
std::unordered_map<std::string, Globals::dbupdater_info> Globals::dbupdater_lastinfo;

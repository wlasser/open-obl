#include <sstream>
#include <iomanip>
#include <string>
#include "system_time.hpp"

std::string systemTimeToISO8601(const struct SystemTime &systemTime) {
  std::stringstream s;
  s << systemTime.year << '-'
    << std::setfill('0') << std::setw(2) << systemTime.month << '-'
    << std::setfill('0') << std::setw(2) << systemTime.day << 'T'
    << std::setfill('0') << std::setw(2) << systemTime.hour << ':'
    << std::setfill('0') << std::setw(2) << systemTime.minute << ':'
    << std::setfill('0') << std::setw(2) << systemTime.second;
  return s.str();
}

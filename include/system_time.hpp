#ifndef SYSTEM_TIME_HPP
#define SYSTEM_TIME_HPP

#include <sstream>
#include <iomanip>

struct SystemTime {
  uint16_t year;
  uint16_t month;
  uint16_t dayOfWeek;
  uint16_t day;
  uint16_t hour;
  uint16_t minute;
  uint16_t second;
  uint16_t millisecond;
};
static const std::size_t SystemTimeSize = 8 * sizeof(uint16_t);

std::string systemTimeToISO8601(const struct SystemTime &);

#endif /* SYSTEM_TIME_HPP */

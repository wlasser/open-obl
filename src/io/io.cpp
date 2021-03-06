#include "io/io.hpp"

std::string io::decodeIosState(std::ios_base::iostate state) {
  std::string stateString;
  // goodbit == 0 so none of the other bits can be set if goodbit is.
  if (state == std::ios::goodbit) return "goodbit";
  if (state & std::ios::badbit) stateString += "|badbit";
  if (state & std::ios::failbit) stateString += "|failbit";
  if (state & std::ios::eofbit) stateString += "|eofbit";
  if (stateString.empty()) return "unknown";
  else if (stateString[0] == '|')
    return stateString.substr(1, std::string::npos);
  else return stateString;
}

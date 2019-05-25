#include "config/globals.hpp"
#include "script_functions.hpp"

float script::GetCurrentTime() {
  return oo::Globals::getSingleton().fGet("GameHour");
}
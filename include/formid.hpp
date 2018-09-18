#ifndef OPENOBLIVION_FORMID_HPP
#define OPENOBLIVION_FORMID_HPP

#include <cstdint>
#include <string>

using FormID = uint32_t;
using IRef = uint32_t;

std::string formIDString(FormID formID);

#endif // OPENOBLIVION_FORMID_HPP

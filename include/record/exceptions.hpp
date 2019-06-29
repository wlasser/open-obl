#ifndef OPENOBL_RECORD_EXCEPTIONS_HPP
#define OPENOBL_RECORD_EXCEPTIONS_HPP

#include <stdexcept>
#include <string_view>

namespace record {

struct RecordNotFoundError : virtual std::runtime_error {
  RecordNotFoundError(std::string_view expected, std::string_view actual) :
      std::runtime_error(std::string("Expected ")
                             .append(expected)
                             .append(", found ")
                             .append(actual)) {}
};

} // namespace record

#endif //OPENOBL_RECORD_EXCEPTIONS_HPP

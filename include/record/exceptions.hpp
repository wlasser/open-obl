#ifndef OPENOBL_RECORD_EXCEPTIONS_HPP
#define OPENOBL_RECORD_EXCEPTIONS_HPP

#include <boost/format.hpp>
#include <stdexcept>
#include <string_view>

namespace record {

struct RecordReadError : virtual std::runtime_error {
  explicit RecordReadError(std::string_view recordType) :
      std::runtime_error(boost::str(
          boost::format("Failed to read %s record") % recordType)) {}
};

struct RecordNotFoundError : virtual std::runtime_error {
  RecordNotFoundError(std::string_view expected, std::string_view actual) :
      std::runtime_error(boost::str(
          boost::format("Expected %s, found %s") % expected % actual)) {}
};

} // namespace record

#endif //OPENOBL_RECORD_EXCEPTIONS_HPP

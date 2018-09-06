#ifndef OPENOBLIVION_RECORD_EXCEPTIONS_HPP
#define OPENOBLIVION_RECORD_EXCEPTIONS_HPP

#include <boost/format.hpp>
#include <stdexcept>
#include <string>

namespace record {

struct RecordReadError : virtual std::runtime_error {
  explicit RecordReadError(const std::string &recordType) :
      std::runtime_error(boost::str(
          boost::format("Failed to read %s record") % recordType)) {}
};

struct RecordNotFoundError : virtual std::runtime_error {
  RecordNotFoundError(const std::string &expected, const std::string &actual) :
      std::runtime_error(boost::str(
          boost::format("Expected %s, found %s") % expected % actual)) {}
};

} // namespace record

#endif //OPENOBLIVION_RECORD_EXCEPTIONS_HPP

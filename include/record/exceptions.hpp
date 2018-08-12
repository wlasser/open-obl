#ifndef OPENOBLIVION_EXCEPTIONS_HPP
#define OPENOBLIVION_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace record {

class RecordReadError : public std::runtime_error {
 public:
  explicit RecordReadError(const std::string &recordType) :
      std::runtime_error(std::string("Failed to read ") + recordType
                             + std::string("record")) {}
};

class RecordNotFoundError : public std::runtime_error {
 public:
  RecordNotFoundError(const std::string &expected, const std::string &actual) :
      std::runtime_error(std::string("Expected ") + expected + ", found "
                             + std::string(actual)) {}
};

} // namespace record

#endif //OPENOBLIVION_EXCEPTIONS_HPP

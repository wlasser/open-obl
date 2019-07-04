#ifndef OPENOBL_RECORD_DEFINITION_HELPERS_HPP
#define OPENOBL_RECORD_DEFINITION_HELPERS_HPP

// Ugly macros for assisting with defining records and subrecords.

// Outside of a class, the standard requires that both the base function
// template and any full specializations be declared before they
// are instantiated. Forgive the macro, it saves a lot of typing.
// Yes `DECLARE_SPECIALIZED_RECORD` is a technically a misnomer, but
// it's not inaccurate, just incomplete.
// This should be called like a function, with a terminating semicolon;
// the semicolon has deliberately been left off the last statement to enforce
// this, since otherwise g++ complains about duplicate semicolons under
// -Wpedantic.
#define DECLARE_SPECIALIZED_RECORD_IO(type) \
template<> struct raw::SizedBinaryIo<raw::type> { \
  static void writeBytes(std::ostream &, const raw::type &, std::size_t); \
  static void readBytes(std::istream &, raw::type &, std::size_t); \
}

#define DECLARE_SPECIALIZED_RECORD(type) \
  template <> std::size_t type::size() const; \
  DECLARE_SPECIALIZED_RECORD_IO(type)

#define DECLARE_SPECIALIZED_SUBRECORD(type) \
  namespace raw { \
  template <> struct SubrecordSize<type> { \
    std::size_t operator()(const type &) const; \
    }; } \
  DECLARE_SPECIALIZED_RECORD_IO(type) \

#endif // OPENOBL_RECORD_DEFINITION_HELPERS_HPP

#ifndef OPENOBLIVION_RECORD_DEFINITION_HELPERS_HPP
#define OPENOBLIVION_RECORD_DEFINITION_HELPERS_HPP

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
#define DECLARE_SPECIALIZED_RECORD_WITH_SIZE(size_type, type) \
  template <> \
  size_type type::size() const; \
  template <> \
  std::ostream& raw::write(std::ostream&, const raw::type&, std::size_t); \
  template <> \
  std::istream& raw::read(std::istream&, raw::type&, std::size_t)

#define DECLARE_SPECIALIZED_RECORD(type) \
  DECLARE_SPECIALIZED_RECORD_WITH_SIZE(uint32_t, type)

#define DECLARE_SPECIALIZED_SUBRECORD(type) \
  DECLARE_SPECIALIZED_RECORD_WITH_SIZE(uint16_t, type)

#endif // OPENOBLIVION_RECORD_DEFINITION_HELPERS_HPP

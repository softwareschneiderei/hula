#pragma once
#include <string>

enum class value_type
{
  void_t,
  bool_t,
  int32_t,
  float_t,
  double_t,
  string_t,
  image8_t,
  image16_t,
};

struct value_type_info
{
  value_type key;
  char const* tango_type_enum;
  char const* tango_type;
  char const* cpp_type;
  char const* cpp_parameter_list;
  char const* input_type;
};

// forward lookup
value_type_info const& info_for(value_type v);
char const* tango_type_enum(value_type v);
const char* tango_type(value_type v);
const char* cpp_type(value_type v);
const char* cpp_parameter_list(value_type v);

// reverse lookup
value_type from_input_type(std::string const& v);

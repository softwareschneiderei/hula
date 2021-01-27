#pragma once
#include <string>

// TODO: the _t tag is very unusual here. It is normally used for types. Should switch to _v or something like that. 
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

enum class attribute_rank_t
{
  scalar,
  spectrum,
  image
};

struct name_type_info_t
{
  value_type key;
  char const* toml_type;
};

struct scalar_type_info_t
{
  value_type key;
  char const* tango_type_enum;
  char const* tango_type;
  char const* cpp_type;
  char const* cpp_parameter_list;
};

struct array_type_info_t
{
  value_type element_type;
  char const* tango_type_enum;
  char const* tango_type;
  char const* cpp_type;
  char const* cpp_parameter_list;
};

// forward lookup
scalar_type_info_t const& scalar_info_for(value_type v);
array_type_info_t const& array_info_for(value_type v);

char const* tango_type_enum(value_type v, bool is_array);
const char* tango_type(value_type v, bool is_array);
const char* cpp_type(value_type v, bool is_array);
const char* cpp_parameter_list(value_type v, bool is_array);

std::string_view name_for(value_type v);

// reverse lookup
value_type from_input_type(std::string_view const& v);

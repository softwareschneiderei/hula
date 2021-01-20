#include "types.hpp"
#include <unordered_map>
#include <stdexcept>
#include <fmt/format.h>

namespace {

constexpr name_type_info_t name_table[] = {
  // enum, toml-type
  {value_type::void_t,"void"},
  {value_type::bool_t, "bool"},
  {value_type::int32_t,"int32"},
  {value_type::float_t, "float" },
  {value_type::double_t, "double" },
  {value_type::string_t, "string" },
  {value_type::image8_t, "image/8" },
  {value_type::image16_t, "image/16" }
};

// See https://tango-controls.readthedocs.io/en/latest/development/device-api/device-server-writing.html?exchanging-data-between-client-and-server#exchanging-data-between-client-and-server
// For the mapping of tango types to c++ types
constexpr scalar_type_info_t scalar_table[] = {
  // enum, tango-enum, tango-type, cpp-return-type, cpp-parameter-list, toml-type
  {value_type::void_t, "Tango::DEV_VOID", nullptr, "void", ""},
  {value_type::bool_t, "Tango::DEV_BOOLEAN", "Tango::DevBoolean", "bool", "bool rhs"},
  {value_type::int32_t, "Tango::DEV_LONG", "Tango::DevLong", "std::int32_t", "std::int32_t rhs"},
  {value_type::float_t, "Tango::DEV_FLOAT", "Tango::DevFloat", "float", "float rhs" },
  {value_type::double_t, "Tango::DEV_DOUBLE", "Tango::DevDouble", "double", "double rhs" },
  // TODO: would be nice to use std::string_view instead here, but tango 9.3.3 does not support C++17 on windows yet (due to usage of std::binary_function etc..)
  {value_type::string_t, "Tango::DEV_STRING", "Tango::DevString", "std::string", "std::string const& rhs" },
  {value_type::image8_t, "Tango::DEV_ENCODED", "Tango::EncodedAttribute", "image<std::uint8_t>", "image<std::uint8_t> const& rhs" },
  {value_type::image16_t, "Tango::DEV_ENCODED", "Tango::EncodedAttribute", "image<std::uint16_t>", "image<std::uint16_t> const& rhs" }
};


constexpr array_type_info_t array_table[] = {
  // enum, tango-enum, tango-type, cpp-return-type, cpp-parameter-list, toml-type
  {value_type::bool_t, "Tango::DEVVAR_BOOLEANARRAY", "Tango::DevVarBooleanArray", "std::vector<bool>", "std::vector<bool> const& rhs"},
  {value_type::int32_t, "Tango::DEVVAR_LONGARRAY", "Tango::DevVarLongArray", "std::vector<std::int32_t>", "std::vector<std::int32_t> const& rhs"},
  {value_type::float_t, "Tango::DEVVAR_FLOATARRAY", "Tango::DevVarFloatArray", "std::vector<float>", "std::vector<float> const& rhs" },
  {value_type::double_t, "Tango::DEVVAR_DOUBLEARRAY", "Tango::DevVarDoubleArray", "std::vector<double>", "std::vector<double> const& rhs" },
  {value_type::string_t, "Tango::DEVVAR_STRINGARRAY", "Tango::DevVarStringArray", "std::vector<std::string>", "std::vector<std::string> const& rhs" },
};

using scalar_map_t = std::unordered_map<value_type, scalar_type_info_t const*>;
using array_map_t = std::unordered_map<value_type, array_type_info_t const*>;
using enum_map_t = std::unordered_map<std::string_view, value_type>;
using name_map_t = std::unordered_map<value_type, std::string_view>;

scalar_map_t create_scalar_map()
{
  scalar_map_t new_map;
  for (auto const& each : scalar_table)
  {
    new_map.insert(std::make_pair(each.key, &each));
  }
  return new_map;
}

array_map_t create_array_map()
{
  array_map_t new_map;
  for (auto const& each : array_table)
  {
    new_map.insert(std::make_pair(each.element_type, &each));
  }
  return new_map;
}

enum_map_t create_enum_map()
{
  enum_map_t new_map;
  for (auto const& each : name_table)
  {
    new_map.insert(std::make_pair(each.toml_type, each.key));
  }
  return new_map;
}

name_map_t create_name_map()
{
  name_map_t new_map;
  for (auto const& each : name_table)
  {
    new_map.insert(std::make_pair(each.key, each.toml_type));
  }
  return new_map;
}


scalar_map_t const scalar_map = create_scalar_map();
array_map_t const array_map = create_array_map();
enum_map_t const enum_map = create_enum_map();
name_map_t const name_map = create_name_map();

}

scalar_type_info_t const& scalar_info_for(value_type v)
{
  auto found = scalar_map.find(v);
  if (found == scalar_map.end())
    throw std::invalid_argument("Unsupported type");
  return *found->second;
}

value_type from_input_type(std::string_view const& v)
{
  auto found = enum_map.find(v);
  if (found == enum_map.end())
    throw std::invalid_argument(fmt::format("Unsupported input type: {0}", v));
  return found->second;
}

char const* tango_type_enum(value_type v)
{
  return scalar_info_for(v).tango_type_enum;
}

std::string_view name_for(value_type v)
{
  return name_map.at(v);
}

const char* tango_type(value_type v)
{
  auto const& info = scalar_info_for(v);
  auto result = info.tango_type;
  if (result == nullptr)
  {
    throw std::invalid_argument(fmt::format("Type {0} has no corresponding tango type", name_for(v)));
  }
  return result;
}

const char* cpp_type(value_type v)
{
  return scalar_info_for(v).cpp_type;
}

const char* cpp_parameter_list(value_type v)
{
  return scalar_info_for(v).cpp_parameter_list;
}

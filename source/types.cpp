#include "types.hpp"
#include <unordered_map>
#include <stdexcept>
#include <fmt/format.h>

namespace {
  
// See https://tango-controls.readthedocs.io/en/latest/development/device-api/device-server-writing.html?exchanging-data-between-client-and-server#exchanging-data-between-client-and-server
// For the mapping of tango types to c++ types
constexpr value_type_info mapping[] = {
  // enum, tango-enum, tango-type, cpp-return-type, cpp-parameter-list, toml-type
  {value_type::void_t, "Tango::DEV_VOID", nullptr, "void", "", "void"},
  {value_type::bool_t, "Tango::DEV_BOOLEAN", "Tango::DevBoolean", "bool", "bool rhs", "bool"},
  {value_type::int32_t, "Tango::DEV_LONG", "Tango::DevLong", "std::int32_t", "std::int32_t rhs", "int32"},
  {value_type::float_t, "Tango::DEV_FLOAT", "Tango::DevFloat", "float", "float rhs", "float" },
  {value_type::double_t, "Tango::DEV_DOUBLE", "Tango::DevDouble", "double", "double rhs", "double" },
  // TODO: would be nice to use std::string_view instead here, but tango 9.3.3 does not support C++17 on windows yet (due to usage of std::binary_function etc..)
  {value_type::string_t, "Tango::DEV_STRING", "Tango::DevString", "std::string", "std::string const& rhs", "string" },
  {value_type::image8_t, "Tango::DEV_ENCODED", "Tango::EncodedAttribute", "image<std::uint8_t>", "image<std::uint8_t> const& rhs", "image/8" },
  {value_type::image16_t, "Tango::DEV_ENCODED", "Tango::EncodedAttribute", "image<std::uint16_t>", "image<std::uint16_t> const& rhs", "image/16" }
};

}

value_type_info const& info_for(value_type v)
{
  using map_type = std::unordered_map<value_type, value_type_info const*>;
  static map_type lookup;
  if (lookup.empty())
  {
    map_type new_map;
    for (auto const& each : mapping)
    {
      new_map.insert(std::make_pair(each.key, &each));

    }
    lookup = std::move(new_map);
  }
  auto found = lookup.find(v);
  if (found == lookup.end())
    throw std::invalid_argument("Unsupported type");
  return *found->second;
}

value_type from_input_type(std::string const& v)
{
  auto matches = [&](value_type_info const& info)
  {
    return v == info.input_type;
  };
  auto found = std::find_if(std::begin(mapping),
    std::end(mapping), matches);

  if (found == std::end(mapping))
    throw std::invalid_argument(fmt::format("Unsupported input type: {0}", v));
  return found->key;
}

char const* tango_type_enum(value_type v)
{
  return info_for(v).tango_type_enum;
}

const char* tango_type(value_type v)
{
  auto const& info = info_for(v);
  auto result = info.tango_type;
  if (result == nullptr)
  {
    throw std::invalid_argument(fmt::format("Type {0} has no corresponding tango type", info.input_type));
  }
  return result;
}

const char* cpp_type(value_type v)
{
  return info_for(v).cpp_type;
}

const char* cpp_parameter_list(value_type v)
{
  return info_for(v).cpp_parameter_list;
}

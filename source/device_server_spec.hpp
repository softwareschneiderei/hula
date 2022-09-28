#pragma once
#include <toml11/toml.hpp>
#include <fmt/format.h>
#include "uncased_name.hpp"
#include "types.hpp"

enum class access_type
{
  read_only,
  write_only,
  read_write,
};

enum class display_level_t
{
  operator_level,
  expert_level,
};

inline bool is_readable(access_type rhs)
{
  switch (rhs)
  {
  case access_type::read_only:
  case access_type::read_write:
    return true;
  default:
    return false;
  }
}

inline bool is_writable(access_type rhs)
{
  switch (rhs)
  {
  case access_type::write_only:
  case access_type::read_write:
    return true;
  default:
    return false;
  }
}

namespace toml
{
  template <typename T>
  auto find_or(toml::value const& v, std::string const& key, T default_result = {}) -> T
  {
    if (!v.contains(key)) return default_result;

    return toml::find<T>(v, key);
  }

  template<>
  struct from<value_type>
  {
    static value_type from_toml(value const& v)
    {
      auto const& code = v.as_string();
      return from_input_type(static_cast<std::string const&>(code));
    }
  };

  template<>
  struct from<access_type>
  {
    static access_type from_toml(value const& v);
  };

  template<>
  struct from<display_level_t>
  {
    static display_level_t from_toml(value const& v);
  };
}

struct device_property
{
  device_property() = default;
  explicit device_property(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , type(toml::find<value_type>(v, "type"))
  {
  }

  uncased_name name;
  value_type type = value_type::void_t;
};

struct attribute_type_t
{
  attribute_type_t() = default;
  explicit attribute_type_t(toml::value const& v);
  explicit attribute_type_t(std::string const& v);

  value_type type = value_type::void_t;
  attribute_rank_t rank = attribute_rank_t::scalar;
  std::array<std::uint32_t, 2> max_size{};
};

struct command_type_t
{
  command_type_t() = default;
  explicit command_type_t(toml::value const& v);
  explicit command_type_t(std::string const& v);

  value_type type = value_type::void_t;
  bool is_array = false;
};

struct attribute
{
  attribute() = default;
  explicit attribute(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , type(toml::find<attribute_type_t>(v, "type"))
  , access(toml::find_or<access_type>(v, "access", access_type::read_only))
  , min_value(toml::find_or<std::string>(v, "min_value", ""))
  , max_value(toml::find_or<std::string>(v, "max_value", ""))
  , unit(toml::find_or<std::string>(v, "unit", ""))
  , display_level(toml::find_or<display_level_t>(v, "display_level", display_level_t::operator_level))
  {
  }

  uncased_name name;
  attribute_type_t type;
  access_type access = access_type::read_only;
  std::string min_value;
  std::string max_value;
  std::string unit;
  display_level_t display_level = display_level_t::operator_level;
};

struct command
{
  command() = default;
  explicit command(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , return_type(toml::find<command_type_t>(v, "return_type"))
  , parameter_type(toml::find<command_type_t>(v, "parameter_type"))
  {
  }

  uncased_name name;
  command_type_t return_type;
  command_type_t parameter_type;
};

struct raw_device_server_spec
{
  explicit raw_device_server_spec(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , device_properties(toml::find_or<std::vector<device_property>>(v, "device_properties"))
  , attributes(toml::find_or<std::vector<attribute>>(v, "attributes"))
  , commands(toml::find_or<std::vector<command>>(v, "commands"))
  {
  }

  uncased_name name;
  std::vector<device_property> device_properties;
  std::vector<attribute> attributes;
  std::vector<command> commands;
};

struct device_server_spec : raw_device_server_spec
{
  explicit device_server_spec(toml::value const& v)
  : device_server_spec(raw_device_server_spec(v))
  {
  }

  explicit device_server_spec(raw_device_server_spec const& raw)
  : raw_device_server_spec(raw)
  {
    device_properties_name = fmt::format("{0}_properties", name.snake_cased());
    base_name = fmt::format("{0}_base", name.snake_cased());
    ds_name = fmt::format("{0}TangoAdaptor", name.camel_cased());
    ds_class_name = fmt::format("{0}TangoClass", name.camel_cased());
    header_name = fmt::format("hula_{0}.hpp", name.snake_cased());
    grouping_namespace_name = name.snake_cased();
  }

  std::string device_properties_name;
  std::string base_name;
  std::string ds_name;
  std::string ds_class_name;
  std::string header_name;
  std::string grouping_namespace_name;
};

// Facades for the type lookup
inline char const* tango_type(command_type_t const& type)
{
  return tango_type(type.type, type.is_array);
}

inline char const* tango_type_enum(command_type_t const& type)
{
  return tango_type_enum(type.type, type.is_array);
}

inline char const* cpp_type(command_type_t const& type)
{
  return cpp_type(type.type, type.is_array);
}

inline char const* cpp_parameter_list(command_type_t const& type)
{
  return cpp_parameter_list(type.type, type.is_array);
}

inline char const* tango_type(attribute_type_t const& type)
{
  return tango_type(type.type, type.rank != attribute_rank_t::scalar);
}

inline char const* tango_type_enum(attribute_type_t const& type)
{
  return tango_type_enum(type.type, type.rank != attribute_rank_t::scalar);
}

inline std::string cpp_type(attribute_type_t const& type)
{
  if (type.rank == attribute_rank_t::image)
  {
    return fmt::format("image<{0}>", cpp_type(type.type, false));
  }
  return cpp_type(type.type, type.rank != attribute_rank_t::scalar);
}

inline std::string cpp_parameter_list(attribute_type_t const& type)
{
  if (type.rank == attribute_rank_t::image)
  {
    return fmt::format("image<{0}> const& rhs", cpp_type(type.type, false));
  }
  return cpp_parameter_list(type.type, type.rank != attribute_rank_t::scalar);
}

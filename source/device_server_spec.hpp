#include <toml11/toml.hpp>
#include <fmt/format.h>
#include "name.hpp"

enum class value_type
{
  void_t,
  long_t,
  float_t,
  string_t,
};

enum class access_type
{
  read_only,
  write_only,
  read_write,
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
  template<>
  struct from<value_type>
  {
    static value_type from_toml(value const& v)
    {
      auto code = v.as_string();
      if (code == "void")
      {
        return value_type::void_t;
      }
      if (code == "long")
      {
        return value_type::long_t;
      }
      if (code == "float")
      {
        return value_type::float_t;
      }
      if (code == "string")
      {
        return value_type::string_t;
      }
      throw std::invalid_argument("Unknown type code: " + code.str);
    }
  };

  template<>
  struct from<access_type>
  {
    static access_type from_toml(value const& v);
  };
}

struct device_property
{
  device_property() = default;
  device_property(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , type(toml::find<value_type>(v, "type"))
  {
  }

  name name;
  value_type type = value_type::void_t;
};

struct attribute
{
  attribute() = default;
  attribute(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , type(toml::find<value_type>(v, "type"))
  , access(toml::find_or<access_type>(v, "access", access_type::read_only))
  {
  }

  name name;
  value_type type = value_type::void_t;
  access_type access = access_type::read_only;
};

struct command
{
  command() {}
  command(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , return_type(toml::find<value_type>(v, "return_type"))
  , parameter_type(toml::find<value_type>(v, "parameter_type"))
  {
  }

  name name;
  value_type return_type = value_type::void_t;
  value_type parameter_type = value_type::void_t;
};

struct raw_device_server_spec
{
  raw_device_server_spec(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , device_properties(toml::find<std::vector<device_property>>(v, "device_properties"))
  , attributes(toml::find<std::vector<attribute>>(v, "attributes"))
  , commands(toml::find<std::vector<command>>(v, "commands"))
  {
  }

  name name;
  std::vector<device_property> device_properties;
  std::vector<attribute> attributes;
  std::vector<command> commands;
};

struct device_server_spec : raw_device_server_spec
{
  device_server_spec(toml::value const& v)
  : device_server_spec(raw_device_server_spec(v))
  {
  }

  device_server_spec(raw_device_server_spec const& raw)
  : raw_device_server_spec(raw)
  {
    device_properties_name = fmt::format("{0}_properties", name.snake_cased());
    base_name = fmt::format("{0}_base", name.snake_cased());
    ds_name = fmt::format("{0}TangoAdaptor", name.camel_cased());
    ds_class_name = fmt::format("{0}TangoClass", name.camel_cased());
  }

  std::string device_properties_name;
  std::string base_name;
  std::string ds_name;
  std::string ds_class_name;
};

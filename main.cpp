#include <toml11/toml.hpp>

enum class value_type
{
  void_t,
  long_t,
  float_t,
};
namespace toml
{
  template<>
  struct from<value_type>
  {
    static value_type from_toml(const value& v)
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
      throw std::invalid_argument("Unknown type code: " + code.str);
    }
  };
}

struct attribute
{
  attribute() {}
  attribute(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , type(toml::find<value_type>(v, "type"))
  {
  }

  std::string name;
  value_type type = value_type::void_t;
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

  std::string name;
  value_type return_type = value_type::void_t;
  value_type parameter_type = value_type::void_t;
};

struct device_server_spec
{
  device_server_spec(toml::value const& v)
  : name(toml::find<std::string>(v, "name"))
  , attributes(toml::find<std::vector<attribute>>(v, "attributes"))
  , commands(toml::find<std::vector<command>>(v, "commands"))
  {
  }

  std::string name;
  std::vector<attribute> attributes;
  std::vector<command> commands;
};

int main(int argc, char* argv[])
{
  if (argc < 2)
    return;

  auto input = toml::parse(argv[1]);
  auto spec = toml::get<device_server_spec>(input);
  return 0;
}

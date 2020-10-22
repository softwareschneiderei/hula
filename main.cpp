#include <toml11/toml.hpp>
#include <fmt/format.h>
#include <iostream>

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

const char* tango_type_enum(value_type v)
{
  switch (v)
  {
  case value_type::long_t:
    return "Tango::DEV_LONG";
  case value_type::float_t:
    return "Tango::DEV_FLOAT";
  default:
  case value_type::void_t:
    return "Tango::DEV_VOID";
  }
}

const char* cpp_type(value_type v)
{
  switch (v)
  {
  case value_type::long_t:
    return "long";
  case value_type::float_t:
    return "float";
  default:
  case value_type::void_t:
    return "void";
  }
}

constexpr char const* BASE_CLASS_TEMPLATE = R"(
class {0}_base
{{
public:
  virtual ~{0}_base() = default;

{1}
}};
)";

constexpr char const* TANGO_ADAPTOR_CLASS_TEMPLATE = R"(
class {0}TangoAdaptor : TANGO_BASE_CLASS
{{
public:
  {0}_base* get();
}};
)";

constexpr char const* ATTRIBUTE_CLASS_TEMPLATE = R"(
class {0}Attrib : public Tango::Attr
{{
public:
  {0}Attrib()
  : Tango::Attr("{1}", {2}, {3}) {{}}
  ~{0}Attrib() override = default;
{4}
}};
)";

constexpr char const* ATTRIBUTE_READ_FUNCTION_TEMPLATE = R"(
  {1} read_value{{}};
  void read(Tango::DeviceImpl* dev, Tango::Attribute& att) override
  {{
    auto impl = static_cast<{0}TangoAdaptor*>(dev)->get();
    read_value = impl->read_{2}();
    att.set_value(read_value);
  }}
)";

std::string attribute_class(std::string const& class_prefix, std::string const& name, std::string const& type, std::string const& mutability, std::string const& members)
{
  return fmt::format(ATTRIBUTE_CLASS_TEMPLATE, class_prefix, name, type, mutability, members);
}

std::string attribute_class(std::string const& device_name, attribute const& input)
{
  std::ostringstream str;
  str << fmt::format(ATTRIBUTE_READ_FUNCTION_TEMPLATE, device_name, cpp_type(input.type), input.name);

  return attribute_class(input.name, input.name, tango_type_enum(input.type), "Tango::READ", str.str());
}

std::string build_base_class(device_server_spec const& spec)
{
  // Build the members for the base class
  std::ostringstream str;
  for (auto const& each : spec.attributes)
  {
    str << fmt::format("  virtual {0} read_{1}() = 0;\n", cpp_type(each.type), each.name);
  }

  return fmt::format(BASE_CLASS_TEMPLATE, spec.name, str.str());
}

std::string build_adaptor_class(device_server_spec const& spec)
{
  return fmt::format(TANGO_ADAPTOR_CLASS_TEMPLATE, spec.name);
}

int main(int argc, char* argv[])
{
  if (argc < 2)
    return 0;

  auto input = toml::parse(argv[1]);
  auto spec = toml::get<device_server_spec>(input);

  std::ostream& out = std::cout;

  out << build_base_class(spec);
  out << build_adaptor_class(spec);

  for (auto const& each : spec.attributes)
  {
    out << attribute_class(spec.name, each);
    out << std::endl;
  }

  return 0;
}

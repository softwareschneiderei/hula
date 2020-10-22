#include <toml11/toml.hpp>
#include <fmt/format.h>
#include <iostream>
#include <filesystem>
#include "name.hpp"

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

  name name;
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

  name name;
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

  name name;
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
  using factory_type = std::function<std::unique_ptr<{0}_base>()>;
  static int register_and_run(int argc, char* argv[], factory_type factory);
}};
)";

constexpr char const* TANGO_ADAPTOR_CLASS_TEMPLATE = R"(
class {0}TangoAdaptor : public TANGO_BASE_CLASS
{{
public:
  {1}_base* get();
}};
)";


constexpr char const* TANGO_ADAPTOR_DEVICE_CLASS_CLASS_TEMPLATE = R"(
class {0}TangoClass : public Tango::DeviceClass 
{{
public:
  void attribute_factory(std::vector<Tango::Attr*>& attributes) override
  {{{1}  }}
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
    att.set_value(&read_value);
  }}
)";

std::string attribute_class(std::string const& class_prefix, std::string const& name, std::string const& type, std::string const& mutability, std::string const& members)
{
  return fmt::format(ATTRIBUTE_CLASS_TEMPLATE, class_prefix, name, type, mutability, members);
}

std::string attribute_class(std::string const& device_name, attribute const& input)
{
  std::ostringstream str;
  str << fmt::format(ATTRIBUTE_READ_FUNCTION_TEMPLATE, device_name, cpp_type(input.type), input.name.snake_cased());

  return attribute_class(input.name.camel_cased(), input.name.camel_cased(), tango_type_enum(input.type), "Tango::READ", str.str());
}

std::string build_base_class(device_server_spec const& spec)
{
  // Build the members for the base class
  std::ostringstream str;
  for (auto const& each : spec.attributes)
  {
    str << fmt::format("  virtual {0} read_{1}() = 0;\n", cpp_type(each.type), each.name.snake_cased());
  }

  return fmt::format(BASE_CLASS_TEMPLATE, spec.name.snake_cased(), str.str());
}

std::string build_adaptor_class(device_server_spec const& spec)
{
  return fmt::format(TANGO_ADAPTOR_CLASS_TEMPLATE, spec.name.camel_cased(), spec.name.snake_cased());
}

std::string build_device_class(device_server_spec const& spec)
{
  constexpr char const* CREATE_ATTRIBUTE_TEMPLATE = R"(
    auto {0} = new {1}Attrib();
    {0}->set_default_properties(Tango::UserDefaultAttrProp{{}});
    attributes.push_back({0});
)";
  std::ostringstream str;
  for (auto const& attribute : spec.attributes)
  {
    str << fmt::format(CREATE_ATTRIBUTE_TEMPLATE, attribute.name.dromedary_cased(), attribute.name.camel_cased());
  }
  return fmt::format(TANGO_ADAPTOR_DEVICE_CLASS_CLASS_TEMPLATE, spec.name.camel_cased(), str.str());
}

int main(int argc, char* argv[])
{
  if (argc < 3)
    return 0;

  auto input = toml::parse(argv[1]);
  auto spec = toml::get<device_server_spec>(input);

  std::filesystem::path output_path{argv[2]};

  std::ofstream header_file(output_path / fmt::format("hula_{0}.hpp", spec.name.snake_cased()));
  
  constexpr char const* HULA_HEADER_HEADER = R"(// Generated by hula. DO NOT MODIFY, CHANGES WILL BE LOST.
#include <functional>
#include <memory>
)";

  header_file << HULA_HEADER_HEADER;
  header_file << build_base_class(spec);

  std::ofstream source_file(output_path / fmt::format("hula_{0}.cpp", spec.name.snake_cased()));
  std::ostream& out = source_file;
  constexpr char const* HULA_IMPLEMENTATION_HEADER = R"(// Generated by hula. DO NOT MODIFY, CHANGES WILL BE LOST.
#include "hula_{0}.hpp"
#include <tango.h>

)";
  out << fmt::format(HULA_IMPLEMENTATION_HEADER, spec.name.snake_cased());
  out << build_adaptor_class(spec);

  for (auto const& each : spec.attributes)
  {
    out << attribute_class(spec.name.camel_cased(), each);
    out << std::endl;
  }

  out << build_device_class(spec);

  return 0;
}

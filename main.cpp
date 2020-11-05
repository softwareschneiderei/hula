#include <fmt/format.h>
#include <iostream>
#include <filesystem>
#include "device_server_spec.hpp"

constexpr const char* tango_type_enum(value_type v)
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

constexpr const char* tango_access_enum(access_type v)
{
  switch (v)
  {
  default:
  case access_type::read_only:
    return "Tango::READ";
  case access_type::write_only:
    return "Tango::WRITE";
  case access_type::read_write:
    return "Tango::READ_WRITE";
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

std::string parameter_list(value_type type)
{
  switch(type)
  {
  case value_type::long_t:
    return "long rhs";
  case value_type::float_t:
    return "float rhs";
  default:
  case value_type::void_t:
    return "";
  }
}

constexpr char const* DEVICE_PROPERTIES_TEMPLATE = R"(
struct {0}
{{
{1}}};
)";

constexpr char const* BASE_CLASS_TEMPLATE = R"(
class {0}
{{
public:
  using factory_type = std::function<std::unique_ptr<{0}>({1} const& properties)>;

  virtual ~{0}() = default;
{2}
  static int register_and_run(int argc, char* argv[], factory_type factory);
}};
)";

constexpr char const* TANGO_ADAPTOR_CLASS_TEMPLATE = R"(
class {0} : public TANGO_BASE_CLASS
{{
public:
  {0}(Tango::DeviceClass* cl, char const* name, factory_type factory)
  : TANGO_BASE_CLASS(cl, name)
  , factory_(std::move(factory))
  , impl_(factory_(load_device_properties()))
  {{
  }}

  void init_device() override
  {{
    impl_.reset();
    impl_ = factory_(load_device_properties());
  }}

  {1}* get()
  {{
    return impl_.get();
  }}

  {2} load_device_properties()
  {{
    // TODO
    return {2}{{}};
  }}
private:
  factory_type factory_;
  std::unique_ptr<{1}> impl_;
}};
)";


constexpr char const* TANGO_ADAPTOR_DEVICE_CLASS_CLASS_TEMPLATE = R"(
class {0}TangoClass : public Tango::DeviceClass 
{{
public:
  static factory_type factory_;

  explicit {0}TangoClass(std::string& name)
  : Tango::DeviceClass(name)
  {{
  }}

  ~{0}TangoClass() final = default;

  void attribute_factory(std::vector<Tango::Attr*>& attributes) override
  {{{1}  }}
  
  void command_factory() override
  {{
{2}  }}

  void device_factory(Tango::DevVarStringArray const* devlist_ptr)
  {{
	  for (unsigned long i=0; i<devlist_ptr->length(); i++)
	  {{
		  device_list.push_back(new {0}TangoAdaptor(this, (*devlist_ptr)[i], factory_));
	  }}

	  for (unsigned long i=1; i<=devlist_ptr->length(); i++)
	  {{
		  auto dev = static_cast<{0}TangoAdaptor*>(device_list[device_list.size()-i]);
		  if ((Tango::Util::_UseDb == true) && (Tango::Util::_FileDb == false))
			  export_device(dev);
		  else
			  export_device(dev, dev->get_name().c_str());
	  }}
  }}
}};

// Define the static factory
factory_type {0}TangoClass::factory_;
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
  void read(Tango::DeviceImpl* dev, Tango::Attribute& attr) override
  {{
    auto impl = static_cast<{0}*>(dev)->get();
    read_value = impl->read_{2}();
    attr.set_value(&read_value);
  }}
)";

constexpr char const* ATTRIBUTE_WRITE_FUNCTION_TEMPLATE = R"(
  void write(Tango::DeviceImpl* dev, Tango::WAttribute& attr) override
  {{
    auto impl = static_cast<{0}*>(dev)->get();
    {1} arg{{}};
    attr.get_write_value(arg);
    impl->write_{2}(arg);
  }}
)";

std::string attribute_class(std::string const& class_prefix, std::string const& name, std::string const& type, std::string const& mutability, std::string const& members)
{
  return fmt::format(ATTRIBUTE_CLASS_TEMPLATE, class_prefix, name, type, mutability, members);
}

std::string attribute_class(std::string const& ds_name, attribute const& input)
{
  std::ostringstream str;

  if (is_readable(input.access))
  {
    str << fmt::format(ATTRIBUTE_READ_FUNCTION_TEMPLATE, ds_name, cpp_type(input.type), input.name.snake_cased());
  }

  if (is_writable(input.access))
  {
    str << fmt::format(ATTRIBUTE_WRITE_FUNCTION_TEMPLATE, ds_name, cpp_type(input.type), input.name.snake_cased());
  }

  return attribute_class(input.name.camel_cased(), input.name.camel_cased(), tango_type_enum(input.type), tango_access_enum(input.access), str.str());
}

std::string build_base_class(device_server_spec const& spec)
{
  // Build the members for the base class
  std::ostringstream str;
  if (!spec.attributes.empty())
  {
    str << "\n  // attributes\n";
    for (auto const& each : spec.attributes)
    {
      if (is_readable(each.access))
      {
        str << fmt::format("  virtual {0} read_{1}() = 0;\n", cpp_type(each.type), each.name.snake_cased());
      }
      if (is_writable(each.access))
      {
        str << fmt::format("  virtual void write_{0}({1}) = 0;\n", each.name.snake_cased(), parameter_list(each.type));
      }
    }
  }

  if (!spec.commands.empty())
  {
    str << "\n  // commands\n";
    for (auto const& each : spec.commands)
    {
      str << fmt::format("  virtual {0} {1}({2}) = 0;\n", cpp_type(each.return_type), each.name.snake_cased(), parameter_list(each.parameter_type));
    }
  }

  return fmt::format(BASE_CLASS_TEMPLATE, spec.base_name, spec.device_properties_name, str.str());
}

constexpr char const* COMMAND_CLASS_TEMPLATE = R"(
class {0}Command : public Tango::Command
{{
public:
  {0}Command()
  : Tango::Command("{0}", {1}, {2}, "", "", Tango::OPERATOR)
  {{}}

  CORBA::Any* execute(Tango::DeviceImpl* dev, CORBA::Any const& input) override
  {{
    auto impl = static_cast<{4}*>(dev)->get();{3}  }}
}};
)";

constexpr char const* COMMAND_VOID_TO_VOID_EXECUTE_TEMPLATE = R"(
    impl->{0}();
    return new CORBA::Any();
)";

constexpr char const* COMMAND_VOID_TO_VALUE_EXECUTE_TEMPLATE = R"(
    return insert(impl->{0}());
)";

constexpr char const* COMMAND_VALUE_TO_VOID_EXECUTE_TEMPLATE = R"(
    {0} arg{{}};
    extract(input, arg);
    impl->{1}(arg);
    return new CORBA::Any();
)";

constexpr char const* COMMAND_VALUE_TO_VALUE_EXECUTE_TEMPLATE = R"(
    {0} arg{{}};
    extract(input, arg);
    return insert(impl->{1}(arg));
)";

std::string command_execute_impl(command const& cmd)
{
  if (cmd.parameter_type == value_type::void_t)
  {
    if (cmd.return_type == value_type::void_t)
    {
      return fmt::format(COMMAND_VOID_TO_VOID_EXECUTE_TEMPLATE, cmd.name.snake_cased());
    }
    else
    {
      return fmt::format(COMMAND_VOID_TO_VALUE_EXECUTE_TEMPLATE, cmd.name.snake_cased());
    }
  }
  else
  {
    if (cmd.return_type == value_type::void_t)
    {
      return fmt::format(COMMAND_VALUE_TO_VOID_EXECUTE_TEMPLATE, cpp_type(cmd.parameter_type), cmd.name.snake_cased());
    }
    else
    {
      return fmt::format(COMMAND_VALUE_TO_VALUE_EXECUTE_TEMPLATE, cpp_type(cmd.parameter_type), cmd.name.snake_cased());
    }
  }
}

std::string command_class(std::string const& ds_name, command const& input)
{
  auto execute = command_execute_impl(input);
  return fmt::format(COMMAND_CLASS_TEMPLATE, input.name.camel_cased(), tango_type_enum(input.parameter_type), tango_type_enum(input.return_type), execute, ds_name);
}

std::string build_adaptor_class(device_server_spec const& spec)
{
  return fmt::format(TANGO_ADAPTOR_CLASS_TEMPLATE, spec.ds_name, spec.base_name, spec.device_properties_name);
}

std::string build_device_class(device_server_spec const& spec)
{
  constexpr char const* CREATE_ATTRIBUTE_TEMPLATE = R"(
    auto {0} = new {1}Attrib();
    {0}->set_default_properties(Tango::UserDefaultAttrProp{{}});
    attributes.push_back({0});
)";
  std::ostringstream attribute_factory_impl;
  for (auto const& attribute : spec.attributes)
  {
    attribute_factory_impl << fmt::format(CREATE_ATTRIBUTE_TEMPLATE, attribute.name.dromedary_cased(), attribute.name.camel_cased());
  }
  constexpr char const* CREATE_COMMAND_TEMPLATE = R"(    command_list.push_back(new {0}Command());)";

  std::ostringstream command_factory_impl;
  for (auto const& command : spec.commands)
  {
    command_factory_impl << fmt::format(CREATE_COMMAND_TEMPLATE, command.name.camel_cased()) << std::endl;
  }

  return fmt::format(TANGO_ADAPTOR_DEVICE_CLASS_CLASS_TEMPLATE,
    spec.name.camel_cased(), attribute_factory_impl.str(), command_factory_impl.str());
}

std::string build_class_factory(device_server_spec const& spec)
{
  constexpr char const* CLASS_FACTORY_TEMPLATE = R"(
void Tango::DServer::class_factory()
{{
  std::string name("{0}");
  add_class(new {0}TangoClass(name));
}}
)";
  return fmt::format(CLASS_FACTORY_TEMPLATE, spec.name.camel_cased());
}

std::string build_runner(device_server_spec const& spec)
{
  constexpr char const* RUNNER_TEMPLATE = R"(

int {0}_base::register_and_run(int argc, char* argv[], factory_type factory)
{{
  try
  {{
    Tango::Util *tg = Tango::Util::init(argc,argv);

    // Register the factory
    {1}TangoClass::factory_ = std::move(factory);

    // Run the server
    tg->server_init(false);
    std::cout << "Ready to accept request" << endl;
    tg->server_run();
  }}
  catch (std::exception const& e)
  {{
    std::cerr << "Uncaught exception: " << e.what() << "\nExiting..." << std::endl;
    return EXIT_FAILURE;
  }}
  catch (CORBA::Exception const& e)
  {{
    Tango::Except::print_exception(e);
    return EXIT_FAILURE;
  }}

  return EXIT_SUCCESS;
}}

)";
  return fmt::format(RUNNER_TEMPLATE, spec.name.snake_cased(), spec.name.camel_cased());
}

std::string build_device_properties_struct(device_server_spec const& spec)
{
  std::ostringstream str;
  for (auto const& device_property : spec.device_properties)
  {
    str << fmt::format("  {0} {1}{{}};\n", cpp_type(device_property.type), device_property.name.snake_cased());
  }
  return fmt::format(DEVICE_PROPERTIES_TEMPLATE, spec.device_properties_name, str.str());
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
  header_file << build_device_properties_struct(spec);
  header_file << build_base_class(spec);

  std::ofstream source_file(output_path / fmt::format("hula_{0}.cpp", spec.name.snake_cased()));
  std::ostream& out = source_file;
  constexpr char const* HULA_IMPLEMENTATION_HEADER = R"(// Generated by hula. DO NOT MODIFY, CHANGES WILL BE LOST.
#include "hula_{0}.hpp"
#include <tango.h>

using factory_type = {0}_base::factory_type;
)";
  out << fmt::format(HULA_IMPLEMENTATION_HEADER, spec.name.snake_cased());
  out << build_adaptor_class(spec);

  for (auto const& each : spec.attributes)
  {
    out << attribute_class(spec.ds_name, each) << std::endl;
  }

  for (auto const& each : spec.commands)
  {
    out << command_class(spec.ds_name, each) << std::endl;
  }

  out << build_device_class(spec);
  out << build_class_factory(spec);
  out << build_runner(spec);

  return 0;
}

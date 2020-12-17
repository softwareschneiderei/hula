#include <fmt/format.h>
#include <iostream>
#include <filesystem>
#include "device_server_spec.hpp"
#include "types.hpp"

template <class T, class Transform>
std::string join_applied(std::vector<T> const& input, std::string const& separator, Transform transform)
{
  if (input.empty())
    return {};

  std::ostringstream str(transform(input.front()));
  for (std::size_t i = 1, ie = input.size(); i != ie; ++i)
    str << separator << transform(input[i]);
  return str.str();
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

constexpr char const* DEVICE_PROPERTIES_TEMPLATE = R"(
struct {0}
{{
{1}}};
)";

constexpr char const* BASE_CLASS_TEMPLATE = R"(
class {0}
{{
public:
  using device_state = hula::device_state;
  using operating_state_result = hula::operating_state_result;
  using factory_type = std::function<std::unique_ptr<{0}>({1} const& properties)>;

  virtual ~{0}() = default;
{2}
  // special
  virtual operating_state_result operating_state();
}};

inline operating_state_result {0}::operating_state()
{{
  return {{device_state::unknown, "Unknown"}};
}}

)";

constexpr char const* TANGO_ADAPTOR_CLASS_TEMPLATE = R"(
class {0} : public TANGO_BASE_CLASS
{{
public:
  using factory_type = {1}::factory_type;
  {0}(Tango::DeviceClass* cl, char const* name, factory_type factory)
  : TANGO_BASE_CLASS(cl, name)
  , factory_(std::move(factory))
  {{
    {0}::init_device();
  }}

  void init_device() override
  {{
    try
    {{
      impl_.reset();
      impl_ = factory_(load_device_properties());
    }}
    catch(...)
    {{
      convert_exception();
    }}
  }}

  {1}* get()
  {{
    return impl_.get();
  }}

  {2} load_device_properties()
  {{{3}
  }}

  Tango::DevState dev_state() override
  {{
    store_operating_state(get()->operating_state());
	  return get_state();
  }}

  Tango::ConstDevString dev_status() override
  {{
    store_operating_state(get()->operating_state());
    return Tango::DeviceImpl::dev_status();
  }}

  void store_operating_state(operating_state_result const& current)
  {{
    auto state = convert_state(current.state);
    set_state(state);
    set_status(current.status);
	  if (state!=Tango::ALARM)
		  Tango::DeviceImpl::dev_state();
  }}

private:
  factory_type factory_;
  std::unique_ptr<{1}> impl_;
}};
)";


constexpr char const* TANGO_ADAPTOR_DEVICE_CLASS_CLASS_TEMPLATE = R"(
class {0} : public Tango::DeviceClass 
{{
public:
  using factory_type = {1}::factory_type;
  static factory_type factory_;

  static Tango::DeviceClass* create()
  {{
    std::string name("{5}");
    return new {0}(name);
  }}

  explicit {0}(std::string& name)
  : Tango::DeviceClass(name)
  {{
    set_default_properties();
  }}

  ~{0}() final = default;

  void attribute_factory(std::vector<Tango::Attr*>& attributes) override
  {{
    using namespace {6};
    {2}
  }}
  
  void command_factory() override
  {{
    using namespace {6};
   {3}
  }}

  void device_factory(Tango::DevVarStringArray const* devlist_ptr)
  {{
	  for (unsigned long i=0; i<devlist_ptr->length(); i++)
	  {{
		  device_list.push_back(new {1}(this, (*devlist_ptr)[i], factory_));
	  }}

	  for (unsigned long i=1; i<=devlist_ptr->length(); i++)
	  {{
		  auto dev = static_cast<{1}*>(device_list[device_list.size()-i]);
		  if ((Tango::Util::_UseDb == true) && (Tango::Util::_FileDb == false))
			  export_device(dev);
		  else
			  export_device(dev, dev->get_name().c_str());
	  }}
  }}
  
  void set_default_properties()
  {{{4}
  }}
}};

// Define the static factory
{0}::factory_type {0}::factory_;
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
    try
    {{
      to_tango<{3}>::assign(read_value, impl->read_{2}());
    }}
    catch(...)
    {{
      convert_exception();
    }}
    attr.set_value(&read_value);
  }}
)";

constexpr char const* ATTRIBUTE_WRITE_FUNCTION_TEMPLATE = R"(
  void write(Tango::DeviceImpl* dev, Tango::WAttribute& attr) override
  {{
    auto impl = static_cast<{0}*>(dev)->get();
    {1} arg{{}};
    attr.get_write_value(arg);
    try
    {{
      impl->write_{2}(arg);
    }}
    catch(...)
    {{
      convert_exception();
    }}
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
    str << fmt::format(ATTRIBUTE_READ_FUNCTION_TEMPLATE, ds_name,
      tango_type(input.type), input.name.snake_cased(), cpp_type(input.type));
  }

  if (is_writable(input.access))
  {
    str << fmt::format(ATTRIBUTE_WRITE_FUNCTION_TEMPLATE, ds_name, tango_type(input.type), input.name.snake_cased());
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
        str << fmt::format("  virtual void write_{0}({1}) = 0;\n", each.name.snake_cased(), cpp_parameter_list(each.type));
      }
    }
  }

  if (!spec.commands.empty())
  {
    str << "\n  // commands\n";
    for (auto const& each : spec.commands)
    {
      str << fmt::format("  virtual {0} {1}({2}) = 0;\n", cpp_type(each.return_type), each.name.snake_cased(), cpp_parameter_list(each.parameter_type));
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
    try
    {{
      impl->{0}();
    }}
    catch(...)
    {{
      convert_exception();
    }}
    return new CORBA::Any();
)";

constexpr char const* COMMAND_VOID_TO_VALUE_EXECUTE_TEMPLATE = R"(
    try
    {{
      return insert(to_tango<{1}>::convert(impl->{0}()));
    }}
    catch(...)
    {{
      convert_exception();
    }}
)";

constexpr char const* COMMAND_VALUE_TO_VOID_EXECUTE_TEMPLATE = R"(
    {0} arg{{}};
    extract(input, arg);
    try
    {{
      impl->{1}(arg);
    }}
    catch(...)
    {{
      convert_exception();
    }}
    return new CORBA::Any();
)";

constexpr char const* COMMAND_VALUE_TO_VALUE_EXECUTE_TEMPLATE = R"(
    {0} arg{{}};
    extract(input, arg);
    try
    {{
      return insert(to_tango<{2}>::convert(impl->{1}(arg)));
    }}
    catch(...)
    {{
      convert_exception();
    }}
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
      return fmt::format(COMMAND_VOID_TO_VALUE_EXECUTE_TEMPLATE, cmd.name.snake_cased(), cpp_type(cmd.return_type));
    }
  }
  else
  {
    if (cmd.return_type == value_type::void_t)
    {
      return fmt::format(COMMAND_VALUE_TO_VOID_EXECUTE_TEMPLATE, tango_type(cmd.parameter_type), cmd.name.snake_cased());
    }
    else
    {
      return fmt::format(COMMAND_VALUE_TO_VALUE_EXECUTE_TEMPLATE, tango_type(cmd.parameter_type), cmd.name.snake_cased(), cpp_type(cmd.return_type));
    }
  }
}

std::string command_class(std::string const& ds_name, command const& input)
{
  auto execute = command_execute_impl(input);
  return fmt::format(COMMAND_CLASS_TEMPLATE, input.name.camel_cased(), tango_type_enum(input.parameter_type), tango_type_enum(input.return_type), execute, ds_name);
}

std::string load_device_properties_impl(device_server_spec const& spec)
{
  constexpr char const* IMPL_TEMPLATE = R"(
    Tango::DbData property_data{{{1}}};
    if (property_data.empty() || !Tango::Util::instance()->_UseDb)
      return {{}};

    // Load property data from the database
	  get_db_device()->get_property(property_data);

    // Write it to the struct
    {0} loaded;
    {2}
    return loaded;
)";
  std::stringstream init_list;
  std::stringstream loader_code;
  
  constexpr char const* LOAD_TEMPLATE = R"(
    if (!property_data[{0}].is_empty())
    {{
      from_tango<{2}>::load(loaded.{1}, property_data[{0}]);
    }}
)";
  
  std::size_t index = 0;
  for (auto const& device_property : spec.device_properties)
  {
    init_list << fmt::format("\"{0}\",", device_property.name.camel_cased());
    loader_code << fmt::format(LOAD_TEMPLATE, index++, device_property.name.snake_cased(), cpp_type(device_property.type));
  }

  return fmt::format(IMPL_TEMPLATE, spec.device_properties_name, init_list.str(), loader_code.str());
}

std::string build_adaptor_class(device_server_spec const& spec)
{
  return fmt::format(TANGO_ADAPTOR_CLASS_TEMPLATE, spec.ds_name, spec.base_name, spec.device_properties_name, load_device_properties_impl(spec));
}

std::string set_default_properties_impl(device_server_spec const& spec)
{
  constexpr const char* DEFAULTER_IMPL = R"(
    // {0}
    {{
      std::string name = "{0}";
      std::string description;
      add_wiz_dev_prop(name, description);
    }})";

  std::ostringstream str;
  for (auto const& device_property : spec.device_properties)
  {
    str << fmt::format(DEFAULTER_IMPL, device_property.name.camel_cased());
  }
  return str.str();
}

std::string build_device_class(device_server_spec const& spec)
{
  constexpr char const* CREATE_ATTRIBUTE_TEMPLATE = R"(
    {{
      auto {0} = new {1}Attrib();
      Tango::UserDefaultAttrProp properties{{}};
      {0}->set_default_properties(properties);
      attributes.push_back({0});
    }}
)";
  std::ostringstream attribute_factory_impl;
  for (auto const& attribute : spec.attributes)
  {
    attribute_factory_impl << fmt::format(CREATE_ATTRIBUTE_TEMPLATE, attribute.name.dromedary_cased(), attribute.name.camel_cased());
  }
  constexpr char const* CREATE_COMMAND_TEMPLATE = R"(command_list.push_back(new {0}Command());)";

  auto command_factory_impl = join_applied(spec.commands, "\n    ", [&](command const& each)
  {
    return fmt::format(CREATE_COMMAND_TEMPLATE, each.name.camel_cased());
  });

  return fmt::format(TANGO_ADAPTOR_DEVICE_CLASS_CLASS_TEMPLATE,
    spec.ds_class_name, spec.ds_name, attribute_factory_impl.str(),
    command_factory_impl, set_default_properties_impl(spec),
    spec.name.camel_cased(), spec.grouping_namespace_name);
}

std::string build_class_factory(std::vector<device_server_spec> const& spec_list)
{
  constexpr char const* CLASS_FACTORY_TEMPLATE = R"(
void Tango::DServer::class_factory()
{{
{0}
}}
)";
  auto body = join_applied(spec_list, "\n", [](device_server_spec const& spec)
  {
    return fmt::format("  add_class({0}::create());", spec.ds_class_name);
  });
  return fmt::format(CLASS_FACTORY_TEMPLATE, body);
}

std::string build_factory_parameters(std::vector<device_server_spec> const& spec_list)
{
  return join_applied(spec_list, ",\n  ", [](device_server_spec const& spec)
  {
    return fmt::format("{0}::factory_type make_{1}", spec.base_name, spec.name.snake_cased());
  });
}

std::string build_runner_declaration(std::vector<device_server_spec> const& spec_list)
{
  auto factory_parameters = build_factory_parameters(spec_list);
  return fmt::format("int register_and_run(int argc, char* argv[],\n  {0});", factory_parameters);
}

std::string build_runner(std::vector<device_server_spec> const& spec_list)
{
  constexpr char const* RUNNER_TEMPLATE = R"(
int hula::register_and_run(int argc, char* argv[],
  {0})
{{
  try
  {{
    Tango::Util *tg = Tango::Util::init(argc,argv);

    // Register the factories
    {1}

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
  auto factory_parameters = build_factory_parameters(spec_list);
  auto factory_assignments = join_applied(spec_list, "\n    ", [](device_server_spec const& spec)
  {
    return fmt::format("{0}::factory_ = std::move(make_{1});", spec.ds_class_name, spec.name.snake_cased());
  });
  return fmt::format(RUNNER_TEMPLATE, factory_parameters, factory_assignments);
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

std::string build_grouping_namespace_start(device_server_spec const& spec)
{
  constexpr char const* TEMPLATE = R"(

// Group attributes and commands
namespace {0} {{
)";

    // Create a grouping namespace in the unnamed namespace for
    // attributes and classes so they do not clash with other devices
    return fmt::format(TEMPLATE, spec.grouping_namespace_name);
}

std::string build_grouping_namespace_end(device_server_spec const& spec)
{
  constexpr char const* TEMPLATE = R"(
}} // {0}

)";
  return fmt::format(TEMPLATE, spec.grouping_namespace_name);
}

constexpr char const* HULA_HEADER_HEADER = R"(// Generated by hula. DO NOT MODIFY, CHANGES WILL BE LOST.
#pragma once
#include <string>
#include <functional>
#include <memory>
#include <cstdint>
#include <vector>

namespace hula {

template <typename element_type>
struct image
{
  std::vector<element_type> data;
  std::size_t width = 0;
  std::size_t height = 0;
};

enum class device_state
{
  on,
  off,
  close,
  open,
  insert,
  extract,
  moving,
  standby,
  fault,
  init,
  running,
  alarm,
  disable,
  unknown
};

struct operating_state_result
{
  device_state state = device_state::unknown;
  std::string status;
};
)";

constexpr char const* HULA_HEADER_FOOTER = R"(

} // hula
)";

constexpr char const* HULA_IMPLEMENTATION_HEADER = R"--(// Generated by hula. DO NOT MODIFY, CHANGES WILL BE LOST.
#include "hula_generated.hpp"
#include <tango.h>

using namespace hula;

namespace {
template <class T>
struct to_tango
{
  static T convert(T rhs)
  {
    return rhs;
  }
  static void assign(T& lhs, T rhs)
  {
    lhs = rhs;
  }
};

// std::int32_t and Tango::DevLong are not the same on some OSes, e.g. Win32
template <>
struct to_tango<std::int32_t>
{
  static_assert(sizeof(Tango::DevLong) == sizeof(std::int32_t), "Tango::DevLong must be compatible to std::int32_t");
  static Tango::DevLong convert(std::int32_t rhs)
  {
    return static_cast<Tango::DevLong>(rhs);
  }
  static void assign(Tango::DevLong& lhs, std::int32_t rhs)
  {
    lhs = static_cast<Tango::DevLong>(rhs);
  }
};

template <>
struct to_tango<std::string>
{
  static Tango::DevString convert(std::string const& rhs)
  {
    return Tango::string_dup(rhs.c_str());
  }

  static void assign(Tango::DevString& lhs, std::string const& rhs)
  {
    lhs = Tango::string_dup(rhs.c_str());
  }
};

template <>
struct to_tango<image<std::uint8_t>>
{
  static void assign(Tango::EncodedAttribute& lhs, image<std::uint8_t>& rhs)
  {
    lhs.encode_gray8(rhs.data.data(), rhs.width, rhs.height);
  }
};

template <>
struct to_tango<image<std::uint16_t>>
{
  static void assign(Tango::EncodedAttribute& lhs, image<std::uint16_t>& rhs)
  {
    lhs.encode_gray16(rhs.data.data(), rhs.width, rhs.height);
  }
};

template <class T>
struct from_tango
{
  static void load(T& lhs, Tango::DbDatum& rhs)
  {
    rhs >> lhs;
  }
};

template <>
struct from_tango<std::int32_t>
{
  static void load(std::int32_t& lhs, Tango::DbDatum& rhs)
  {
    Tango::DevLong tmp{};
    rhs >> tmp;
    lhs = static_cast<std::int32_t>(tmp);
  }
};


[[noreturn]] void convert_exception()
{
  try
  {
    throw;
  }
  catch (Tango::DevFailed const&)
  {
    throw;
  }
  catch (std::exception const& e)
  {
    Tango::Except::throw_exception("STD_EXCEPTION", e.what(), "convert_exception()");
  }
  catch (...)
  {
    Tango::Except::throw_exception("UNKNOWN_EXCEPTION", "Unknown exception", "convert_exception()");
  }
}

inline Tango::DevState convert_state(device_state s)
{
  // Make sure the hula definitions match up
  static_assert(static_cast<Tango::DevState>(device_state::on)==Tango::ON, "ON does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::off)==Tango::OFF, "OFF does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::close)==Tango::CLOSE, "CLOSE does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::open)==Tango::OPEN, "OPEN does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::insert)==Tango::INSERT, "INSERT does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::extract)==Tango::EXTRACT, "EXTRACT does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::moving)==Tango::MOVING, "MOVING does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::standby)==Tango::STANDBY, "STANDBY does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::fault)==Tango::FAULT, "FAULT does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::init)==Tango::INIT, "INIT does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::running)==Tango::RUNNING, "RUNNING does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::alarm)==Tango::ALARM, "ALARM does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::disable)==Tango::DISABLE, "DISABLE does not match up");
  static_assert(static_cast<Tango::DevState>(device_state::unknown)==Tango::UNKNOWN, "UNKNOWN does not match up");
  return static_cast<Tango::DevState>(s);
}

)--";

constexpr char const* HULA_IMPLEMENTATION_PUBLIC_SECTION_START = R"(
} // namespace
)";

int run(int argc, char* argv[])
{
  if (argc < 3)
  {
    fmt::print("{0} <spec> (<spec> ...) <output-path>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // The number of specs
  auto const N = argc - 2;
  std::filesystem::path output_path{argv[argc-1]};
  if (!exists(output_path))
  {
    fmt::print("Output path {0} does not exist\n", output_path.string());
    return EXIT_FAILURE;
  }

  std::vector<device_server_spec> spec_list;
  for (int i = 0; i < N; ++i)
  {
    auto input = toml::parse(argv[1]);
    auto spec = toml::get<device_server_spec>(input);
    spec_list.push_back(spec);
  }
  

  std::ofstream header_file(output_path / "hula_generated.hpp");
  std::ofstream source_file(output_path / "hula_generated.cpp");
  header_file << HULA_HEADER_HEADER;
  source_file << HULA_IMPLEMENTATION_HEADER;

  for (auto const& spec : spec_list)
  {
    header_file << build_device_properties_struct(spec);
    header_file << build_base_class(spec);

    std::ostream& out = source_file;

    out << build_adaptor_class(spec);

    out << build_grouping_namespace_start(spec);
    for (auto const& each : spec.attributes)
    {
      out << attribute_class(spec.ds_name, each) << std::endl;
    }

    for (auto const& each : spec.commands)
    {
      out << command_class(spec.ds_name, each) << std::endl;
    }
    out << build_grouping_namespace_end(spec);
    out << build_device_class(spec);
  }

  header_file << build_runner_declaration(spec_list);
  header_file << HULA_HEADER_FOOTER;

  source_file << HULA_IMPLEMENTATION_PUBLIC_SECTION_START;
  source_file << build_class_factory(spec_list);
  source_file << build_runner(spec_list);

  return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
  if (argc == 0)
  {
    return EXIT_FAILURE;
  }

  try
  {
    return run(argc, argv);
  }
  catch (std::exception const& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
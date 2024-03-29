#include "code_generator.hpp"

using namespace std::string_literals;

template <class T, class Transform>
std::string join_applied(std::vector<T> const& input, std::string const& separator, Transform transform)
{
  if (input.empty())
    return {};

  std::ostringstream str;
  str << transform(input.front());
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

constexpr const char* tango_display_level(display_level_t v)
{
  switch (v)
  {
  default:
  case display_level_t::operator_level:
    return "Tango::OPERATOR";
  case display_level_t::expert_level:
    return "Tango::EXPERT";
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
  template <class T>
  using image = hula::image<T>;
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
class {0} final : public TANGO_BASE_CLASS
{{
public:
  using factory_type = {1}::factory_type;

  {0}(Tango::DeviceClass* cl, char const* name, factory_type factory)
  : TANGO_BASE_CLASS(cl, name)
  , factory_(std::move(factory))
  {{
    {0}::init_device();
  }}

  void init_device() final
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

  static {1}* get(Tango::DeviceImpl* device)
  {{
    return static_cast<{0}*>(device)->impl_.get();
  }}

  {2} load_device_properties()
  {{{3}
  }}

  Tango::DevState dev_state() final
  {{
    store_operating_state(impl_->operating_state());
	  return get_state();
  }}

  Tango::ConstDevString dev_status() final
  {{
    store_operating_state(impl_->operating_state());
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

  void attribute_factory(std::vector<Tango::Attr*>& attributes) final
  {{
    using namespace {6};
    {2}
  }}
  
  void command_factory() final
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
		  auto dev = device_list[device_list.size()-i];
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
class {0}Attrib : public {6}
{{
public:
  {0}Attrib()
  : {6}("{1}", {2}, {3}{5}) {{}}
  ~{0}Attrib() final = default;
{4}
}};
)";

constexpr char const* ATTRIBUTE_READ_FUNCTION_TEMPLATE = R"(
  {1} read_value{{}};
  void read(Tango::DeviceImpl* dev, Tango::Attribute& attr) final
  {{
    auto impl = {0}::get(dev);
    try
    {{
      to_tango<{3}>::assign(read_value, impl->read_{2}());
    }}
    catch(...)
    {{
      convert_exception();
    }}
    attr.set_value({4});
  }}
)";

constexpr char const* ATTRIBUTE_WRITE_FUNCTION_TEMPLATE = R"(
  void write(Tango::DeviceImpl* dev, Tango::WAttribute& attr) final
  {{
    auto impl = {0}::get(dev);
    {1} arg{{}};
    attr.get_write_value(arg);
    try
    {{
      impl->write_{2}({3});
    }}
    catch(...)
    {{
      convert_exception();
    }}
  }}
)";

std::string attribute_class(std::string const& class_prefix, std::string const& name,
  std::string const& type, std::string const& mutability, std::string const& members,
  std::string const& additional_ctor_args, std::string const& attribute_base_class)
{
  return fmt::format(ATTRIBUTE_CLASS_TEMPLATE, class_prefix, name, type, mutability,
    members, additional_ctor_args, attribute_base_class);
}

std::string read_value_type(attribute_type_t const& type)
{
  switch (type.rank)
  {
  default:
  case attribute_rank_t::scalar:
    return tango_type(type);

  case attribute_rank_t::spectrum:
    return fmt::format("std::vector<{0}>", tango_type(type.type, false));

  case attribute_rank_t::image:
    return fmt::format("image<{0}>", tango_type(type.type, false));
  }
}

std::string write_temporary_type(attribute_type_t const& type)
{
  switch (type.rank)
  {
  default:
  case attribute_rank_t::scalar:
    return tango_type(type);

  case attribute_rank_t::spectrum:
  case attribute_rank_t::image:
    return tango_type(type.type, false) + " const*"s;
  };
}

std::string attribute_class(std::string const& ds_name, attribute const& input)
{
  std::ostringstream str;

  std::string additional_ctor_args;
  std::string attribute_base_class;
  switch (input.type.rank)
  {
  case attribute_rank_t::image:
    additional_ctor_args = fmt::format(", {0}, {1}", input.type.max_size[0], input.type.max_size[1]);
    attribute_base_class = "Tango::ImageAttr";
    break;
  case attribute_rank_t::spectrum:
    additional_ctor_args = fmt::format(", {0}", input.type.max_size[0]);
    attribute_base_class = "Tango::SpectrumAttr";
    break;
  default:
  case attribute_rank_t::scalar:
    attribute_base_class = "Tango::Attr";
    break;
  };
  if (is_readable(input.access))
  {
    std::string set_value_args;
    if (input.type.rank == attribute_rank_t::spectrum)
    {
      set_value_args = "read_value.data(), read_value.size()"s;
    }
    else if (input.type.rank == attribute_rank_t::image)
    {
      set_value_args = "read_value.data.data(), read_value.width, read_value.height"s;
    }
    else
    {
      set_value_args = "&read_value"s;
    }

    str << fmt::format(ATTRIBUTE_READ_FUNCTION_TEMPLATE,
      ds_name, read_value_type(input.type), input.name.snake_cased(),
      cpp_type(input.type), set_value_args);
  }

  if (is_writable(input.access))
  {
    std::string argument = "arg"s;
    if (input.type.rank == attribute_rank_t::spectrum)
    {
      argument = fmt::format("std::vector<{0}>{{arg, arg+attr.get_w_dim_x()}}", cpp_type(input.type.type, false));
    }
    else if (input.type.rank == attribute_rank_t::image)
    {
      // This is quite ugly. Could maybe use generic Tango::WAttribute to image<T> conversion code
      argument = fmt::format("image<{0}>{{std::vector<{0}>{{arg, arg+(attr.get_w_dim_x()*attr.get_w_dim_y())}},\n        static_cast<std::size_t>(attr.get_w_dim_x()),\n        static_cast<std::size_t>(attr.get_w_dim_y())}}", cpp_type(input.type.type, false));
    }

    str << fmt::format(ATTRIBUTE_WRITE_FUNCTION_TEMPLATE, ds_name, write_temporary_type(input.type), input.name.snake_cased(), argument);
  }

  // Attribute type. That is just the element type for spectrums and images
  auto attribute_tango_type = tango_type_enum(input.type.type, false);

  return attribute_class(input.name.camel_cased(), input.name.camel_cased(),
    attribute_tango_type, tango_access_enum(input.access),
    str.str(), additional_ctor_args, attribute_base_class);
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
  : Tango::Command("{0}", {1}, {2}, "{3}", "{4}", {5})
  {{}}

  CORBA::Any* execute(Tango::DeviceImpl* dev, CORBA::Any const& input) final
  {{
    auto impl = {7}::get(dev);{6}  }}
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
      impl->{2}(prepare<{1}>::argument(arg));
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
      return insert(to_tango<{3}>::convert(impl->{2}(prepare<{1}>::argument(arg))));
    }}
    catch(...)
    {{
      convert_exception();
    }}
)";

std::string command_temporary_type(command_type_t const& type)
{
  std::string base = tango_type(type);
  if (type.is_array)
  {
    return base + " const*";
  }
  return base;
}

std::string command_execute_impl(command const& cmd)
{
  if (cmd.parameter_type.type == value_type::void_t)
  {
    if (cmd.return_type.type == value_type::void_t)
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
    if (cmd.return_type.type == value_type::void_t)
    {
      return fmt::format(COMMAND_VALUE_TO_VOID_EXECUTE_TEMPLATE,
        command_temporary_type(cmd.parameter_type),
        cpp_type(cmd.parameter_type), cmd.name.snake_cased());
    }
    else
    {
      return fmt::format(COMMAND_VALUE_TO_VALUE_EXECUTE_TEMPLATE,
        command_temporary_type(cmd.parameter_type), cpp_type(cmd.parameter_type),
        cmd.name.snake_cased(), cpp_type(cmd.return_type));
    }
  }
}

std::string command_class(std::string const& ds_name, command const& input)
{
  auto execute = command_execute_impl(input);
  return fmt::format(COMMAND_CLASS_TEMPLATE,
    input.name.camel_cased(),
    tango_type_enum(input.parameter_type),
    tango_type_enum(input.return_type),
    input.parameter_description,
    input.return_description,
    tango_display_level(input.display_level),
    execute, ds_name);
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
    loader_code << fmt::format(LOAD_TEMPLATE, index++, device_property.name.snake_cased(), cpp_type(device_property.type, false));
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
      std::string description = "{1}";
      add_wiz_dev_prop(name, description);
    }})";

  std::ostringstream str;
  for (auto const& device_property : spec.device_properties)
  {
    str << fmt::format(DEFAULTER_IMPL, device_property.name.camel_cased(), device_property.description);
  }
  return str.str();
}

std::string build_attribute_factory_snippet(attribute const& attribute)
{
  constexpr char const* CREATE_ATTRIBUTE_TEMPLATE = R"(
    {{
      auto {0} = new {1}Attrib();
      Tango::UserDefaultAttrProp properties{{}};{2}
      {0}->set_default_properties(properties);
      {0}->set_disp_level({3});
      attributes.push_back({0});
    }}
)";
  auto variable_name = attribute.name.dromedary_cased();

  std::ostringstream extra_properties;
  std::tuple<char const*, std::string> methods_and_values[] = {
    {"set_description", attribute.description},
    {"set_unit", attribute.unit},
    {"set_min_value", attribute.min_value},
    {"set_max_value", attribute.max_value},
  };
  for (auto& [method_name, value] : methods_and_values)
  {
    if (value.empty())
      continue;
    extra_properties << fmt::format("\n      properties.{0}(\"{1}\");", method_name, value);
  }

  return fmt::format(CREATE_ATTRIBUTE_TEMPLATE, variable_name, attribute.name.camel_cased(), extra_properties.str(),
    tango_display_level(attribute.display_level));

}

std::string build_device_class(device_server_spec const& spec)
{
  std::ostringstream attribute_factory_impl;
  for (auto const& attribute : spec.attributes)
  {
    attribute_factory_impl << build_attribute_factory_snippet(attribute);
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
    str << fmt::format("  {0} {1}{{}};\n", cpp_type(device_property.type, false), device_property.name.snake_cased());
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
#include <algorithm>

namespace hula {

template <typename T>
struct image
{
  template <typename X>
  static image cast(image<X> const& rhs)
  {
    std::vector<T> data;
    auto N = rhs.data.size();
    data.resize(rhs.data.size());
    std::transform(rhs.data.begin(), rhs.data.end(), data.begin(), [](auto v) {return static_cast<T>(v);});
    return image<T>{std::move(data), rhs.width, rhs.height};
  }

  std::vector<T> data;
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
  // This is used in commands for insert(...)
  static T convert(T rhs)
  {
    return rhs;
  }
  // This is used to assign to read members in attributes
  static void assign(T& lhs, T rhs)
  {
    lhs = rhs;
  }
};

template <class T, class X>
inline void assign_to(std::vector<T>& lhs, std::vector<X> const& rhs)
{
  lhs.resize(rhs.size());
  for (std::size_t i = 0, ie = rhs.size(); i < ie; ++i)
    lhs[i] = static_cast<T>(rhs[i]);
}

template <class TangoArray, class T>
inline TangoArray* copied_to_tango(std::vector<T> const& rhs)
{
    auto result = std::make_unique<TangoArray>();
    if (!rhs.empty())
    {
      std::size_t N = rhs.size();
      result->length(N);
      for (std::size_t i = 0; i < N; ++i)
        (*result)[i] = rhs[i];
    }
    return result.release();
}

template <class T>
struct prepare
{
  template <class X>
  static T argument(X v)
  {
    return v;
  }
};

template <class T>
struct prepare<std::vector<T>>
{
  template <class S>
  static std::vector<T> argument(_CORBA_Unbounded_Sequence<S> const* rhs)
  {
    std::vector<T> result(rhs->length());
    for (std::size_t i = 0, ie = result.size(); i < ie; ++i)
      result[i] = (*rhs)[i];
    return result;
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

// NOTE: these instantiations do not currently overlap with the uint16 and uint8 variants,
// because we do not have those as basic datatypes yet
template <class T>
struct to_tango<image<T>>
{
  template <class X>
  static void assign(image<X>& lhs, image<T> const& rhs)
  {
    lhs = image<X>::cast(rhs);
  }
};

template <>
struct to_tango<std::vector<std::int32_t>>
{
  static Tango::DevVarLongArray* convert(std::vector<std::int32_t> const& rhs)
  {
    return copied_to_tango<Tango::DevVarLongArray>(rhs);
  }

  static void assign(std::vector<Tango::DevLong>& lhs, std::vector<std::int32_t> const& rhs)
  {
    assign_to(lhs, rhs);
  }
};

template <>
struct to_tango<std::vector<double>>
{
  static Tango::DevVarDoubleArray* convert(std::vector<double> const& rhs)
  {
    return copied_to_tango<Tango::DevVarDoubleArray>(rhs);
  }

  static void assign(std::vector<Tango::DevDouble>& lhs, std::vector<double> const& rhs)
  {
    assign_to(lhs, rhs);
  }
};

template <>
struct to_tango<std::vector<float>>
{
  static Tango::DevVarFloatArray* convert(std::vector<double> const& rhs)
  {
    return copied_to_tango<Tango::DevVarFloatArray>(rhs);
  }

  static void assign(std::vector<Tango::DevFloat>& lhs, std::vector<float> const& rhs)
  {
    assign_to(lhs, rhs);
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


void generate_code(std::vector<device_server_spec> const& spec_list,
  std::filesystem::path const& output_path)
{
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
}

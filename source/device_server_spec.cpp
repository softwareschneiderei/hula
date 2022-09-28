#include "device_server_spec.hpp"

namespace
{

bool contains(std::vector<toml::value> const& parts, std::string const& key)
{
  return std::find_if(parts.begin(), parts.end(), [&](toml::value const& lhs)
  {
    return lhs.as_string() == key;
  }) != parts.end();
}

std::uint32_t parse_integer(std::string_view const& rhs)
{
  std::size_t count = 0;
  
  auto str = std::string{rhs};
  auto result = std::stoi(str, &count);
  if (count != rhs.size())
  {
    throw std::invalid_argument("Malformed integer: " + str);
  }
  return result;
}

std::uint32_t parse_size(std::string_view const& rhs)
{
  auto result = parse_integer(rhs);
  if (result <= 0)
  {
    throw std::invalid_argument("Malformed size, it cannot be zero");
  }
  return result;
}

struct type_code_with_suffix
{
  std::string_view type;
  std::string_view suffix;
  // There's a difference between an empty suffix (e.g. string[]) and no suffix (e.g. string)
  bool has_suffix = false;
};

type_code_with_suffix split_suffix(std::string_view const& type_code)
{
  auto suffix_begin = type_code.find('[');
  if (suffix_begin == std::string::npos)
  {
    return {type_code, {}, false};
  }

  auto suffix_end = type_code.find(']', suffix_begin);
  if (suffix_end == std::string::npos)
  {
    throw std::invalid_argument("Malformed attribute type suffix: no end brace.");
  }

  auto type_tag = type_code.substr(0, suffix_begin);
  auto suffix = type_code.substr(suffix_begin+1, suffix_end-suffix_begin-1);
  return {type_tag, suffix, true};
}

}

access_type toml::from<access_type>::from_toml(value const& v)
{
  auto const& parts = v.as_array();
  auto readable = contains(parts, "read");
  auto writable = contains(parts, "write");
  if (readable)
  {
    if (writable)
      return access_type::read_write;
    return access_type::read_only;
  }
  if (writable)
    return access_type::write_only;
  // default to read only..
  return access_type::read_only;
}

display_level_t toml::from<display_level_t>::from_toml(value const& v)
{
  if (v.as_string() == "operator")
    return display_level_t::operator_level;
  if (v.as_string() == "expert")
    return display_level_t::expert_level;
  throw std::invalid_argument("Invalid attribute display level: " + v.as_string().str);
}

attribute_type_t::attribute_type_t(toml::value const& rhs)
// Need an explicit type here to convert from toml::string to std::string
: attribute_type_t(static_cast<std::string const&>(rhs.as_string()))
{
}

attribute_type_t::attribute_type_t(std::string const& type_code)
{
  auto [tag, suffix, has_suffix] = split_suffix(type_code);

  this->type = from_input_type(std::string{tag});

  if (!has_suffix)
  {
    return;
  }

  if (type == value_type::void_t)
  {
    throw std::invalid_argument("Cannot have void arrays");
  }
    
  auto separator = suffix.find(',');
  if (separator == std::string::npos)
  {
    rank = attribute_rank_t::spectrum;
    max_size[0] = parse_size(suffix);
    return;
  }
  
  auto first = suffix.substr(0, separator);
  auto second = suffix.substr(separator+1);

  this->rank = attribute_rank_t::image;
  max_size[0] = parse_size(first);
  max_size[1] = parse_size(second);
}

command_type_t::command_type_t(toml::value const& v)
// Need an explicit type here to convert from toml::string to std::string
: command_type_t(static_cast<std::string const&>(v.as_string()))
{
}

command_type_t::command_type_t(std::string const& type_code)
{
  auto [tag, suffix, has_suffix] = split_suffix(type_code);

  if (has_suffix && !suffix.empty())
  {
    throw std::invalid_argument("Braces need to empty for array command types");
  }

  is_array = has_suffix;
  type = from_input_type(std::string{tag});

  if (is_array && type == value_type::void_t)
  {
    throw std::invalid_argument("Cannot have void arrays");
  }
}

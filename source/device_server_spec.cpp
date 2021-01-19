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

std::uint32_t parse_integer(std::string const& rhs)
{
  std::size_t count = 0;
  auto result = std::stoi(rhs, &count);
  if (count != rhs.size())
  {
    throw std::invalid_argument("Malformed integer: " + rhs);
  }
  return result;
}

std::uint32_t parse_size(std::string const& rhs)
{
  auto result = parse_integer(rhs);
  if (result <= 0)
  {
    throw std::invalid_argument("Malformed size, it cannot be zero");
  }
  return result;
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

attribute_type_t::attribute_type_t(toml::value const& rhs)
{
  // Need an explicit type here to convert from toml::string to std::string
  std::string const& type_code = rhs.as_string();

  auto suffix_begin = type_code.find('[');
  if (suffix_begin == std::string::npos)
  {
    this->type = from_input_type(type_code);
    return;
  }

  auto suffix_end = type_code.find(']', suffix_begin);
  if (suffix_end == std::string::npos)
  {
    throw std::invalid_argument("Malformed attribute type suffix: no end brace.");
  }

  auto type_tag = type_code.substr(0, suffix_begin);
  this->type = from_input_type(type_tag);
  auto suffix = type_code.substr(suffix_begin+1, suffix_end-suffix_begin-1);
  
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

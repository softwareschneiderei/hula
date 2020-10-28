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
}

access_type toml::from<access_type>::from_toml(value const& v)
{
  auto parts = v.as_array();
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

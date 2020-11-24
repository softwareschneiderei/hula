#include "uncased_name.hpp"
#include <sstream>

uncased_name::uncased_name(std::string const& str)
{
  for (char x : str)
  {
    if (std::isupper(x))
    {
      parts_.push_back(std::string(1, static_cast<char>(std::tolower(x))));
      continue;
    }
    if (x == '_')
    {
      parts_.push_back({});
      continue;
    }

    if (parts_.empty())
      parts_.push_back(std::string(1, x));
    else
      parts_.back().push_back(x);
  }
}

std::string uncased_name::snake_cased() const
{
  if (parts_.empty())
    return "";

  std::ostringstream str;
  str << parts_.front();
  for (std::size_t i = 1; i < parts_.size(); ++i)
  {
    str << '_' << parts_[i];
  }

  return str.str();
}

std::string uncased_name::camel_cased() const
{
  if (parts_.empty())
    return "";

  std::ostringstream str;
  
  for (auto const& part : parts_)
  {
    if (part.empty())
      continue;

    str << static_cast<char>(std::toupper(part.front())) << (part.c_str() + 1);
  }

  return str.str();
}

std::string uncased_name::dromedary_cased() const
{
  if (parts_.empty())
    return "";

  std::ostringstream str;
  
  bool first = false;
  for (auto const& part : parts_)
  {
    if (part.empty())
      continue;

    if (!first)
    {
      str << part;
      first = true;
      continue;
    }

    str << static_cast<char>(std::toupper(part.front())) << (part.c_str() + 1);
  }

  return str.str();
}

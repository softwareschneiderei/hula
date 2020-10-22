#include <vector>
#include <string>

class name
{
public:
  explicit name(std::string const& str);

  std::string snake_cased() const;
  std::string camel_cased() const;

private:
  std::vector<std::string> parts_;
};
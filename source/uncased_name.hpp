#include <vector>
#include <string>

class uncased_name
{
public:
  uncased_name() = default;
  explicit uncased_name(std::string const& str);

  std::string snake_cased() const;
  std::string camel_cased() const;
  std::string dromedary_cased() const;

private:
  std::vector<std::string> parts_;
};
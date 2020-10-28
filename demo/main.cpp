#include "hula_cool_camera.hpp"
#include <iostream>

class cool_camera : public cool_camera_base
{
public:
  long read_binning() override
  {
    return 42;
  }

  float read_exposure_time() override
  {
    return 0.05f;
  }

  void record() override
  {
    std::cout << "Recording!" << std::endl;
  }
};

int main(int argc, char* argv[])
{
  return cool_camera::register_and_run(argc, argv, [] { return std::make_unique<cool_camera>(); });
}

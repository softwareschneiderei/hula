#include "hula_cool_camera.hpp"

class cool_camera : public cool_camera_base
{
public:
  long read_binning()
  {
    return 0;
  }

  float read_exposure_time()
  {
    return 0.05f;
  }
};

int main(int argc, char* argv[])
{
  return cool_camera::register_and_run(argc, argv, [] { return std::make_unique<cool_camera>(); });
}

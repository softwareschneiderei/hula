#include "hula_cool_camera.hpp"
#include <iostream>

class cool_camera : public cool_camera_base
{
public:
  long read_binning() override
  {
    return binning_;
  }

  void write_binning(long rhs) override
  {
    binning_ = rhs;
  }

  float read_exposure_time() override
  {
    return exposure_time_;
  }

  void record() override
  {
    std::cout << "Recording!" << std::endl;
  }

  long square(long rhs) override
  {
    return rhs*rhs;
  }
  
  float report() override
  {
    return 1234.5f;
  }

  void act(float rhs) override
  {
    exposure_time_ = rhs;
  }
private:
  float exposure_time_ = 0.05f;
  long binning_ = 42;
};

int main(int argc, char* argv[])
{
  return cool_camera::register_and_run(argc, argv, [] { return std::make_unique<cool_camera>(); });
}

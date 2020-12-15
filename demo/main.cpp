#include "hula_cool_camera.hpp"
#include <iostream>

class cool_camera : public cool_camera_base
{
public:
  cool_camera(cool_camera_properties const& p)
  : address_(p.address)
  {
  }

  std::int32_t read_binning() override
  {
    return binning_;
  }

  void write_binning(std::int32_t rhs) override
  {
    binning_ = rhs;
  }

  std::string read_notes() override
  {
    return notes_;
  }

  void write_notes(std::string const& rhs) override
  {
    notes_ = rhs;
  }

  float read_exposure_time() override
  {
    return exposure_time_;
  }

  void record() override
  {
    std::cout << "Recording!" << std::endl;
  }

  std::int32_t square(std::int32_t rhs) override
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

  std::string talk(std::string const& rhs) override
  {
    return "You said " + rhs + " @ " + address_;
  }

  operating_state_result operating_state() override
  {
    return {device_state::on, "Powered on!"};
  }

private:
  float exposure_time_ = 0.05f;
  std::int32_t binning_ = 42;
  std::string address_;
  std::string notes_;
};

int main(int argc, char* argv[])
{
  return cool_camera::register_and_run(argc, argv, [] (cool_camera_properties const& p) {
    return std::make_unique<cool_camera>(p); 
  });
}

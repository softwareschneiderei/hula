#include "hula_generated.hpp"
#include <iostream>

class cool_camera : public hula::cool_camera_base
{
public:
  cool_camera(hula::cool_camera_properties const& p)
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

class camera_stand : public hula::camera_stand_base
{
public:
  camera_stand(hula::camera_stand_properties const& properties)
  {
  }
private:

  // Inherited via camera_stand_base
  virtual std::string read_notes() override
  {
    return notes_;
  }
  virtual void write_notes(std::string const& rhs) override
  {
    notes_ = rhs;
  }
  virtual double read_position_x() override
  {
    return actual_x_;
  }
  virtual double read_position_y() override
  {
    return actual_y_;
  }
  virtual void move_x(double rhs) override
  {
    target_x_ = rhs;
  }
  virtual void move_y(double rhs) override
  {
    target_y_ = rhs;
  }
  virtual void act() override
  {
    actual_x_ = target_x_;
    actual_y_ = target_y_;
  }

private:
  double target_x_ = 0.;
  double target_y_ = 0.;
  
  double actual_x_ = 0.;
  double actual_y_ = 0.;

  std::string notes_;
};

int main(int argc, char* argv[])
{
  auto camera_factory = [](hula::cool_camera_properties const& p) {
    return std::make_unique<cool_camera>(p); 
  };
  auto stand_factory = [](hula::camera_stand_properties const& p) {
    return std::make_unique<camera_stand>(p);
  };

  return hula::register_and_run(argc, argv, camera_factory, stand_factory);
}

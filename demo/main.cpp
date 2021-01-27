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

  image<std::uint8_t> read_image() override
  {
    return {};
  }

  image<std::int32_t> read_raw_image() override
  {
    return {};
  }

  std::vector<std::int32_t> read_histogram() override
  {
    return {};
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

  std::string read_notes() override
  {
    return notes_;
  }

  void write_notes(std::string const& rhs) override
  {
    notes_ = rhs;
  }

  std::vector<double> read_position() override
  {
    return {actual_x_, actual_y_};
  }

  void write_position(std::vector<double> const& rhs) override
  {
    actual_x_ = rhs.at(0);
    actual_y_ = rhs.at(1);
  }

  void move(std::vector<double> const& rhs) override
  {
    target_x_ = rhs.at(0);
    target_y_ = rhs.at(1);
  }

  void act() override
  {
    actual_x_ = target_x_;
    actual_y_ = target_y_;
  }

  std::vector<std::int32_t> read_steps() override
  {
    return std::vector<std::int32_t>();
  }

  image<std::int32_t> read_label_image() override
  {
    return label_image_;
  }

  void write_label_image(image<std::int32_t> const& rhs) override
  {
    label_image_ = rhs;
  }

private:
  double target_x_ = 0.;
  double target_y_ = 0.;
  
  double actual_x_ = 0.;
  double actual_y_ = 0.;

  std::string notes_;
  image<std::int32_t> label_image_;
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

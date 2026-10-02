#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

#ifdef USE_OUTPUT
#include "esphome/components/output/float_output.h"
#endif

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

#include <vector>

namespace esphome {
namespace ad5593r {

class AD5593RComponent;

#ifdef USE_OUTPUT
class AD5593RChannel : public output::FloatOutput {
 public:
  AD5593RChannel(AD5593RComponent *parent, uint8_t channel) : parent_(parent), channel_(channel) {}
  void write_state(float state) override;
  uint8_t get_channel() const { return this->channel_; }

 protected:
  AD5593RComponent *parent_;
  uint8_t channel_;
};
#endif

#ifdef USE_SENSOR
class AD5593RSensor : public PollingComponent, public sensor::Sensor {
 public:
  AD5593RSensor(AD5593RComponent *parent, uint8_t channel) : parent_(parent), channel_(channel) {}
  void update() override;
  uint8_t get_channel() const { return this->channel_; }

 protected:
  AD5593RComponent *parent_;
  uint8_t channel_;
};
#endif

#ifdef USE_BINARY_SENSOR
class AD5593RBinarySensor : public binary_sensor::BinarySensor {
 public:
  AD5593RBinarySensor(AD5593RComponent *parent, uint8_t channel) : parent_(parent), channel_(channel) {}
  uint8_t get_channel() const { return this->channel_; }

 protected:
  AD5593RComponent *parent_;
  uint8_t channel_;
};
#endif

#ifdef USE_SWITCH
class AD5593RSwitch : public switch_::Switch {
 public:
  AD5593RSwitch(AD5593RComponent *parent, uint8_t channel) : parent_(parent), channel_(channel) {}
  void write_state(bool state) override;
  uint8_t get_channel() const { return this->channel_; }

 protected:
  AD5593RComponent *parent_;
  uint8_t channel_;
};
#endif

class AD5593RComponent : public Component, public i2c::I2CDevice {
 public:
  AD5593RComponent() = default;

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void loop() override;

  void set_gain_2x(bool gain_2x) { this->gain_2x_ = gain_2x; }
  void set_reference_voltage(float vref) { this->reference_voltage_ = vref; }

#ifdef USE_OUTPUT
  void register_dac_channel(AD5593RChannel *channel);
  bool write_dac(uint8_t channel, float level);
#endif

#ifdef USE_SENSOR
  void register_adc_sensor(AD5593RSensor *sensor);
  bool read_adc_voltage(uint8_t channel, float &voltage);
  bool read_temperature(float &temp_c);
#endif

#ifdef USE_BINARY_SENSOR
  void register_gpio_input(AD5593RBinarySensor *bs, bool pulldown);
#endif

#ifdef USE_SWITCH
  void register_gpio_output(AD5593RSwitch *sw);
  bool write_gpio(uint8_t channel, bool state);
#endif

  bool write_reg_16(uint8_t reg, uint16_t value);
  bool read_adc_raw(uint8_t channel, uint16_t &raw_value);
  bool read_gpio(uint8_t &state_mask);

 protected:
  float reference_voltage_{2.5f};
  bool gain_2x_{true};

  uint8_t dac_pin_mask_{0x00};
  uint8_t adc_pin_mask_{0x00};
  uint8_t gpio_out_mask_{0x00};
  uint8_t gpio_in_mask_{0x00};
  uint8_t pulldown_mask_{0x00};
  uint8_t gpio_out_state_{0x00};

  uint32_t last_gpio_read_{0};

#ifdef USE_BINARY_SENSOR
  std::vector<AD5593RBinarySensor *> binary_sensors_;
#endif
};

}  // namespace ad5593r
}  // namespace esphome

#include "ad5593r.h"
#include "esphome/core/log.h"
#include <cmath>
#include <algorithm>

namespace esphome {
namespace ad5593r {

static const char *const TAG = "ad5593r";

// AD5593R Control Registers (Mode 0000)
static const uint8_t AD5593R_REG_NOP             = 0x00;
static const uint8_t AD5593R_REG_ADC_SEQ         = 0x02;
static const uint8_t AD5593R_REG_GEN_CTRL        = 0x03;
static const uint8_t AD5593R_REG_ADC_CONFIG      = 0x04;
static const uint8_t AD5593R_REG_DAC_CONFIG      = 0x05;
static const uint8_t AD5593R_REG_PULLDOWN        = 0x06;
static const uint8_t AD5593R_REG_LDAC_MODE       = 0x07;
static const uint8_t AD5593R_REG_GPIO_OUT_CONFIG = 0x08;
static const uint8_t AD5593R_REG_GPIO_OUT_DATA   = 0x09;
static const uint8_t AD5593R_REG_GPIO_IN_CONFIG  = 0x0A;
static const uint8_t AD5593R_REG_POWER_REF_CTRL  = 0x0B;
static const uint8_t AD5593R_REG_OPEN_DRAIN      = 0x0C;
static const uint8_t AD5593R_REG_THREE_STATE     = 0x0D;
static const uint8_t AD5593R_REG_SW_RESET        = 0x0F;

// Direct Operation Commands
static const uint8_t AD5593R_CMD_DAC_WRITE       = 0x10;
static const uint8_t AD5593R_CMD_ADC_READ        = 0x40;
static const uint8_t AD5593R_CMD_GPIO_READ       = 0x60;

#ifdef USE_OUTPUT
AD5593RChannel::AD5593RChannel(AD5593RComponent *parent, uint8_t channel)
    : parent_(parent), channel_(channel) {}

void AD5593RChannel::write_state(float state) {
  this->parent_->write_dac(this->channel_, state);
}
#endif

#ifdef USE_SENSOR
AD5593RSensor::AD5593RSensor(AD5593RComponent *parent, uint8_t channel)
    : parent_(parent), channel_(channel) {}

void AD5593RSensor::update() {
  if (this->parent_->is_failed()) {
    return;
  }

  if (this->channel_ == 8) {
    float temp_c;
    if (this->parent_->read_temperature(temp_c)) {
      this->publish_state(temp_c);
    } else {
      this->status_set_warning();
    }
  } else {
    float voltage;
    if (this->parent_->read_adc_voltage(this->channel_, voltage)) {
      this->publish_state(voltage);
    } else {
      this->status_set_warning();
    }
  }
}
#endif

#ifdef USE_BINARY_SENSOR
AD5593RBinarySensor::AD5593RBinarySensor(AD5593RComponent *parent, uint8_t channel)
    : parent_(parent), channel_(channel) {}
#endif

#ifdef USE_SWITCH
AD5593RSwitch::AD5593RSwitch(AD5593RComponent *parent, uint8_t channel)
    : parent_(parent), channel_(channel) {}

void AD5593RSwitch::write_state(bool state) {
  if (this->parent_->write_gpio(this->channel_, state)) {
    this->publish_state(state);
  }
}
#endif

bool AD5593RComponent::write_reg_16(uint8_t reg, uint16_t value) {
  uint8_t data[2] = {static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF)};
  return this->write_bytes(reg, data, 2);
}

void AD5593RComponent::setup() {
  ESP_LOGCONFIG(TAG, "Initializing AD5593R at I2C address 0x%02X...", this->address_);

  // 1. Software Reset
  if (!this->write_reg_16(AD5593R_REG_SW_RESET, 0x0AC5)) {
    ESP_LOGE(TAG, "Communication failed: AD5593R did not respond to software reset!");
    this->mark_failed();
    return;
  }
  delay(5);

  // 2. Power-down / Reference Control: Enable internal 2.5V reference (bit 9)
  if (!this->write_reg_16(AD5593R_REG_POWER_REF_CTRL, 0x0200)) {
    ESP_LOGE(TAG, "Failed to enable internal reference on AD5593R!");
    this->mark_failed();
    return;
  }
  delay(2);

  // 3. General-Purpose Control: ADC buffer enabled (bit 8), Gain 2x for ADC (bit 5) & DAC (bit 4)
  uint16_t gen_ctrl = 0x0100;
  if (this->gain_2x_) {
    gen_ctrl |= 0x0030;
  }
  if (!this->write_reg_16(AD5593R_REG_GEN_CTRL, gen_ctrl)) {
    ESP_LOGE(TAG, "Failed to configure General-Purpose Control register!");
    this->mark_failed();
    return;
  }

  // 4. Configure DAC pins
  if (this->dac_pin_mask_ != 0) {
    if (!this->write_reg_16(AD5593R_REG_DAC_CONFIG, this->dac_pin_mask_)) {
      ESP_LOGE(TAG, "Failed to configure DAC pins (mask: 0x%02X)!", this->dac_pin_mask_);
      this->mark_failed();
      return;
    }
    // Initialize active DAC outputs to 0V
    for (uint8_t ch = 0; ch < 8; ch++) {
      if (this->dac_pin_mask_ & (1 << ch)) {
        this->write_dac(ch, 0.0f);
      }
    }
  }

  // 5. Configure ADC pins
  if (this->adc_pin_mask_ != 0) {
    if (!this->write_reg_16(AD5593R_REG_ADC_CONFIG, this->adc_pin_mask_)) {
      ESP_LOGE(TAG, "Failed to configure ADC pins (mask: 0x%02X)!", this->adc_pin_mask_);
      this->mark_failed();
      return;
    }
  }

  // 6. Configure GPIO Outputs
  if (this->gpio_out_mask_ != 0) {
    if (!this->write_reg_16(AD5593R_REG_GPIO_OUT_CONFIG, this->gpio_out_mask_)) {
      ESP_LOGE(TAG, "Failed to configure GPIO outputs (mask: 0x%02X)!", this->gpio_out_mask_);
      this->mark_failed();
      return;
    }
    this->write_reg_16(AD5593R_REG_GPIO_OUT_DATA, this->gpio_out_state_);
  }

  // 7. Configure GPIO Inputs
  if (this->gpio_in_mask_ != 0) {
    if (!this->write_reg_16(AD5593R_REG_GPIO_IN_CONFIG, this->gpio_in_mask_)) {
      ESP_LOGE(TAG, "Failed to configure GPIO inputs (mask: 0x%02X)!", this->gpio_in_mask_);
      this->mark_failed();
      return;
    }
  }

  // 8. Configure Pull-down resistors
  if (this->pulldown_mask_ != 0) {
    if (!this->write_reg_16(AD5593R_REG_PULLDOWN, this->pulldown_mask_)) {
      ESP_LOGE(TAG, "Failed to configure pull-downs (mask: 0x%02X)!", this->pulldown_mask_);
      this->mark_failed();
      return;
    }
  }

  ESP_LOGCONFIG(TAG, "AD5593R initialized successfully. Gain=%s (0-%.1fV)",
                this->gain_2x_ ? "2x" : "1x",
                this->gain_2x_ ? (this->reference_voltage_ * 2.0f) : this->reference_voltage_);
}

void AD5593RComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "AD5593R:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication with AD5593R failed!");
    return;
  }
  ESP_LOGCONFIG(TAG, "  Reference Voltage: %.2f V", this->reference_voltage_);
  ESP_LOGCONFIG(TAG, "  Gain Mode: %s (Max: %.2f V)",
                this->gain_2x_ ? "2x" : "1x",
                this->gain_2x_ ? (this->reference_voltage_ * 2.0f) : this->reference_voltage_);
  ESP_LOGCONFIG(TAG, "  DAC Pins Mask: 0x%02X", this->dac_pin_mask_);
  ESP_LOGCONFIG(TAG, "  ADC Pins Mask: 0x%02X", this->adc_pin_mask_);
  ESP_LOGCONFIG(TAG, "  GPIO Outputs Mask: 0x%02X", this->gpio_out_mask_);
  ESP_LOGCONFIG(TAG, "  GPIO Inputs Mask: 0x%02X", this->gpio_in_mask_);
  ESP_LOGCONFIG(TAG, "  Pull-Down Mask: 0x%02X", this->pulldown_mask_);
}

void AD5593RComponent::loop() {
#ifdef USE_BINARY_SENSOR
  if (this->is_failed() || this->binary_sensors_.empty()) {
    return;
  }

  uint32_t now = millis();
  if (now - this->last_gpio_read_ < 30) {
    return;
  }
  this->last_gpio_read_ = now;

  uint8_t pin_states = 0;
  if (!this->read_gpio(pin_states)) {
    return;
  }

  for (auto *bs : this->binary_sensors_) {
    bool state = (pin_states & (1 << bs->get_channel())) != 0;
    if (!bs->has_state() || bs->state != state) {
      bs->publish_state(state);
    }
  }
#endif
}

#ifdef USE_OUTPUT
void AD5593RComponent::register_dac_channel(AD5593RChannel *channel) {
  if (channel->get_channel() < 8) {
    this->dac_pin_mask_ |= (1 << channel->get_channel());
  }
}

bool AD5593RComponent::write_dac(uint8_t channel, float level) {
  if (this->is_failed() || channel > 7) {
    return false;
  }

  float clamped = std::max(0.0f, std::min(1.0f, level));
  uint16_t code = static_cast<uint16_t>(std::round(clamped * 4095.0f));
  if (code > 4095) {
    code = 4095;
  }

  uint16_t val = (static_cast<uint16_t>(channel & 0x07) << 12) | (code & 0x0FFF);
  uint8_t data[2] = {static_cast<uint8_t>(val >> 8), static_cast<uint8_t>(val & 0xFF)};
  uint8_t cmd = AD5593R_CMD_DAC_WRITE | (channel & 0x07);

  return this->write_bytes(cmd, data, 2);
}
#endif

#ifdef USE_SENSOR
void AD5593RComponent::register_adc_sensor(AD5593RSensor *sensor) {
  uint8_t ch = sensor->get_channel();
  if (ch < 8) {
    this->adc_pin_mask_ |= (1 << ch);
  }
}

bool AD5593RComponent::read_adc_raw(uint8_t channel, uint16_t &raw_value) {
  if (this->is_failed()) {
    return false;
  }

  // Select sequence
  uint16_t seq = (channel == 8) ? (1 << 8) : (1 << channel);
  if (!this->write_reg_16(AD5593R_REG_ADC_SEQ, seq)) {
    ESP_LOGW(TAG, "Failed to write ADC sequence register for channel %u", channel);
    return false;
  }

  delayMicroseconds(5);

  uint8_t buf[2];
  if (!this->read_bytes(AD5593R_CMD_ADC_READ, buf, 2)) {
    ESP_LOGW(TAG, "Failed to read ADC data for channel %u", channel);
    return false;
  }

  raw_value = ((static_cast<uint16_t>(buf[0]) & 0x0F) << 8) | buf[1];
  return true;
}

bool AD5593RComponent::read_adc_voltage(uint8_t channel, float &voltage) {
  if (channel > 7) {
    return false;
  }
  uint16_t raw;
  if (!this->read_adc_raw(channel, raw)) {
    return false;
  }
  float max_v = this->gain_2x_ ? (this->reference_voltage_ * 2.0f) : this->reference_voltage_;
  voltage = (static_cast<float>(raw) / 4095.0f) * max_v;
  return true;
}

bool AD5593RComponent::read_temperature(float &temp_c) {
  uint16_t raw;
  if (!this->read_adc_raw(8, raw)) {
    return false;
  }
  if (this->gain_2x_) {
    temp_c = 25.0f + (static_cast<float>(raw) - 409.5f) / 1.327f;
  } else {
    temp_c = 25.0f + (static_cast<float>(raw) - 819.0f) / 2.654f;
  }
  return true;
}
#endif

#ifdef USE_BINARY_SENSOR
void AD5593RComponent::register_gpio_input(AD5593RBinarySensor *bs, bool pulldown) {
  uint8_t ch = bs->get_channel();
  if (ch < 8) {
    this->gpio_in_mask_ |= (1 << ch);
    if (pulldown) {
      this->pulldown_mask_ |= (1 << ch);
    }
    this->binary_sensors_.push_back(bs);
  }
}

bool AD5593RComponent::read_gpio(uint8_t &state_mask) {
  if (this->is_failed()) {
    return false;
  }
  uint8_t buf[2];
  if (!this->read_bytes(AD5593R_CMD_GPIO_READ, buf, 2)) {
    return false;
  }
  state_mask = buf[1];
  return true;
}
#endif

#ifdef USE_SWITCH
void AD5593RComponent::register_gpio_output(AD5593RSwitch *sw) {
  uint8_t ch = sw->get_channel();
  if (ch < 8) {
    this->gpio_out_mask_ |= (1 << ch);
  }
}

bool AD5593RComponent::write_gpio(uint8_t channel, bool state) {
  if (this->is_failed() || channel > 7) {
    return false;
  }
  if (state) {
    this->gpio_out_state_ |= (1 << channel);
  } else {
    this->gpio_out_state_ &= ~(1 << channel);
  }
  return this->write_reg_16(AD5593R_REG_GPIO_OUT_DATA, this->gpio_out_state_);
}
#endif

}  // namespace ad5593r
}  // namespace esphome

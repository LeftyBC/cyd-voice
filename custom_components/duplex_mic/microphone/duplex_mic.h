#pragma once

#include "esphome/core/component.h"
#include "esphome/components/microphone/microphone.h"
#include "driver/i2s_std.h"

namespace esphome {
namespace duplex_mic {

class DuplexMicrophone : public microphone::Microphone, public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void start() override;
  void stop() override;

  void set_rx_handle(i2s_chan_handle_t handle) { this->rx_handle_ = handle; }

 protected:
  static void mic_task_(void *param);

  i2s_chan_handle_t rx_handle_{nullptr};
  TaskHandle_t task_handle_{nullptr};
  volatile bool task_running_{false};
};

}  // namespace duplex_mic
}  // namespace esphome

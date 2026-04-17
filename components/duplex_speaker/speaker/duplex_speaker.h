#pragma once

#include "esphome/core/component.h"
#include "esphome/components/speaker/speaker.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

namespace esphome {
namespace duplex_speaker {

class DuplexSpeaker : public speaker::Speaker, public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  size_t play(const uint8_t *data, size_t length, TickType_t ticks_to_wait) override;
  size_t play(const uint8_t *data, size_t length) override;
  void start() override;
  void stop() override;
  void finish() override;
  bool has_buffered_data() const override;

  void set_tx_handle(i2s_chan_handle_t handle) { this->tx_handle_ = handle; }

 protected:
  static void speaker_task_(void *param);

  i2s_chan_handle_t tx_handle_{nullptr};
  RingbufHandle_t ring_buffer_{nullptr};
  TaskHandle_t task_handle_{nullptr};
  volatile bool task_running_{false};
  volatile bool finish_requested_{false};
  static const size_t RING_BUFFER_SIZE = 32768;  // 32KB ~1s @ 16kHz 16bit mono
};

}  // namespace duplex_speaker
}  // namespace esphome

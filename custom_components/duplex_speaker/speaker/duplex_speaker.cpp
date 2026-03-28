#include "duplex_speaker.h"
#include "esphome/core/log.h"

namespace esphome {
namespace duplex_speaker {

static const char *const TAG = "duplex_speaker";

void DuplexSpeaker::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Duplex Speaker...");
  this->audio_stream_info_ = audio::AudioStreamInfo(16, 1, 16000);
  this->ring_buffer_ = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
  if (this->ring_buffer_ == nullptr) {
    ESP_LOGE(TAG, "Failed to create ring buffer");
    this->mark_failed();
  }
}

void DuplexSpeaker::start() {
  if (this->state_ == speaker::STATE_RUNNING)
    return;
  if (this->tx_handle_ == nullptr) {
    ESP_LOGE(TAG, "TX handle not set!");
    return;
  }
  this->state_ = speaker::STATE_STARTING;
  this->finish_requested_ = false;
  this->task_running_ = true;

  // Enable amplifier (GPIO1, active LOW)
  gpio_set_level(GPIO_NUM_1, 0);

  xTaskCreatePinnedToCore(speaker_task_, "duplex_spk", 4096, this, 5, &this->task_handle_, 0);
  this->state_ = speaker::STATE_RUNNING;
  ESP_LOGD(TAG, "Speaker started");
}

void DuplexSpeaker::stop() {
  if (this->state_ == speaker::STATE_STOPPED)
    return;
  this->state_ = speaker::STATE_STOPPING;
  this->task_running_ = false;
  if (this->task_handle_ != nullptr) {
    vTaskDelay(pdMS_TO_TICKS(200));
    this->task_handle_ = nullptr;
  }
  // Disable amplifier
  gpio_set_level(GPIO_NUM_1, 1);
  this->state_ = speaker::STATE_STOPPED;
  ESP_LOGD(TAG, "Speaker stopped");
}

void DuplexSpeaker::finish() {
  this->finish_requested_ = true;
  // Task will stop when ring buffer is empty
}

size_t DuplexSpeaker::play(const uint8_t *data, size_t length, TickType_t ticks_to_wait) {
  if (this->state_ != speaker::STATE_RUNNING) {
    this->start();
  }
  // Send to ring buffer
  if (xRingbufferSend(this->ring_buffer_, data, length, ticks_to_wait) == pdTRUE) {
    return length;
  }
  // Ring buffer full - try partial write
  for (size_t i = 0; i < length; i += 256) {
    size_t chunk = std::min((size_t)256, length - i);
    if (xRingbufferSend(this->ring_buffer_, data + i, chunk, ticks_to_wait) != pdTRUE) {
      return i;
    }
  }
  return length;
}

size_t DuplexSpeaker::play(const uint8_t *data, size_t length) {
  return this->play(data, length, pdMS_TO_TICKS(100));
}

bool DuplexSpeaker::has_buffered_data() const {
  if (this->ring_buffer_ == nullptr) return false;
  UBaseType_t items_waiting = 0;
  vRingbufferGetInfo(this->ring_buffer_, nullptr, nullptr, nullptr, nullptr, &items_waiting);
  return items_waiting > 0;
}

void DuplexSpeaker::loop() {}

void DuplexSpeaker::speaker_task_(void *param) {
  auto *self = static_cast<DuplexSpeaker *>(param);
  uint32_t idle_count = 0;

  while (self->task_running_) {
    size_t item_size = 0;
    void *item = xRingbufferReceiveUpTo(self->ring_buffer_, &item_size, pdMS_TO_TICKS(50), 960);

    if (item != nullptr && item_size > 0) {
      idle_count = 0;
      size_t bytes_written = 0;
      i2s_channel_write(self->tx_handle_, item, item_size, &bytes_written, pdMS_TO_TICKS(200));
      vRingbufferReturnItem(self->ring_buffer_, item);
    } else {
      idle_count++;
      // If finish requested and no more data, stop
      if (self->finish_requested_ && idle_count > 5) {
        break;
      }
    }
  }

  // Disable amplifier
  gpio_set_level(GPIO_NUM_1, 1);
  self->state_ = speaker::STATE_STOPPED;
  self->task_handle_ = nullptr;
  ESP_LOGD(TAG, "Speaker task ended");
  vTaskDelete(NULL);
}

}  // namespace duplex_speaker
}  // namespace esphome

#include "duplex_mic.h"
#include "esphome/core/log.h"

namespace esphome {
namespace duplex_mic {

static const char *const TAG = "duplex_mic";

void DuplexMicrophone::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Duplex Microphone...");
  this->audio_stream_info_ = audio::AudioStreamInfo(16, 1, 16000);
}

void DuplexMicrophone::start() {
  if (this->state_ == microphone::STATE_RUNNING)
    return;
  if (this->rx_handle_ == nullptr) {
    ESP_LOGE(TAG, "RX handle not set!");
    return;
  }
  this->state_ = microphone::STATE_STARTING;
  this->task_running_ = true;
  xTaskCreatePinnedToCore(mic_task_, "duplex_mic", 4096, this, 5, &this->task_handle_, 0);
  this->state_ = microphone::STATE_RUNNING;
  ESP_LOGD(TAG, "Microphone started");
}

void DuplexMicrophone::stop() {
  if (this->state_ == microphone::STATE_STOPPED)
    return;
  this->state_ = microphone::STATE_STOPPING;
  this->task_running_ = false;
  if (this->task_handle_ != nullptr) {
    vTaskDelay(pdMS_TO_TICKS(200));
    this->task_handle_ = nullptr;
  }
  this->state_ = microphone::STATE_STOPPED;
  ESP_LOGD(TAG, "Microphone stopped");
}

void DuplexMicrophone::loop() {}

void DuplexMicrophone::mic_task_(void *param) {
  auto *self = static_cast<DuplexMicrophone *>(param);
  std::vector<uint8_t> buffer(960);  // 30ms @ 16kHz 16bit mono

  while (self->task_running_) {
    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(self->rx_handle_, buffer.data(), buffer.size(),
                                     &bytes_read, pdMS_TO_TICKS(100));
    if (err == ESP_OK && bytes_read > 0) {
      buffer.resize(bytes_read);
      self->data_callbacks_.call(buffer);
      buffer.resize(960);
    }
  }
  vTaskDelete(NULL);
}

}  // namespace duplex_mic
}  // namespace esphome

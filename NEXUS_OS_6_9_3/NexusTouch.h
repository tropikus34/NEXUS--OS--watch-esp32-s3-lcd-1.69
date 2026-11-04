#pragma once
// NEXUS 6.9.5 — optional CST816T interaction layer.
// Device mapping: Waveshare ESP32-S3-LCD-1.69 / noahclark556 project.
// A detected touch chip is not required: button navigation remains functional.
// Never poll the touch controller while LCD is asleep (light-sleep friendly).
#include <Arduino.h>
#include <Wire.h>

#ifndef NEXUS_ENABLE_TOUCH
#define NEXUS_ENABLE_TOUCH 1
#endif

namespace NexusTouch {
static constexpr uint8_t ADDRESS = 0x15;
static constexpr uint16_t WIDTH = 240, HEIGHT = 280;
static bool present = false, down = false, longDelivered = false;
static uint16_t startX = 0, startY = 0, lastX = 0, lastY = 0;
static uint32_t lastPoll = 0, startMs = 0, lastAction = 0, lastProbe = 0;
static uint8_t consecutiveErrors = 0;

inline bool readPoint(uint16_t& x, uint16_t& y, bool& pressed) {
  Wire.beginTransmission(ADDRESS);
  Wire.write((uint8_t)0x02);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)ADDRESS, 5) != 5) return false;
  const uint8_t fingers = Wire.read();
  const uint8_t xh = Wire.read(), xl = Wire.read(), yh = Wire.read(), yl = Wire.read();
  pressed = (fingers & 0x0F) != 0;
  x = (uint16_t(xh & 0x0F) << 8) | xl;
  y = (uint16_t(yh & 0x0F) << 8) | yl;
  return true;
}

inline void begin() {
#if NEXUS_ENABLE_TOUCH
  // Wire.begin() must already have completed. No reset that might change the
  // IMU/RTC bus or disturb wake configuration.
  Wire.beginTransmission(ADDRESS);
  present = Wire.endTransmission() == 0;
  Serial.println(present ? "NEXUS TOUCH: CST816T found" : "NEXUS TOUCH: absent (button only)");
#else
  present = false;
#endif
}

// Action callbacks are application-owned. Single tap activates the highlighted
// item (equivalent to a button hold in NAV), a held touch advances the selected
// field, and horizontal swipes work in both NAV and EDIT modes.
// Return true if an action ran, so the caller can refresh the inactivity timer.
inline bool service(bool awake, bool enabled, bool editing,
                    void (*onNext)(), void (*onPrev)(),
                    void (*onTap)(), void (*onHold)()) {
#if NEXUS_ENABLE_TOUCH
  if (!present || !awake || !enabled) { down = false; return false; }
  const uint32_t now = millis();
  if ((uint32_t)(now - lastPoll) < 35) return false;
  lastPoll = now;
  uint16_t x = 0, y = 0; bool pressed = false;
  if (!readPoint(x, y, pressed)) {
    down = false;
    if (++consecutiveErrors >= 8) {
      present = false; // Don't stall the I2C bus indefinitely if touch is unplugged.
      lastProbe = now;
    }
    return false;
  }
  consecutiveErrors = 0;
  if (pressed) {
    if (x >= WIDTH || y >= HEIGHT) { down = false; return false; }
    if (!down) {
      down = true; longDelivered = false;
      startX = lastX = x; startY = lastY = y; startMs = now;
    } else {
      lastX = x; lastY = y;
      if (!longDelivered && (uint32_t)(now - startMs) >= 750 &&
          abs((int)x - (int)startX) < 22 && abs((int)y - (int)startY) < 22 &&
          (lastAction == 0 || (uint32_t)(now - lastAction) >= 250)) {
        longDelivered = true;
        lastAction = now;
        if (onHold) onHold();
        return true;
      }
    }
    return false;
  }
  if (!down) return false;
  down = false;
  if (longDelivered) return false;
  const uint32_t duration = now - startMs;
  const int dx = (int)lastX - (int)startX;
  const int dy = (int)lastY - (int)startY;
  if ((lastAction != 0 && (uint32_t)(now - lastAction) < 180) || duration > 1100) return false;
  if (abs(dx) >= 48 && abs(dx) >= (abs(dy) * 3) / 2) {
    lastAction = now;
    if (dx < 0) { if (onNext) onNext(); }
    else { if (onPrev) onPrev(); }
    return true;
  }
  if (abs(dx) <= 18 && abs(dy) <= 18 && duration <= 550) {
    lastAction = now;
    if (onTap) onTap();
    return true;
  }
  (void)editing; // Caller chooses per-mode callbacks; held for ABI clarity.
#else
  (void)awake; (void)enabled; (void)editing;
  (void)onNext; (void)onPrev; (void)onTap; (void)onHold;
#endif
  return false;
}

inline void serviceReconnect(bool awake) {
#if NEXUS_ENABLE_TOUCH
  // Non-blocking recovery only while awake; no periodic I2C traffic in sleep.
  if (present || !awake) return;
  const uint32_t now = millis();
  if ((uint32_t)(now - lastProbe) < 10000) return;
  lastProbe = now;
  Wire.beginTransmission(ADDRESS);
  present = Wire.endTransmission() == 0;
  consecutiveErrors = 0;
  if (present) Serial.println("NEXUS TOUCH: reconnected");
#else
  (void)awake;
#endif
}
}  // namespace NexusTouch

/*
 * HexMote Firmware -- v0.4
 * SuperMini nRF52840 + ICM-20948 + ALPS joystick
 *
 * v0.4 switches to the official Adafruit BLEHidGamepad class instead of a
 * hand-rolled BLEHidGeneric descriptor -- caught real compile errors in the
 * v0.3 approach (setReportLen/setReportDescriptor signatures were wrong).
 * This is now built on the library's own tested gamepad implementation.
 *
 * v0.3 added Serial debug output (gated by DEBUG_SERIAL below) so you can
 * verify buttons, joystick, and IMU are alive over USB before trusting the
 * BLE side. Flip DEBUG_SERIAL to 0 once everything checks out.
 *
 * Architecture (locked in):
 *   - Standard BLE HID gamepad (buttons + stick) -> works natively in Steam
 *     Input, Windows, Linux, anywhere -- zero extra software required.
 *   - Separate custom BLE GATT service streaming raw accel+gyro -> read by
 *     your Steam Deck DSU companion app, which relays it to Dolphin/Cemu/
 *     Yuzu as a standard DSU (cemuhook) motion source.
 *   - Steam Input gyro (native, non-emulator games) is NOT covered by this
 *     path -- that's a separate DS4-identity-spoof problem, not built here.
 *
 * Pin mapping (from hexmote_pcb.kicad_sch):
 *   A=P0.09  B=P1.11  X=P1.13  Y=P1.15
 *   L=P0.06  R=P0.17  +=P0.20  -=P0.08
 *   Joystick click (SEL) = P1.00
 *   Joystick X (AIN7) = P0.31   Joystick Y (AIN0) = P0.02
 *   I2C: SCL=P0.11  SDA=P1.04  IMU INT=P1.06 (unused, poll-only for now)
 *
 * Charging: fully hardware (onboard SuperMini charge IC + SW10 power
 * switch). No firmware involvement.
 *
 * !! IMPORTANT: the motion GATT UUIDs and 12-byte payload layout below are
 * placeholders I defined myself. I don't have visibility into what your
 * specific Steam Deck DSU companion app expects on the BLE side -- you'll
 * need to check its docs/source and either match its expected UUID/format,
 * or adjust the app's parsing to match what's below. Flagging this clearly
 * so it's not a silent assumption.
 *
 * Compile-UNTESTED (no nRF52 toolchain in this sandbox). Pin assignments
 * verified against the schematic; BLE/IMU boilerplate is standard-pattern
 * but give it a real smoke test before trusting it.
 */

#include <bluefruit.h>
#include <Wire.h>
#include <nrf_gpio.h>
#include <nrf_saadc.h>

// ---------------- Debug scaffolding ----------------
// Flip to 0 once everything's verified -- Serial prints cost a little loop
// time and there's no reason to keep them running once BLE is trusted.
#define DEBUG_SERIAL 1

#if DEBUG_SERIAL
  #define DBG_BEGIN(baud)   Serial.begin(baud)
  #define DBG_PRINT(...)    Serial.print(__VA_ARGS__)
  #define DBG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
  #define DBG_BEGIN(baud)
  #define DBG_PRINT(...)
  #define DBG_PRINTLN(...)
#endif

// ---------------- Pin definitions (raw GPIO numbers) ----------------
#define PIN_A     NRF_GPIO_PIN_MAP(0, 9)
#define PIN_B     NRF_GPIO_PIN_MAP(1, 11)
#define PIN_X     NRF_GPIO_PIN_MAP(1, 13)
#define PIN_Y     NRF_GPIO_PIN_MAP(1, 15)
#define PIN_L     NRF_GPIO_PIN_MAP(0, 6)
#define PIN_R     NRF_GPIO_PIN_MAP(0, 17)
#define PIN_PLUS  NRF_GPIO_PIN_MAP(0, 20)
#define PIN_MINUS NRF_GPIO_PIN_MAP(0, 8)
#define PIN_SEL   NRF_GPIO_PIN_MAP(1, 0)   // joystick click

#define PIN_SDA   NRF_GPIO_PIN_MAP(1, 4)
#define PIN_SCL   NRF_GPIO_PIN_MAP(0, 11)

#define AIN_JOY_X  7   // P0.31
#define AIN_JOY_Y  0   // P0.02

// ---------------- ICM-20948 register map (minimal subset) ----------------
#define ICM_ADDR        0x69   // change to 0x68 if your breakout pulls AD0 low
#define REG_BANK_SEL    0x7F
#define B0_WHO_AM_I     0x00
#define B0_PWR_MGMT_1   0x06
#define B0_ACCEL_XOUT_H 0x2D
#define B0_GYRO_XOUT_H  0x33
#define B2_GYRO_CFG_1   0x01
#define B2_ACCEL_CFG    0x14

bool icmSelectBank(uint8_t bank) {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(REG_BANK_SEL);
  Wire.write(bank << 4);
  return Wire.endTransmission() == 0;
}

bool icmWriteReg(uint8_t bank, uint8_t reg, uint8_t val) {
  icmSelectBank(bank);
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool icmReadRegs(uint8_t bank, uint8_t reg, uint8_t *buf, uint8_t len) {
  icmSelectBank(bank);
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(ICM_ADDR, (int)len);
  for (uint8_t i = 0; i < len && Wire.available(); i++) buf[i] = Wire.read();
  return true;
}

uint8_t icmLastWhoAmI = 0; // stashed for debug printing after icmInit()

bool icmInit() {
  uint8_t who = 0;
  icmReadRegs(0, B0_WHO_AM_I, &who, 1);
  icmLastWhoAmI = who;
  if (who != 0xEA) return false; // wrong address or not detected -- check ICM_ADDR/wiring

  icmWriteReg(0, B0_PWR_MGMT_1, 0x01);   // wake, auto clock select
  delay(10);
  icmWriteReg(2, B2_GYRO_CFG_1, 0x00);   // +/-250 dps, no DLPF (default)
  icmWriteReg(2, B2_ACCEL_CFG, 0x00);    // +/-2g, no DLPF (default)
  return true;
}

void icmReadMotion(int16_t &ax, int16_t &ay, int16_t &az,
                    int16_t &gx, int16_t &gy, int16_t &gz) {
  uint8_t raw[6];
  icmReadRegs(0, B0_ACCEL_XOUT_H, raw, 6);
  ax = (int16_t)((raw[0] << 8) | raw[1]);
  ay = (int16_t)((raw[2] << 8) | raw[3]);
  az = (int16_t)((raw[4] << 8) | raw[5]);

  icmReadRegs(0, B0_GYRO_XOUT_H, raw, 6);
  gx = (int16_t)((raw[0] << 8) | raw[1]);
  gy = (int16_t)((raw[2] << 8) | raw[3]);
  gz = (int16_t)((raw[4] << 8) | raw[5]);
}

// ---------------- Minimal blocking SAADC read ----------------
uint16_t analogReadRaw(uint8_t ain) {
  NRF_SAADC->ENABLE = 1;
  NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_10bit;

  NRF_SAADC->CH[0].CONFIG =
      (SAADC_CH_CONFIG_GAIN_Gain1_4 << SAADC_CH_CONFIG_GAIN_Pos) |
      (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
      (SAADC_CH_CONFIG_TACQ_20us << SAADC_CH_CONFIG_TACQ_Pos) |
      (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos);
  NRF_SAADC->CH[0].PSELP = ain + 1; // AIN0..AIN7 map to PSELP 1..8
  NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC;

  static int16_t result;
  NRF_SAADC->RESULT.PTR = (uint32_t)&result;
  NRF_SAADC->RESULT.MAXCNT = 1;

  NRF_SAADC->TASKS_START = 1;
  while (!NRF_SAADC->EVENTS_STARTED);
  NRF_SAADC->EVENTS_STARTED = 0;

  NRF_SAADC->TASKS_SAMPLE = 1;
  while (!NRF_SAADC->EVENTS_END);
  NRF_SAADC->EVENTS_END = 0;

  NRF_SAADC->TASKS_STOP = 1;
  while (!NRF_SAADC->EVENTS_STOPPED);
  NRF_SAADC->EVENTS_STOPPED = 0;

  NRF_SAADC->ENABLE = 0;
  if (result < 0) result = 0;
  return (uint16_t)result; // 0-1023 (10-bit)
}

// ---------------- BLE HID gamepad (official Adafruit BLEHidGamepad class) ----------------
// Using the library's own gamepad implementation instead of a hand-rolled
// custom HID descriptor -- far less surface area for the kind of API
// mismatches that bit us with the earlier BLEHidGeneric approach.
BLEDis bledis;
BLEHidGamepad blegamepad;
hid_gamepad_report_t gp; // defined in hid.h via Adafruit_TinyUSB, pulled in by bluefruit.h

// Button bit assignments within gp.buttons (bitmask, up to 32 buttons supported)
#define BTN_A     (1UL << 0)
#define BTN_B     (1UL << 1)
#define BTN_X     (1UL << 2)
#define BTN_Y     (1UL << 3)
#define BTN_L     (1UL << 4)
#define BTN_R     (1UL << 5)
#define BTN_PLUS  (1UL << 6)
#define BTN_MINUS (1UL << 7)
#define BTN_SEL   (1UL << 8)

// ---------------- Custom motion GATT service ----------------
// PLACEHOLDER UUIDs -- verify/match against your DSU companion app.
const uint8_t MOTION_SVC_UUID[16]    = {0x9E,0x5D,0x80,0xE0,0x25,0x08,0x4B,0x92,0x8B,0x2F,0x4B,0x1C,0x01,0x00,0x0F,0xA0};
const uint8_t MOTION_CHAR_UUID[16]   = {0x9E,0x5D,0x80,0xE0,0x25,0x08,0x4B,0x92,0x8B,0x2F,0x4B,0x1C,0x02,0x00,0x0F,0xA0};

BLEService        motionService(MOTION_SVC_UUID);
BLECharacteristic motionChar(MOTION_CHAR_UUID);

#pragma pack(push, 1)
struct HexMotionPacket {
  int16_t ax, ay, az;   // raw accel, +/-2g range, LSB per ICM-20948 datasheet
  int16_t gx, gy, gz;   // raw gyro, +/-250 dps range, LSB per ICM-20948 datasheet
};
#pragma pack(pop)

HexMotionPacket motionPacket;

void setupMotionService() {
  motionService.begin();

  motionChar.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  motionChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  motionChar.setFixedLen(sizeof(HexMotionPacket));
  motionChar.begin();
}

// ---------------- BLE setup ----------------
void setupBLE() {
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("HexMote");

  bledis.setManufacturer("hephaestushex");
  bledis.setModel("HexMote v0.4");
  bledis.begin();

  blegamepad.begin();

  setupMotionService();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_GAMEPAD);
  Bluefruit.Advertising.addService(blegamepad);
  Bluefruit.Advertising.addService(motionService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

// ---------------- Buttons ----------------
void setupButtons() {
  int pins[] = {PIN_A, PIN_B, PIN_X, PIN_Y, PIN_L, PIN_R, PIN_PLUS, PIN_MINUS, PIN_SEL};
  for (int p : pins) nrf_gpio_cfg_input(p, NRF_GPIO_PIN_PULLUP);
}

uint32_t readButtons() {
  uint32_t b = 0;
  if (!nrf_gpio_pin_read(PIN_A))     b |= BTN_A;
  if (!nrf_gpio_pin_read(PIN_B))     b |= BTN_B;
  if (!nrf_gpio_pin_read(PIN_X))     b |= BTN_X;
  if (!nrf_gpio_pin_read(PIN_Y))     b |= BTN_Y;
  if (!nrf_gpio_pin_read(PIN_L))     b |= BTN_L;
  if (!nrf_gpio_pin_read(PIN_R))     b |= BTN_R;
  if (!nrf_gpio_pin_read(PIN_PLUS))  b |= BTN_PLUS;
  if (!nrf_gpio_pin_read(PIN_MINUS)) b |= BTN_MINUS;
  if (!nrf_gpio_pin_read(PIN_SEL))   b |= BTN_SEL;
  return b;
}

bool icmReady = false;
uint32_t lastMotionMs = 0;
const uint32_t MOTION_INTERVAL_MS = 10; // ~100Hz motion stream

void setup() {
  DBG_BEGIN(115200);
#if DEBUG_SERIAL
  uint32_t serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart < 3000)) { /* give USB CDC a few seconds, don't hang forever if no monitor is open */ }
#endif
  DBG_PRINTLN();
  DBG_PRINTLN(F("=== HexMote booting ==="));

  setupButtons();
  DBG_PRINTLN(F("Buttons configured (pull-ups enabled)"));

  Wire.setPins(PIN_SDA, PIN_SCL);
  Wire.begin();
  Wire.setClock(400000);

  icmReady = icmInit(); // false -> motion packet stays zero, check wiring/address
  DBG_PRINT(F("ICM-20948 WHO_AM_I = 0x"));
  DBG_PRINTLN(String(icmLastWhoAmI, HEX));
  DBG_PRINTLN(icmReady ? F("ICM-20948 init OK") : F("ICM-20948 init FAILED -- check I2C wiring/address (try 0x68 if 0x69 fails)"));

  setupBLE();
  DBG_PRINTLN(F("BLE advertising started as 'HexMote'"));
  DBG_PRINTLN(F("=== Setup complete ==="));
}

#if DEBUG_SERIAL
uint32_t lastDebugMs = 0;
const uint32_t DEBUG_INTERVAL_MS = 250; // 4x/sec -- readable without flooding the monitor

void printDebugLine(uint32_t buttons, uint16_t rawX, uint16_t rawY) {
  DBG_PRINT(F("BLE:"));
  DBG_PRINT(Bluefruit.connected() ? F("connected") : F("advertising"));

  DBG_PRINT(F("  buttons:0b"));
  for (int i = 8; i >= 0; i--) DBG_PRINT((buttons >> i) & 1);

  DBG_PRINT(F("  joyX:"));
  DBG_PRINT(rawX);
  DBG_PRINT(F("  joyY:"));
  DBG_PRINT(rawY);

  if (icmReady) {
    DBG_PRINT(F("  accel(ax,ay,az):"));
    DBG_PRINT(motionPacket.ax); DBG_PRINT(F(","));
    DBG_PRINT(motionPacket.ay); DBG_PRINT(F(","));
    DBG_PRINT(motionPacket.az);
    DBG_PRINT(F("  gyro(gx,gy,gz):"));
    DBG_PRINT(motionPacket.gx); DBG_PRINT(F(","));
    DBG_PRINT(motionPacket.gy); DBG_PRINT(F(","));
    DBG_PRINT(motionPacket.gz);
  } else {
    DBG_PRINT(F("  IMU:not detected"));
  }
  DBG_PRINTLN();
}
#endif

void loop() {
  // --- Buttons + joystick read every loop, regardless of BLE state, so ---
  // --- Serial debug output works even before/without a BLE connection. ---
  uint32_t buttons = readButtons();
  uint16_t rawX = analogReadRaw(AIN_JOY_X);
  uint16_t rawY = analogReadRaw(AIN_JOY_Y);

  uint32_t now = millis();

  // --- Motion read: own cadence, independent of connection state ---
  if (now - lastMotionMs >= MOTION_INTERVAL_MS) {
    lastMotionMs = now;
    if (icmReady) {
      icmReadMotion(motionPacket.ax, motionPacket.ay, motionPacket.az,
                    motionPacket.gx, motionPacket.gy, motionPacket.gz);
    }
  }

#if DEBUG_SERIAL
  if (now - lastDebugMs >= DEBUG_INTERVAL_MS) {
    lastDebugMs = now;
    printDebugLine(buttons, rawX, rawY);
  }
#endif

  if (!Bluefruit.connected()) {
    delay(20);
    return;
  }

  // --- Gamepad report: buttons + stick, sent every loop while connected ---
  // hid_gamepad_report_t expects signed -127..127, center 0, per TinyUSB's
  // spec -- convert from the 10-bit 0..1023 ADC reading accordingly.
  gp.x = (int8_t)(((int32_t)rawX - 512) * 127 / 512);
  gp.y = (int8_t)(((int32_t)rawY - 512) * 127 / 512);
  gp.z = 0;
  gp.rz = 0;
  gp.rx = 0;
  gp.ry = 0;
  gp.hat = 0;
  gp.buttons = buttons;
  blegamepad.report(&gp);

  // --- Motion notify: send whatever we most recently read above ---
  if (motionChar.notifyEnabled()) {
    motionChar.notify(&motionPacket, sizeof(motionPacket));
  }

  delay(4); // ~125Hz+ loop rate for button/stick responsiveness
}

#include <Wire.h>
#include "Adafruit_TinyUSB.h"

/**
 * @file encusb.ino
 * @brief Outputs AS5600 angle values through USB serial communication
 */

/**
 * @brief LED pin definitions
 */
constexpr uint8_t LED_ERROR = PA15;
constexpr uint8_t LED_POWER = PB1;

/**
 * @brief AS5600 I2C address
 */
constexpr uint8_t AS5600_ADDR = 0x36;

/**
 * @brief AS5600 register addresses
 */
constexpr uint8_t REG_STATUS = 0x0B;
constexpr uint8_t REG_RAW_ANGLE = 0x0C;

/**
 * @brief AS5600 status bits
 */
constexpr uint8_t STATUS_MD = 0x20;  ///< Magnet detected
constexpr uint8_t STATUS_ML = 0x10;  ///< Magnet strength too low
constexpr uint8_t STATUS_MH = 0x08;  ///< Magnet strength too high

/**
 * @brief Reads data from an AS5600 register
 *
 * @param reg Register address to read
 * @param data Buffer to store received data
 * @param len Number of bytes to read
 *
 * @return true Read successful
 * @return false Communication failed
 */
bool readRegister(uint8_t reg, uint8_t* data, uint8_t len)
{
  Wire.beginTransmission(AS5600_ADDR);

  Wire.write(reg);

  if (Wire.endTransmission(false) != 0)
    return false;

  if (Wire.requestFrom(AS5600_ADDR, len) != len)
    return false;

  for (uint8_t i = 0; i < len; i++)
  {
    data[i] = Wire.read();
  }

  return true;
}


/**
 * @brief Reads the raw angle value from AS5600
 *
 * @return 12-bit angle value (0-4095)
 */
uint16_t readAngleRaw()
{
  uint8_t data[2];

  if (!readRegister(REG_RAW_ANGLE, data, 2))
    return 0;

  return (((uint16_t)data[0] << 8) | data[1]) & 0x0FFF;
}


/**
 * @brief Reads the AS5600 status register
 *
 * @return Status bit field
 */
uint8_t readStatus()
{
  uint8_t status = 0;

  readRegister(
    REG_STATUS,
    &status,
    1
  );

  return status;
}


/**
 * @brief Controls the error LED
 *
 * @param error true to turn on the LED
 */
void setErrorLED(bool error)
{
  digitalWrite(
    LED_ERROR,
    error ? HIGH : LOW
  );
}


/**
 * @brief Initialization routine
 */
void setup()
{
  if (!TinyUSBDevice.isInitialized())
  {
    TinyUSBDevice.begin(0);
  }

  SerialTinyUSB.begin(115200);
  Wire.begin();

  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);

  digitalWrite(LED_POWER, HIGH);
  digitalWrite(LED_ERROR, LOW);
}


/**
 * @brief Main loop
 */
void loop()
{
#ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
#endif

  uint8_t status = readStatus();
  bool error = true;
  int16_t angle = -1;

  if (status & STATUS_MD)
  {
    angle = readAngleRaw();
    if (!(status & (STATUS_ML | STATUS_MH)))
    {
      error = false;
    }
  }

  setErrorLED(error);
  SerialTinyUSB.println(angle);

  delay(1);
}

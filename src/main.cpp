// PD-Camera project by Tom Granger 
// Highly experimental, use at your own risk :)

#include <Arduino.h>
#include <EEPROM.h>
#include <Dither.h>
#include <OV7670.h>
#include <SD.h>
#include "USBHost_t36.h"

// Adds a ton of debug info to the Teensy's serial 
// including a mirror of the Playdate's serial output.
// #define DEBUG 1

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif

const int ledpin = 13;
const char *PLAYDATE_PRODUCT_NAME = "Playdate";
const unsigned int MAX_INPUT = 80;
elapsedMillis sinceLastMessage;
uint32_t baud = 115200;
USBHost myusb;
USBHub hub1(myusb);
USBSerial_BigBuffer userial(myusb, 1);

USBDriver *drivers[] = {&hub1, &userial};
#define CNT_DEVICES (sizeof(drivers) / sizeof(drivers[0]))
const char *driver_names[CNT_DEVICES] = {"Hub1", "USERIAL1"};
bool driver_active[CNT_DEVICES] = {false, false};

//  Put all the buffers in DMAMEM
uint8_t fcaptbuff[320l * 240l * 2] DMAMEM;
uint8_t cbuff1[320l * 240l * 2] DMAMEM;
uint8_t cbuff2[320l * 240l * 2] DMAMEM;
Dither image(320, 240);

const char compileTime[] = " Compiled on " __DATE__ " " __TIME__;

#define LEDON digitalWriteFast(ledpin, HIGH); // Also marks IRQ handler timing
#define LEDOFF digitalWriteFast(ledpin, LOW);

void blinkN(int times)
{
  for (int i = 0; i < times; i++)
  {
    delay(200);
    LEDON;
    delay(200);
    LEDOFF;
  }
}

void blinkForever(void)
{
  while (1)
  {
    blinkN(1);
  }
}

const int pinCamReset = 14;

// Dither library overrides
#define fastEDDither_remove_artifacts true
#define _size 4

enum CameraStatus {
  INITIALIZING,
  AWAITING_CONNECTION,
  CONNECTED,
  FRAME_REQUESTED,
  SENDING_FRAME,
};

CameraStatus status = INITIALIZING;

enum DitherType
{
  STUCKI,
  ATKINSON,
  FS,
  FAST,
  BAYER,
  RNDM,
  THRESHOLD
};

DitherType currDitherType = STUCKI;
uint8_t thresh = 128;

/*
  These are the 2 Lua functions sent by the Teensy via eval command.
  See https://github.com/t0mg/pd-camera-hardware/lua/readme.md for details.
*/
static const int statusCharIndex = 87;
static const int evalStatusCodeLength = 107;
static const byte evalStatusCode[evalStatusCodeLength] PROGMEM = {
    27, 76, 117, 97, 84, 0, 25, 147, 13, 10, 26, 10, 4, 4, 4, 120,
    86, 0, 0, 0, 64, 185, 67, 1, 147, 64, 112, 114, 111, 99, 101, 115,
    115, 83, 116, 97, 116, 117, 115, 46, 108, 117, 97, 128, 128, 0, 1, 2,
    133, 79, 0, 0, 0, 9, 0, 0, 0, 131, 128, 0, 0, 66, 0, 2,
    1, 68, 0, 1, 1, 130, 4, 142, 95, 99, 97, 109, 101, 114, 97, 83,
    116, 97, 116, 117, 115, 4, 130, 49, 129, 1, 0, 0, 128, 133, 1, 0,
    0, 0, 0, 128, 128, 129, 133, 95, 69, 78, 86};

enum StatusCode
{
  INIT = 48,         // 0 in ASCII
  READY = 49,        // 1
  STREAMING = 50,    // 2
  OK = 51,           // 3
  ERROR = 52         // 4
};

void sendStatusCode(StatusCode code)
{
  byte statusMessage[evalStatusCodeLength];
  memcpy(statusMessage, evalStatusCode, evalStatusCodeLength);
  statusMessage[statusCharIndex] = code;
  userial.flush();
  userial.println("eval 107");
  userial.write(statusMessage, evalStatusCodeLength);
  sinceLastMessage = 0;
}

static const int headerLength = 86;
static const byte evalPayloadHeader[headerLength] PROGMEM = {
    27, 76, 117, 97, 84, 0, 25, 147, 13, 10, 26, 10, 4, 4, 4, 120,
    86, 0, 0, 0, 64, 185, 67, 1, 146, 64, 112, 114, 111, 99, 101, 115,
    115, 73, 109, 97, 103, 101, 46, 108, 117, 97, 128, 128, 0, 1, 2, 133,
    79, 0, 0, 0, 9, 0, 0, 0, 131, 128, 0, 0, 66, 0, 2, 1,
    68, 0, 1, 1, 130, 4, 141, 95, 99, 97, 109, 101, 114, 97, 73, 109,
    97, 103, 101, 20, 75, 129}; // for 320x240 (QVGA) image
// 93, 225}; for 400x240 image

static const int footerLength = 19;
static const byte evalPayloadFooter[footerLength] PROGMEM = {
    129, 1, 0, 0, 128, 133, 1, 0, 0, 0,
    0, 128, 128, 129, 133, 95, 69, 78, 86};

static const int imageWidth = 320;
static const int imageHeight = 240;
static const int packedImageLength = imageWidth * imageHeight / 8;
static const int payloadLength = headerLength + packedImageLength + footerLength;
char evalCommand[12];

void processImage(void)
{
  if (status != FRAME_REQUESTED) {
    return;
  }
  status = SENDING_FRAME;
  uint32_t imagesize = OV7670.ImageSize();

  OV7670.ClearFrameReady();
  // wait until cbuff2 is ready
  while (OV7670.FrameReady() != 2)
  {
  }
  // pack Y values at the first half of the buffer
  for (uint32_t i = 0u; i < imagesize / 2; i++)
  {
    fcaptbuff[i] = cbuff2[2 * i];
  }
  // apply dithering to the first half of the buffer
  switch (currDitherType)
  {
  case STUCKI:
    image.StuckiDither(fcaptbuff);
    break;
  case ATKINSON:
    image.AtkinsonDither(fcaptbuff);
    break;
  case FS:
    image.FSDither(fcaptbuff);
    break;
  case FAST:
    image.fastEDDither(fcaptbuff);
    break;
  case BAYER:
    image.patternDither(fcaptbuff);
    break;
  case RNDM:
    image.randomDither(fcaptbuff, false);
    break;
  default:
    image.thresholding(fcaptbuff, thresh);
    break;
  }
  // pack the image as 1bit in a new byte array
  byte payload[payloadLength];
  memcpy(payload, evalPayloadHeader, headerLength);
  memcpy(&payload[headerLength + packedImageLength], evalPayloadFooter, footerLength);
  int byteIndex = headerLength;
  int pixelIndex = 0;
  int bufferIndex = 0;
  while (byteIndex < headerLength + packedImageLength)
  {
    byte currentByte = 0;
    for (int n = 7; n >= 0; n--)
    {
      uint8_t val = fcaptbuff[bufferIndex++];
      if (val == 0u)
      {
        // clear bit at byteindex
        currentByte &= ~(1 << n);
      }
      else
      {
        // set bit at byteindex
        currentByte |= (1 << n);
      }
      pixelIndex++;
      pixelIndex = pixelIndex % imageWidth;
    }
    payload[byteIndex++] = currentByte;
  }
  if (status != SENDING_FRAME)
  {
    return;
  }
  userial.flush();
  userial.println(evalCommand);
  userial.write(payload, payloadLength);
  sinceLastMessage = 0;
  status = CONNECTED;
}

void processUpdatedDeviceListInfo()
{
  for (uint8_t i = 0; i < CNT_DEVICES; i++)
  {
    if (*drivers[i] != driver_active[i])
    {
      if (driver_active[i])
      {
#ifdef DEBUG
        Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
#endif
        driver_active[i] = false;
        if (strcmp(driver_names[i], "USERIAL1") == 0)
        {
          status = INITIALIZING;
          blinkN(3);
          // sendStatusCode(INIT);
          // userial.end();
        }
      }
      else
      {
        driver_active[i] = true;
        const uint8_t *product = drivers[i]->product();
#ifdef DEBUG
        Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
        const uint8_t *psz = drivers[i]->manufacturer();
        if (psz && *psz)
          Serial.printf("  manufacturer: %s\n", psz);
        if (product && *product)
          Serial.printf("  product: %s\n", product);
        psz = drivers[i]->serialNumber();
        if (psz && *psz)
          Serial.printf("  Serial: %s\n", psz);
#endif
        // If this is a new Serial device.
        if (drivers[i] == &userial) //&& strcmp((const char *)product, PLAYDATE_PRODUCT_NAME) == 0)
        {
          DEBUG_PRINT("\nConnected to Playdate Serial, yay!");
          blinkN(1);
          status = AWAITING_CONNECTION;
          userial.begin(baud);
        }
      }
    }
  }
}

bool prefix(const char *pre, const char *str)
{
  return strncmp(pre, str, strlen(pre)) == 0;
}

const char *dataWithPrefix(const char *prefix, const char *string)
{
  int prefix_length = strlen(prefix);
  int string_length = strlen(string);

  if (prefix_length > string_length)
  {
    return NULL;
  }

  for (int i = 0; i < prefix_length; i++)
  {
    if (string[i] != prefix[i])
    {
      return NULL;
    }
  }

  return string + prefix_length;
}

void process_data(const char *data)
{
#ifdef DEBUG
  Serial.print("[Playdate] ");
  Serial.println(data);
#endif
  // Early exit if this is not a Camera command message.
  if (!prefix("Camera:", data))
  {
    return;
  }

  sinceLastMessage = 0;
  // Basic Camera commands.
  if (strcmp(data, "Camera:connect") == 0)
  {
    // if (status == AWAITING_CONNECTION)
    // {
      sendStatusCode(READY);
      status = CONNECTED;
    // }
    return;
  }
  else if (strcmp(data, "Camera:readyForNextFrame") == 0)
  {
    sendStatusCode(STREAMING);
    status = FRAME_REQUESTED;
    return;
  }
  else if (strcmp(data, "Camera:disconnect") == 0)
  {
    status = AWAITING_CONNECTION;
    return;
  }

  // Dithering mode setting.
  const char *ditherMode = dataWithPrefix("Camera:dither:", data);
  if (ditherMode != NULL)
  {
    if (strcmp(ditherMode, "stucki") == 0)
    {
      DEBUG_PRINT("Switching to Stucki dithering");
      currDitherType = STUCKI;
      // sendStatusCode(OK);
    }
    else if (strcmp(ditherMode, "atkinson") == 0)
    {
      DEBUG_PRINT("Switching to Atkinson dithering");
      currDitherType = ATKINSON;
      // sendStatusCode(OK);
    }
    else if (strcmp(ditherMode, "fs") == 0)
    {
      DEBUG_PRINT("Switching to Floyd-Steinberg dithering");
      currDitherType = FS;
      // sendStatusCode(OK);
    }
    else if (strcmp(ditherMode, "fast") == 0)
    {
      DEBUG_PRINT("Switching to Fast Error Diffusion dithering");
      currDitherType = FAST;
      // sendStatusCode(OK);
    }
    else if (strcmp(ditherMode, "random") == 0)
    {
      DEBUG_PRINT("Switching to random dithering");
      currDitherType = RNDM;
      // sendStatusCode(OK);
    }
    else if (strcmp(ditherMode, "bayer") == 0)
    {
      DEBUG_PRINT("Switching to bayer patterned dithering");
      currDitherType = BAYER;
      // sendStatusCode(OK);
    }
    else if (strcmp(ditherMode, "threshold") == 0)
    {
      DEBUG_PRINT("Switching to thresholding");
      currDitherType = THRESHOLD;
      // sendStatusCode(OK);
    }
    else
    {
      DEBUG_PRINT("Unknown dither mode requested");
      sendStatusCode(ERROR);
    }
    return;
  }

  // Contrast setting.
  const char *constrastStr = dataWithPrefix("Camera:contrast:", data);
  if (constrastStr != NULL)
  {
    int contrastValue = atoi(constrastStr);
    if (contrastValue > 0 && contrastValue < 256)
    {
      OV7670.SetContrast(contrastValue);
      // sendStatusCode(OK);
      return;
    }
    sendStatusCode(ERROR);
    return;
  }

  // Brightness setting.
  const char *brightnessStr = dataWithPrefix("Camera:brightness:", data);
  if (brightnessStr != NULL)
  {
    int brightnessValue = atoi(brightnessStr);
    if (brightnessValue > 0 && brightnessValue < 256)
    {
      OV7670.SetBrightness(brightnessValue);
      // sendStatusCode(OK);
      return;
    }
    sendStatusCode(ERROR);
    return;
  }

  // Threshold setting.
  const char *thresholdStr = dataWithPrefix("Camera:threshold:", data);
  if (thresholdStr != NULL)
  {
    int thresholdValue = atoi(thresholdStr);
    if (thresholdValue > 0 && thresholdValue < 256)
    {
      thresh = thresholdValue;
      // sendStatusCode(OK);
      return;
    }
    sendStatusCode(ERROR);
    return;
  }

  // Mirror (selfie mode) setting.
  const char *mirrorStr = dataWithPrefix("Camera:mirror:", data);
  if (mirrorStr != NULL)
  {
    int mirrorValue = atoi(mirrorStr);
    if (mirrorValue == 1 || mirrorValue == 0)
    {
      OV7670.WriteRegister(REG_MVFP, MVFP_FLIP + MVFP_MIRROR * abs(mirrorValue - 1));
      // sendStatusCode(OK);
      return;
    }
    sendStatusCode(ERROR);
    return;
  }
}

// from http://www.gammon.com.au/serial
void processIncomingByte(const byte inByte)
{
  static char inputLine[MAX_INPUT];
  static unsigned int inputPos = 0;

  switch (inByte)
  {
  case '\n':                   // end of text
    inputLine[inputPos] = 0; // terminating null byte
    // terminator reached! process inputLine here ...
    process_data(inputLine);
    // reset buffer for next time
    inputPos = 0;
    break;
  case '\r': // discard carriage return
    break;
  default:
    // keep adding if not full ... allow for terminating null byte
    if (inputPos < (MAX_INPUT - 1))
      inputLine[inputPos++] = inByte;
    break;
  } // end of switch

} // end of processIncomingByte

void setup()
{
  sprintf(evalCommand, "eval %d", payloadLength);
  image.buildBayerPattern();

  myusb.begin();

  Serial.begin(115200);
  delay(200);
  Wire.begin();

  pinMode(pinCamReset, OUTPUT);
  pinMode(ledpin, OUTPUT);

  digitalWriteFast(pinCamReset, LOW);
  delay(10);
  digitalWriteFast(pinCamReset, HIGH); // subsequent resets via SCB

#ifdef DEBUG
  // while (!Serial)
  // ; // wait for Arduino Serial Monitor
  Serial.printf("\n\nPlaydate Camera adapter %s\n", compileTime);
#endif

  if (OV7670.begin(QVGA, cbuff1, cbuff2))
  {
    OV7670.SetOutMode(YUV422);
    OV7670.WriteRegister(REG_TSLB, 1 << 4);
    OV7670.WriteRegister(0x67, 0);
    OV7670.WriteRegister(0x68, 0);
    OV7670.WriteRegister(REG_MVFP, MVFP_FLIP + MVFP_MIRROR);
    OV7670.ShowCamConfig();
    OV7670.ShowCSIRegisters();
#ifdef DEBUG
    Serial.println("OV7670 camera initialized.");
    Serial.printf("cbuff1 at   %p\n", cbuff1);
    Serial.printf("cbuff2 at    %p\n", cbuff2);
    Serial.printf("fcaptbuff at %p\n", fcaptbuff);
#endif
  }
  else
  {
#ifdef DEBUG
    Serial.println("Error initializing OV7670");
#endif
    blinkForever();
  }

#ifdef DEBUG
  OV7670.ShowCamConfig();
#endif

  LEDOFF
}

void loop()
{
  myusb.Task();
  processUpdatedDeviceListInfo();

  // Send requested image to the Camera app.
  if (status == FRAME_REQUESTED)
  {
    processImage();
  }
  // Let the Camera app know we're ready to send bytes.
  else if (sinceLastMessage >= 1000 && status != CONNECTED) {
    sinceLastMessage = sinceLastMessage - 1000;
    // sendStatusCode(READY);
  }

  // Monitor Playdate serial and process incoming messages.
  if (status != INITIALIZING) {
    while (userial.available() > 0)
      processIncomingByte(userial.read());
  }

}
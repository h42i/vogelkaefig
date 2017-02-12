#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Espanol.h>
#include <APA102.h>
#include <Espanol.h>
#include <stdlib.h>

#define MODE_RAINBOW   0
#define MODE_SET_COLOR 1
#define MODE_OFF       2

#define MODE_TOPIC "hasi/lights/birdcage/mode"
#define DATA_TOPIC "hasi/lights/birdcage"

#define OTA_PASSWORD "bugsbunny"

char* ssid     = "HaSi-Kein-Internet-Legacy";
char* password = "bugsbunny";
char* broker   = "atlas.hasi";
int port       = 1883;

int mode = MODE_RAINBOW;

const uint8_t dataPin = 14;
const uint8_t clockPin = 12;

APA102<dataPin, clockPin> ledStrip;

const uint16_t ledCount = 56;

rgb_color colors[ledCount];
uint8_t brightness = 31;

char *stringFromBytes(byte *payload, unsigned int length)
{
    char *str = new char[length + 1];
    memcpy(str, payload, length);
    str[length] = 0;

    return str;
}

void callback(char *topic, byte *payload, unsigned int length)
{
    if (strcmp(topic, MODE_TOPIC) == 0)
    {
        char *modeString = stringFromBytes(payload, length);

        if (strcmp(modeString, "rainbow") == 0)
        {
            mode = MODE_RAINBOW;
        }
        else if (strcmp(modeString, "set_color") == 0)
        {
            mode = MODE_SET_COLOR;
        }
        else if (strcmp(modeString, "off") == 0)
        {
            mode = MODE_OFF;

            for (uint16_t i = 0; i < ledCount; i++)
            {
                colors[i] = (rgb_color){0, 0, 9};
            }

            ledStrip.write(colors, ledCount, 0);
        }

        delete[] modeString;
    }
    else if (strcmp(topic, DATA_TOPIC) == 0)
    {
        if (mode == MODE_SET_COLOR)
        {
            char *hexString = stringFromBytes(payload, length);

            uint32_t number = strtoul(&hexString[1], NULL, 16);

            uint8_t r = (number >> 16) & 0xFF;
            uint8_t g = (number >> 8) & 0xFF;
            uint8_t b = number & 0xFF;

            for (uint16_t i = 0; i < ledCount; i++)
            {
                colors[i] = (rgb_color){r, g, b};
            }

            ledStrip.write(colors, ledCount, brightness);
        }
    }

    String msg = topic;
    msg += " - ";
    msg += (char*) payload;

    Serial.println(msg);
}

rgb_color hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;

    switch((h / 60) % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }

    return (rgb_color){r, g, b};
}

static uint16_t rainbowLast = millis();
void mode_rainbow()
{
    uint16_t diff = rainbowLast - millis();

    uint8_t time = millis() >> 4;

    if (diff > 10)
    {
        for (uint16_t i = 0; i < ledCount; i++)
        {
            byte x = time - i * 8;
            colors[i] = hsvToRgb((uint32_t) x * 359 / 256, 255, 255);
        }

        ledStrip.write(colors, ledCount, brightness);

        rainbowLast = millis();
    }
}

void setup()
{
    Serial.begin(115200);

    Espanol.setCallback(callback);
    Espanol.begin(ssid, password, "birdcage", broker, port);

    Espanol.setDebug(true);
    Espanol.subscribe(MODE_TOPIC);
    Espanol.subscribe(DATA_TOPIC);

    ArduinoOTA.setHostname("birdcage");
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
}

void loop()
{
    switch (mode)
    {
        case MODE_RAINBOW:
            mode_rainbow();
            break;

        case MODE_SET_COLOR:
            break;

        case MODE_OFF:
            break;
    }

    ArduinoOTA.handle();
    Espanol.loop();
}

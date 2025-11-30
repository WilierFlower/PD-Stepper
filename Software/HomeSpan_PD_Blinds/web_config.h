#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "motion.h"
#include "tmc_init.h"
#include "as5600.h"

extern AsyncWebServer server;

void WebConfig_begin();
String WebConfig_getHTML();


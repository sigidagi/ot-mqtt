#ifndef OT_RTOS_CLIENT_CFG_HPP_
#define OT_RTOS_CLIENT_CFG_HPP_

//static const char   CLOUDIOT_SERVER_ADDRESS[]   = "ffa48553d3bd43ccaec31f521ecb192e.s2.eu.hivemq.cloud";
static const char   CLOUDIOT_SERVER_ADDRESS[]   = "test.mosquitto.org";
static const char   CLOUDIOT_SERVER_USER[]      = "";
static const char   CLOUDIOT_SERVER_PASS[]      = "";
//static unsigned int CLOUDIOT_SERVER_PORT        = 8883;
static unsigned int CLOUDIOT_SERVER_PORT        = 1883;

static const char   CLOUDIOT_DEVICE_ID[]        = "bedroom";  // subtopic for claire/device_id/temperature
static const char   CLOUDIOT_CLIENT_ID[]        = "claire-client";

#endif

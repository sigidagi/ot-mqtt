/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <FreeRTOS.h>
#include <task.h>

#include "mqtt_client.hpp"

#include <openthread/ip6.h>
#include <openthread/joiner.h>
#include <openthread/openthread-freertos.h>
#include <openthread/thread.h>

#include <lwip/altcp_tcp.h>
#include <lwip/apps/http_client.h>
#include <lwip/netdb.h>
#include <lwip/tcpip.h>

#include "net/utils/nat64_utils.h"

#define MSG_MAX_LENGTH 100

static TaskHandle_t sDemoTask;

static void setupNat64(void)
{
    ip6_addr_t nat64Prefix;

    nat64Prefix.zone = 0;
    inet_pton(AF_INET6, "64:ff9b::", nat64Prefix.addr);
    setNat64Prefix(&nat64Prefix);
}

void setNetworkConfiguration(otInstance *aInstance)
{
    static char          aNetworkName[] = "OpenThread-Sigis";
    otOperationalDataset aDataset;

    memset(&aDataset, 0, sizeof(otOperationalDataset));

    /*
     * Fields that can be configured in otOperationDataset to override defaults:
     *     Network Name, Mesh Local Prefix, Extended PAN ID, PAN ID, Delay Timer,
     *     Channel, Channel Mask Page 0, Network Key, PSKc, Security Policy
     */
    
    //aDataset.mActiveTimestamp.mSeconds             = 1;
    //aDataset.mActiveTimestamp.mTicks               = 0;
    //aDataset.mActiveTimestamp.mAuthoritative       = false;
    aDataset.mComponents.mIsActiveTimestampPresent = false;

    /* Set Channel */
    aDataset.mChannel                      = 12;
    aDataset.mComponents.mIsChannelPresent = true;

    /* Set Pan ID */
    aDataset.mPanId                      = (otPanId)0xf913;
    aDataset.mComponents.mIsPanIdPresent = true;

    /* Set Extended Pan ID */
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {0x73, 0xa7, 0x52, 0x23, 0x03, 0xca, 0x17, 0x66};
    memcpy(aDataset.mExtendedPanId.m8, extPanId, sizeof(aDataset.mExtendedPanId));
    aDataset.mComponents.mIsExtendedPanIdPresent = false;

    /* Set network key */
    uint8_t key[OT_MASTER_KEY_SIZE] = {0x5c, 0xd9, 0x87, 0x02, 0xb2, 0x88, 0x5d, 0xb5, 0xe3, 0xdc, 0xba, 0x39, 0x58, 0xef, 0xd9, 0x2e}; //rpi2
    //const char networkkey[] = "5cd98702b2885db5e3dcba3958efd92e", *pos = networkkey;
    //unsigned char key[OT_MASTER_KEY_SIZE];

    memcpy(aDataset.mMasterKey.m8, key, sizeof(aDataset.mMasterKey));
    aDataset.mComponents.mIsMasterKeyPresent = true;

    /* Set Network Name */
    size_t length = strlen(aNetworkName);
    assert(length <= OT_NETWORK_NAME_MAX_SIZE);
    memcpy(aDataset.mNetworkName.m8, aNetworkName, length);
    aDataset.mComponents.mIsNetworkNamePresent = true;
    
    printf("Set dataset with network key\n");
    OT_API_CALL(otDatasetSetActive(aInstance, &aDataset));
    /* Set the router selection jitter to override the 2 minute default.
       CLI cmd > routerselectionjitter 20
       Warning: For demo purposes only - not to be used in a real product */
    //uint8_t jitterValue = 20;
    //otThreadSetRouterSelectionJitter(aInstance, jitterValue);
}

void handleNetifStateChanged(uint32_t aFlags, void *instance)
{
   if ((aFlags & OT_CHANGED_THREAD_ROLE) != 0)
   {
       otDeviceRole changedRole = otThreadGetDeviceRole((otInstance*)instance);
       switch (changedRole)
       {
       case OT_DEVICE_ROLE_LEADER:
           printf("OT_DEVICE_ROLE_LEADER\r\n");
           break;

       case OT_DEVICE_ROLE_ROUTER:
           printf("OT_DEVICE_ROLE_ROUTER\r\n");
           break;

       case OT_DEVICE_ROLE_CHILD:
           printf("OT_DEVICE_ROLE_CHILD\r\n");
           break;

       case OT_DEVICE_ROLE_DETACHED:
       case OT_DEVICE_ROLE_DISABLED:
           printf("OT_DEVICE_ROLE_DETACHED or OT_DEVICE_ROLE_DISABLED\r\n");
           break;
        }
    }
}



static void configCallback(const char *aTopic, const char *aMsg, uint16_t aMsgLength)
{
    printf("Topic %s get message len = %d %s\r\n", aTopic, aMsgLength, aMsg);
}


void demo101Task(void *p)
{
    /* Override default network credentials */
    setNetworkConfiguration(otrGetInstance());

    /* Register Thread state change handler */
    otSetStateChangedCallback(otrGetInstance(), handleNetifStateChanged, otrGetInstance());

    /* Start the Thread network interface (CLI cmd > ifconfig up) */
    OT_API_CALL(otIp6SetEnabled(otrGetInstance(), true));

    // thread start
    printf("Enable thread\n");
    OT_API_CALL(otThreadSetEnabled(otrGetInstance(), true));
    setupNat64();

    // wait a while for thread to connect
    vTaskDelay(pdMS_TO_TICKS(2000));

    // dns64 www.google.com
    printf("Connect mqtt client\n");
    //dnsNat64Address("www.google.com", &serverAddr.u_addr.ip6);

    claire::IotClientCfg *cfg = static_cast<claire::IotClientCfg *>(p);
    char                     subTopic[claire::IotMqttClient::kTopicDataMaxLength];
    int                      temperature = 0;

    claire::IotMqttClient client(*cfg);
    client.Connect();

    printf("Connect done\r\n");

    snprintf(subTopic, sizeof(subTopic), "/devices/%s/config", cfg->mDeviceId);
    printf("Subscribe to %s\n", subTopic);
    client.Subscribe(subTopic, configCallback);


    // periodically curl www.google.com
    while (true)
    {
        char pubTopic[claire::IotMqttClient::kTopicDataMaxLength];
        char msg[MSG_MAX_LENGTH];

        temperature++;
        temperature %= 20;
        snprintf(pubTopic, sizeof(pubTopic), "/devices/%s/events", cfg->mDeviceId);
        snprintf(msg, sizeof(msg), "{\"temperature\": %d}", temperature - 5);
        client.Publish(pubTopic, msg, strlen(msg));
        printf("Publish message: %s\r\n", msg);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    vTaskDelete(NULL);
}


void otrUserInit(void)
{
    xTaskCreate(demo101Task, "demo", 3000, &sDemoTask, 2, &sDemoTask);
}

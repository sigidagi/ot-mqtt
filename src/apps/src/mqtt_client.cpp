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

#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/netdb.h"
#include "lwip/tcpip.h"

#include "jwt.h"

#include <stdlib.h>
#include <string.h>

#include "common/code_utils.hpp"
#include "mqtt_client.hpp"
#include "net/utils/nat64_utils.h"
#include "net/utils/time_ntp.h"

#include <openthread/openthread-freertos.h>

#include <mbedtls/debug.h>

#define MQTT_CLIENT_NOTIFY_VALUE (1 << 9)
#define MQTT_PUBSUB_NOTIFY_VALUE (1 << 10)

using namespace claire;

namespace claire {

struct ConnectContext
{
    IotMqttClient *mClient;
    TaskHandle_t              mHandle;
};

static const int           kQos     = 1;
static const unsigned long kTimeout = 10000L;

static const unsigned long kInitialConnectIntervalMillis     = 500L;
static const unsigned long kMaxConnectIntervalMillis         = 6000L;
static const unsigned long kMaxConnectRetryTimeElapsedMillis = 900000L;
static const float         kIntervalMultiplier               = 1.5f;

static void GetIatExp(char *aIat, char *aExt, int time_size)
{
    time_t now_seconds = timeNtp();

#ifdef PLATFORM_linux
    snprintf(aIat, (size_t)time_size, "%zu", static_cast<size_t>(now_seconds));
    snprintf(aExt, (size_t)time_size, "%zu", static_cast<size_t>(now_seconds + 3600));
#else // arm gcc has some problems handling %zu
    snprintf(aIat, (size_t)time_size, "%lu", static_cast<size_t>(now_seconds));
    snprintf(aExt, (size_t)time_size, "%lu", static_cast<size_t>(now_seconds + 3600));
#endif
}

void IotMqttClient::MqttPubSubChanged(void *aArg, err_t aResult)
{
    ConnectContext *ctx         = static_cast<ConnectContext *>(aArg);
    ctx->mClient->mPubSubResult = aResult;
    xTaskNotify(ctx->mHandle, MQTT_PUBSUB_NOTIFY_VALUE, eSetBits);
}

void IotMqttClient::MqttConnectChanged(mqtt_client_t *aClient, void *aArg, mqtt_connection_status_t aResult)
{
    (void)aClient;

    ConnectContext *ctx = static_cast<ConnectContext *>(aArg);

    if (aResult == MQTT_CONNECT_ACCEPTED)
    {
        printf("Mqtt Connected\r\n");
    }

    if (aResult != MQTT_CONNECT_DISCONNECTED)
    {
        ctx->mClient->mConnectResult = aResult;
        xTaskNotify(ctx->mHandle, MQTT_CLIENT_NOTIFY_VALUE, eSetBits);
    }
}

IotMqttClient::IotMqttClient(const IotClientCfg &aConfig)
    : mConfig(aConfig)
    , mMqttClient(NULL)
    , mSubCb(NULL)
{
}

int IotMqttClient::Connect(void)
{
    int       ret         = 0;
    uint32_t  notifyValue = 0;
    ip_addr_t serverAddr;

    mMqttClient = mqtt_client_new();
    memset(&mClientInfo, 0, sizeof(mClientInfo));
    mClientInfo.keep_alive  = 60;
    mClientInfo.client_user = NULL;
    
    printf("Connecting to address %s\n", mConfig.mAddress);
    //serverAddr.u_addr.ip6.  

    if (dnsNat64Address(mConfig.mAddress, &serverAddr.u_addr.ip6) == 0)
    {
        struct ConnectContext ctx;

        ctx.mClient     = this;
        ctx.mHandle     = xTaskGetCurrentTaskHandle();
        serverAddr.type = IPADDR_TYPE_V6;

        LOCK_TCPIP_CORE();
        mqtt_client_connect(mMqttClient, &serverAddr, kMqttPort, &IotMqttClient::MqttConnectChanged, &ctx,
                            &mClientInfo);
        UNLOCK_TCPIP_CORE();

        while ((notifyValue & MQTT_CLIENT_NOTIFY_VALUE) == 0)
        {
            xTaskNotifyWait(0, MQTT_CLIENT_NOTIFY_VALUE, &notifyValue, portMAX_DELAY);
        }
        ret = (mConnectResult == 0) ? 0 : -1;
    }
    else
    {
        printf("Unable resolve dnsNAT64 address\n");
        ret = -1;
    }

    return ret;
}

int IotMqttClient::Publish(const char *aTopic, const char *aMsg, size_t aMsgLength)
{
    uint32_t              notifyValue = 0;
    struct ConnectContext ctx;

    ctx.mClient = this;
    ctx.mHandle = xTaskGetCurrentTaskHandle();
    mqtt_publish(mMqttClient, aTopic, aMsg, aMsgLength, kQos, 0, &IotMqttClient::MqttPubSubChanged, &ctx);

    while ((notifyValue & MQTT_PUBSUB_NOTIFY_VALUE) == 0)
    {
        xTaskNotifyWait(0, MQTT_PUBSUB_NOTIFY_VALUE, &notifyValue, portMAX_DELAY);
    }
    return (mPubSubResult == 0) ? 0 : -1;
}

void IotMqttClient::MqttDataCallback(void *aArg, const uint8_t *aData, uint16_t aLength, uint8_t aFlags)
{
    IotMqttClient *client = static_cast<IotMqttClient *>(aArg);
    client->mqttDataCallback(aData, aLength, aFlags);
}

void IotMqttClient::mqttDataCallback(const uint8_t *aData, uint16_t aLength, uint8_t aFlags)
{
    uint16_t capacity = sizeof(mSubDataBuf) - mDataOffset - 1;
    uint16_t copySize = capacity < aLength ? capacity : aLength;

    printf("data callback len=%d\r\n", aLength);

    memcpy(mSubDataBuf + mDataOffset, aData, copySize);
    mDataOffset += copySize;
    if (aFlags & MQTT_DATA_FLAG_LAST)
    {
        mSubDataBuf[mDataOffset] = 0;
        if (mSubCb)
        {
            mSubCb(mSubTopicNameBuf, mSubDataBuf, mDataOffset);
        }
        mDataOffset = 0;
    }
}

void IotMqttClient::MqttPublishCallback(void *aArg, const char *aTopic, uint32_t aTotalLength)
{
    IotMqttClient *client = static_cast<IotMqttClient *>(aArg);

    client->mqttPublishCallback(aTopic, aTotalLength);
}

void IotMqttClient::mqttPublishCallback(const char *aTopic, uint32_t aTotalLength)
{
    (void)aTotalLength;

    strncpy(mSubTopicNameBuf, aTopic, sizeof(mSubTopicNameBuf));
}

int IotMqttClient::Subscribe(const char *aTopic, MqttTopicDataCallback aCb)
{
    int                   ret         = 0;
    uint32_t              notifyValue = 0;
    struct ConnectContext ctx;

    // Currently we only support subscribing to one topic
    VerifyOrExit(mSubCb == NULL, ret = -1);
    mSubCb      = aCb;
    ctx.mClient = this;
    ctx.mHandle = xTaskGetCurrentTaskHandle();

    mqtt_set_inpub_callback(mMqttClient, MqttPublishCallback, MqttDataCallback, this);
    mqtt_subscribe(mMqttClient, aTopic, 1, MqttPubSubChanged, &ctx);
    mDataOffset = 0;

    while ((notifyValue & MQTT_PUBSUB_NOTIFY_VALUE) == 0)
    {
        xTaskNotifyWait(0, MQTT_PUBSUB_NOTIFY_VALUE, &notifyValue, portMAX_DELAY);
    }

    ret = (mPubSubResult == 0) ? 0 : -1;
exit:
    return ret;
}

IotMqttClient::~IotMqttClient(void)
{
    if (mMqttClient)
    {
        mqtt_client_free(mMqttClient);
    }
    if (mClientInfo.tls_config)
    {
        altcp_tls_free_config(mClientInfo.tls_config);
    }
    if (mClientInfo.client_pass)
    {
        free(const_cast<char *>(mClientInfo.client_pass));
    }
}

} // namespace claire




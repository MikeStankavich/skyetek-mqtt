#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <MQTTClient.h>

#include <time.h>
#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "SkyeTekMQTT"
//#define TOPIC       "MQTT Examples"
//#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     5000L

void getTimestamp(TCHAR * buf) {

    time_t timer;
    struct tm *tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buf, 26, "%Y:%m:%d %H:%M:%S", tm_info);
}

unsigned char isStop = 0;
MQTTClient client;
TCHAR mqttTopic[256] = "";

unsigned char SelectLoopCallback(LPSKYETEK_TAG lpTag, void *user) {

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    TCHAR ts[32] = "";

    if (!isStop && lpTag != NULL) {
        getTimestamp(ts);
        printf("skyetek-mqtt [%s]: Type: %s; Tag: %s\n", ts, SkyeTek_GetTagTypeNameFromType(lpTag->type), lpTag->friendly);

        pubmsg.payload = (void *) lpTag->friendly;
        pubmsg.payloadlen = strlen(lpTag->friendly);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, mqttTopic, &pubmsg, &token);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);

        getTimestamp(ts);
        printf("skyetek-mqtt [%s]: MQTT message with delivery token %d delivered\n", ts, token);

        SkyeTek_FreeTag(lpTag);
    }
    return (!isStop);
}

int CallSelectTags(LPSKYETEK_READER lpReader) {
    SKYETEK_STATUS st;

    memset(mqttTopic, 0, 256 * sizeof(TCHAR));
    _stprintf(mqttTopic, "SkyeT1ek/%s", lpReader->rid);
    printf("topic: %s\n", mqttTopic);

    // the SkyeTek_SelectTags function does not return until the loop is done
    printf("Entering select loop...\n");
    st = SkyeTek_SelectTags(lpReader, AUTO_DETECT, SelectLoopCallback, 0, 1, NULL);
    if (st != SKYETEK_SUCCESS) {
        printf("Select loop failed\n");
        return 0;
    }
    printf("Select loop done\n");
    return 1;
}


int main(int argc, char *argv[]) {

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    TCHAR ts[26];

    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("skyetek-mqtt [%s]: Failed to connect, return code %d\n", ts, rc);
        exit(-1);
    }

    LPSKYETEK_DEVICE *devices = NULL;
    LPSKYETEK_READER *readers = NULL;
    LPSKYETEK_TAG *tags = NULL;
    SKYETEK_STATUS status;
    unsigned short count = 0;
    int numDevices = 0;
    int numReaders = 0;
    const int delay = 400000;    //wait at least 400ms after closing the interface before re-opening (USB enumeration)
    const int iterations = 2;    //number of select tag operations to perform for each test
    int failures = 0;
    int total = 0;

    if ((numDevices = SkyeTek_DiscoverDevices(&devices)) > 0) {
        //printf("example: devices=%d", numDevices);
        if ((numReaders = SkyeTek_DiscoverReaders(devices, numDevices, &readers)) > 0) {
            //printf("example: readers=%d\n", numReaders);
            for (int i = 0; i < numReaders; i++) {
                getTimestamp(ts);
                printf("skyetek-mqtt [%s]: Reader Found: %s-%s-%s-%s-%s\n", ts, readers[i]->rid, readers[i]->friendly,
                       readers[i]->manufacturer, readers[i]->model, readers[i]->firmware);
                CallSelectTags(readers[i]);
            }
        }
        else {
            failures++;
            printf("skyetek-mqtt [%s]: failures = %d/%d\n", ts, failures, total);
            printf("ERROR: No readers found\n");
        }
    }
    else {
        failures++;
        printf("skyetek-mqtt [%s]: failures = %d/%d\n", ts, failures, total);
        printf("ERROR: No devices found\n");
    }
    SkyeTek_FreeDevices(devices, numDevices);
    SkyeTek_FreeReaders(readers, numReaders);
//    usleep(delay);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    rc = -2;
    return rc;
}
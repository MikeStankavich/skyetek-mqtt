#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <MQTTClient.h>

#include <time.h>
#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientPub"
//#define TOPIC       "MQTT Examples"
//#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     5000L

void getTimestamp(TCHAR *buf) {

    time_t timer;
    struct tm *tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buf, 26, "%Y:%m:%d %H:%M:%S", tm_info);
}

unsigned char isStop = 0;
MQTTClient client;
TCHAR mqttTopic[256] = "";

int PublishTag(LPSKYETEK_TAG lpTag, unsigned char antennaId) {

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    TCHAR payload[128] = "";

    TCHAR ts[32] = "";

    if (!isStop && lpTag != NULL) {
        getTimestamp(ts);
        printf("skyetek-mqtt [%s]: Antenna: %x; Tag: %s\n", ts, antennaId, lpTag->friendly);
        sprintf(payload,
                "{\"readAt\":\"[%s]\",\"antennaId\":\"%x\",\"tagId\":\"%s\"}",
                ts, antennaId, lpTag->friendly);

        pubmsg.payload = (void *) payload;
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

int CallGetTags(LPSKYETEK_READER lpReader, unsigned char antennaId) {
    SKYETEK_STATUS st;
    SKYETEK_TAGTYPE tagType = AUTO_DETECT;
    LPSKYETEK_TAG *lpTags = NULL;
    unsigned short count = 0;

    printf("Reading tags from antenna %x...\n", antennaId);

    st = SkyeTek_GetTags(lpReader, tagType, &lpTags, &count);
    if (st != SKYETEK_SUCCESS) {
        printf("No tags found\n");
    }
    int ti;
    for (ti = 0;
         ti < count;
         ti++) {
        printf("Found tag: %s\n", lpTags[ti]->friendly);
//        PublishTag(lpTags[ti], antennaId);
    }
    SkyeTek_FreeTags(lpReader, lpTags, count);
}

int SwitchAntenna(LPSKYETEK_READER lpReader, unsigned char *antennaId) {
    // variables
    //    unsigned int i;
    //    double f;
    LPSKYETEK_DATA lpData;

    // get system parameter
    SKYETEK_STATUS st = SkyeTek_GetSystemParameter(lpReader, SYS_MUX_CONTROL, &lpData);
    if (st != SKYETEK_SUCCESS) {
        printf("error: could not get SYS_MUX_CONTROL: %s\n", SkyeTek_GetStatusMessage(st));
        goto failure;
    }
    // check value
    if (lpData == NULL || lpData->size == 0) {
        printf("error: SYS_MUX_CONTROL is NULL or empty\n");
        goto failure;
    }
    // print value
    printf("current SYS_MUX_CONTROL is: %x\n", lpData->data[0]);

    //    SkyeTek_FreeData(lpData);
    //    lpData = SkyeTek_AllocateData(1);
    lpData->data[0] ^= 0x01;
    *antennaId = lpData->data[0];
    printf("new SYS_MUX_CONTROL is: %x\n", lpData->data[0]);

    // set system parameter
    st = SkyeTek_SetSystemParameter(lpReader, SYS_MUX_CONTROL, lpData);
    if (st != SKYETEK_SUCCESS) {
        printf("error: could not set SYS_MUX_CONTROL: %s\n", SkyeTek_GetStatusMessage(st));
        goto failure;
    }
    printf("successfully set SYS_MUX_CONTROL\n");
    SkyeTek_FreeData(lpData);  // do nothing if NULL
    printf("done\n");
    return 1;

    failure:
    SkyeTek_FreeData(lpData);  // do nothing if NULL
    return 0;

}

void stdump(TCHAR *msg) {
    printf("SkyeTek Debug: %s", msg);
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

    // SkyeTek_SetDebugger((SKYETEK_DEBUG_CALLBACK) stdump );

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
    unsigned char antennaId = 0;

    if ((numDevices = SkyeTek_DiscoverDevices(&devices)) > 0) {
        //printf("example: devices=%d", numDevices);
        if ((numReaders = SkyeTek_DiscoverReaders(devices, numDevices, &readers)) > 0) {
            //printf("example: readers=%d\n", numReaders);
            for (int i = 0; i < numReaders; i++) {
                getTimestamp(ts);
                printf("skyetek-mqtt [%s]: Reader Found: %s-%s-%s-%s-%s\n", ts, readers[i]->rid, readers[i]->friendly,
                       readers[i]->manufacturer, readers[i]->model, readers[i]->firmware);

                LPSKYETEK_DEVICE *devices = NULL;
                LPSKYETEK_READER *readers = NULL;

                SkyeTek_DiscoverDevices(&devices);
                printf("skyetek-mqtt: devices=%d", numDevices);
                SkyeTek_DiscoverReaders(devices, numDevices, &readers);
                printf("skyetek-mqtt [%s]: Reader Found: %s-%s-%s-%s-%s\n", ts, readers[i]->rid, readers[i]->friendly,
                       readers[i]->manufacturer, readers[i]->model, readers[i]->firmware);

                int loopCount = 1;
                while (SwitchAntenna(readers[i], &antennaId)) {
                    printf("\ntest iteration: %d\n", loopCount);
                    CallGetTags(readers[i], antennaId);
                    if (loopCount++ % 1000 == 0) {
                        printf("call reset\n");
//                        loopCount = 0;
                        SkyeTek_ResetDevice(readers[i]);
                        sleep(2);
                        SkyeTek_DiscoverDevices(&devices);
                        printf("skyetek-mqtt: devices=%d", numDevices);
                        SkyeTek_DiscoverReaders(devices, numDevices, &readers);
                        printf("skyetek-mqtt [%s]: Reader Found: %s-%s-%s-%s-%s\n", ts, readers[i]->rid, readers[i]->friendly,
                               readers[i]->manufacturer, readers[i]->model, readers[i]->firmware);
                    }
                }

                // SkyeTek_SetSystemParameter(readers[i], SYS_MUX_CONTROL, )
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
    usleep(delay);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}


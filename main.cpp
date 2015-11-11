#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#include <time.h>
#include "SkyeTekAPI.h"
#include "SkyeTekProtocol.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     3000L

int main(int argc, char* argv[])
{

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    pubmsg.payload = (void *) PAYLOAD;
    pubmsg.payloadlen = strlen(PAYLOAD);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
                   "on topic %s for client with ClientID: %s\n",
           (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with delivery token %d delivered\n", token);


    LPSKYETEK_DEVICE *devices = NULL;
    LPSKYETEK_READER *readers = NULL;
    LPSKYETEK_TAG *tags = NULL;
    SKYETEK_STATUS status;
    unsigned short count = 0;
    int numDevices = 0;
    int numReaders = 0;
    const int delay = 400000;  	//wait at least 400ms after closing the interface before re-opening (USB enumeration)
    const int tests = 3; 		//number of open/close tests to perform
    const int iterations = 10; 	//number of select tag operations to perform for each test
    int failures = 0;
    int total = 0;

    for(int m = 0; m < tests; m++)
    {
        total ++;
        printf("\n\nTEST #%d\n", total);
        if((numDevices = SkyeTek_DiscoverDevices(&devices)) > 0)
        {
            //printf("example: devices=%d", numDevices);
            if((numReaders = SkyeTek_DiscoverReaders(devices,numDevices,&readers)) > 0 )
            {
                //printf("example: readers=%d\n", numReaders);
                for(int i = 0; i < numReaders; i++)
                {
                    printf("Reader Found: %s-%s-%s\n", readers[i]->manufacturer, readers[i]->model, readers[i]->firmware);
                    for(int k = 0; k < iterations; k++)
                    {
                        printf("\tIteration = %d\n",k);
                        status = SkyeTek_GetTags(readers[0], AUTO_DETECT, &tags, &count);
                        if(status == SKYETEK_SUCCESS)
                        {
                            if(count == 0)
                            {
                                printf("\t\tNo tags found\n");
                            }
                            else
                            {
                                for(int j = 0; j < count; j++)
                                {
                                    TCHAR tagText[256];
                                    memset(tagText,0,256*sizeof(TCHAR));

                                    _stprintf(tagText, "%s-%s", SkyeTek_GetTagTypeNameFromType(tags[j]->type), tags[j]->friendly);
                                    printf("\t\tTag Found: %s\n", tagText);

                                    pubmsg.payload = (void *) &tagText;
                                    pubmsg.payloadlen = strlen(tagText);
                                    pubmsg.qos = QOS;
                                    pubmsg.retained = 0;
                                    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
                                    printf("Waiting for up to %d seconds for publication of %s\n"
                                                   "on topic %s for client with ClientID: %s\n",
                                           (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
                                    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
                                    printf("Message with delivery token %d delivered\n", token);
                                }
                            }
                        }
                        else
                        {
                            printf("ERROR: GetTags failed\n");
                        }
                    }
                    SkyeTek_FreeTags(readers[i],tags,count);
                }
            }
            else
            {
                failures ++;
                printf("failures = %d/%d\n", failures, total);
                printf("ERROR: No readers found\n");
            }
        }
        else
        {
            failures ++;
            printf("failures = %d/%d\n", failures, total);
            printf("ERROR: No devices found\n");
        }
        SkyeTek_FreeDevices(devices,numDevices);
        SkyeTek_FreeReaders(readers,numReaders);
        usleep(delay);
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <time.h>

#define ADDRESS     "broker.hivemq.com:1883"

#define CLIENTID1   "SakaSerbiaPz"
#define CLIENTID2	"SakaSerbiaRegulator1"
#define	CLIENTID3	"SakaSerbiaProcess1"

#define TOPIC1		"saka_pz"
#define TOPIC2		"saka_regulator"
#define TOPIC3      "saka_process"

#define QOS         1
#define TIMEOUT     1000L

MQTTClient client1, client3, client2;
MQTTClient_connectOptions conn_opts2 = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;


int rc;

float y;
int x=0, pz, deltap=5;
char x_payload[20];

volatile MQTTClient_deliveryToken deliveredtoken1;
volatile MQTTClient_deliveryToken deliveredtoken3;

void delivered1(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken1 = dt;
}

int msgarrvd1(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i, number[2]={0,0};
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    
    for(i=0; i<message->payloadlen; i++)
    {
    	number[i]=*payloadptr - '0';
        putchar(*payloadptr++);
    }
    
	pz=number[0]*10+number[1];
    putchar('\n');
    
	MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost1(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void delivered3(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken1 = dt;
}

int msgarrvd3(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char msg[20];
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;

    for(i=0; i<message->payloadlen; i++)
    {
    	msg[i]=*payloadptr;
        putchar(*payloadptr++);
    }
    y=atof(msg);
        
    if ((y-pz)>deltap)
	{
		x=0;
	}
	else if((pz-y)>deltap)
	{
		x=100;
	}

	sprintf(x_payload,"%d",x);
	pubmsg.payload = x_payload;
    pubmsg.payloadlen = (int)strlen(x_payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(client2, TOPIC2, &pubmsg, &token);
    rc = MQTTClient_waitForCompletion(client2, token, TIMEOUT);
    putchar('\n');
    
	MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost3(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    MQTTClient_connectOptions conn_opts1 = MQTTClient_connectOptions_initializer;
    MQTTClient_connectOptions conn_opts3 = MQTTClient_connectOptions_initializer;
	int rc;
    int ch;

    MQTTClient_create(&client1, ADDRESS, CLIENTID1,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts1.keepAliveInterval = 20;
    conn_opts1.cleansession = 1;

    MQTTClient_setCallbacks(client1, NULL, connlost1, msgarrvd1, delivered1);
    
    MQTTClient_create(&client3, ADDRESS, CLIENTID3,
    MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts1.keepAliveInterval = 20;
    conn_opts1.cleansession = 1;
    
    MQTTClient_setCallbacks(client3, NULL, connlost3, msgarrvd3, delivered3);
    MQTTClient_create(&client2, ADDRESS, CLIENTID2,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts2.keepAliveInterval = 20;
    conn_opts2.cleansession = 1;

    if ((rc = MQTTClient_connect(client2, &conn_opts2)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    if ((rc = MQTTClient_connect(client1, &conn_opts1)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC1, CLIENTID1, QOS);

    MQTTClient_subscribe(client1, TOPIC1, QOS);
    
    if ((rc = MQTTClient_connect(client3, &conn_opts3)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    
	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC3, CLIENTID3, QOS);
    
	MQTTClient_subscribe(client3, TOPIC3, QOS);
    
	while(1)
	{
		sleep(1);
	}
    
    MQTTClient_unsubscribe(client3, TOPIC3);
    MQTTClient_disconnect(client3, 10000);
    MQTTClient_destroy(&client3);
    
    MQTTClient_disconnect(client2, 10000);
    MQTTClient_destroy(&client2);

    MQTTClient_unsubscribe(client1, TOPIC1);
    MQTTClient_disconnect(client1, 10000);
    MQTTClient_destroy(&client1);
    return rc;
} 



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <time.h>

#define ADDRESS     "broker.hivemq.com:1883"
#define CLIENTID1   "SakaSerbiaRegulator"
#define CLIENTID2	"SakaSerbiaProcess"

#define TOPIC1      "saka_regulator"
#define TOPIC2		"saka_process"

#define QOS         1
#define TIMEOUT     100L

char payload_test[10];

float y[2]= {0,0};
float x[2]= {0,0};
 
MQTTClient client1, client2;
MQTTClient_connectOptions conn_opts1 = MQTTClient_connectOptions_initializer;
MQTTClient_connectOptions conn_opts2 = MQTTClient_connectOptions_initializer;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient_deliveryToken token;

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i, number[3]={0,0,0};
    char* payloadptr;
    
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
    	number[i]=*payloadptr-'0';
        putchar(*payloadptr++);
        
    }

    putchar('\n');
    
    x[1] = number[0]*100+number[1]*10+number[2];

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    
    int rc;
    int ch;

    MQTTClient_create(&client1, ADDRESS, CLIENTID1,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts1.keepAliveInterval = 20;
    conn_opts1.cleansession = 1;

    MQTTClient_setCallbacks(client1, NULL, connlost, msgarrvd, delivered);
    
    MQTTClient_create(&client2, ADDRESS, CLIENTID2,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts2.keepAliveInterval = 20;
    conn_opts2.cleansession = 1;
    
    if ((rc = MQTTClient_connect(client1, &conn_opts1)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    
    if ((rc = MQTTClient_connect(client2, &conn_opts2)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    
	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC1, CLIENTID1, QOS);
    MQTTClient_subscribe(client1, TOPIC1, QOS);
    
    while(1){
		sleep(1);
				
		y[1] = (x[1]+x[0])*1/21+y[0]*19/21;
		y[0]=y[1];
		x[0]=x[1];
	    
		sprintf(payload_test,"%f",y[1]);
	    
	    pubmsg.payload = payload_test;
	    pubmsg.payloadlen = (int)strlen(payload_test);
	    pubmsg.qos = QOS;
	    pubmsg.retained = 0;
	    
	    MQTTClient_publishMessage(client2, TOPIC2, &pubmsg, &token);

	    rc = MQTTClient_waitForCompletion(client2, token, TIMEOUT);
	}

    MQTTClient_unsubscribe(client1, TOPIC1);
    MQTTClient_disconnect(client1, 10000);
    MQTTClient_destroy(&client1);
    
	MQTTClient_disconnect(client2, 10000);
    MQTTClient_destroy(&client2);
    
	return rc;
} 


// https://github.com/256dpi/arduino-mqtt

#include <Ethernet.h>
#include <MQTT.h>
#include <string.h>

byte mac[] = { 0x5A, 0xA2, 0xDA, 0x0D, 0x56, 0x7A };  // MAC-Adresse des Ethernet-Shield
byte ip[] = { 192, 168, 178, 2 };                       // <- change to match your network

EthernetClient net;
MQTTClient client;

unsigned long lastMillis = 0;
int reconnect = 0;

//MQTT-Broker config
const char mqttClientId[] = "homebecketal"; //The MQTT Client ID
const char mqttUsername[] = "homeassistant";
const char mqttPassword[] = "Ash1agohd7dif7IfaeF8xahdae3phaeQuieXuiXiw2ieVei7izeiqu7eguemeaNg"; //Username and password for your MQTT-Broker

// homebecketal
// Ein Aus Off ist für Ausgänge
#define Ein 0
#define Aus 1
#define Off 1
#define On 0

#define WifiEin 0
#define WifiAus 1

#define GetEin 0
#define GetAus 1

// // Eingänge ab 7.1.23
#define INFLURUNTEN 0
#define INWOHNZIMMER 1    // Für Wohnzimmer habe ich die Klemme 4 auf der Klemmenleiste genommen. Die zweite war defekt. Eingang aud dem Arduino ist aber 1
#define INFLUROBEN 2
#define INKUECHE 3
//  #define INAUTOMATIK 
// // Ausgänge ab 7.1.23
#define OUTFLURUNTEN 5
#define OUTWOHNZIMMER 6
#define OUTFLUROBEN 7
#define OUTKUECHE 8
#define OUTAUTOMATIK 9

// Variablen
int i = 0;
int ii = 0;
int j = 0;
int zlWkp = 0;			// Zähler WakeUp
int liInState = 0;
int liStateWifi = 0;	// Abendlicht Weihnachten
char buf[100];

// Array Zuordnungen
int laiOut[4] = { OUTFLURUNTEN,OUTWOHNZIMMER,OUTFLUROBEN,OUTKUECHE };
int laiOutOnNextOn[4] = { OUTFLUROBEN,101,OUTFLURUNTEN,OUTFLURUNTEN }; 	// Ausgang Ein, nächster Ausgang Ein (999 ist nix))
int laiOutOnNextOff[4] = { 999,999,999,999 };							              // Ausgang Ein, nächster Ausgang Aus (ab 100 ist Wifi)
int laiOutOffNextOn[4] = { 999,OUTFLURUNTEN,999,OUTFLURUNTEN };			    // Ausgang Aus, nächster Ausgang Ein
int laiOutOffNextOff[4] = { OUTFLUROBEN,999,OUTFLURUNTEN,999 }; 			  // Ausgang Aus, nächster Ausgang Aus
int laiOutOnAndOff[4] = { 999,101,999,999 };	    						          // Ausgang Ein, sofort anderen Ausgang auf aus (z.B. Wifilampe Wohnz.)
// int laiOutRandom [4] = {OUTFLURUNTEN,OUTWOHNZIMMER,OUTFLUROBEN,OUTKUECHE};				// Todo Ausgang für Random Todo Ulf außer Betrieb
int laiOutState[4] = { Aus,Aus,Aus,Aus };
int laiOutStateNew[4] = { Ein,Ein,Ein,Ein };
// hier nur die Hardware (nur 4)
int laiIn[4] = { INFLURUNTEN,INWOHNZIMMER,INFLUROBEN,INKUECHE };
int laiInState[4] = { 0,0,0,0 };            // Statuswechsel erkennen
int laiInStateOld[4] = { 0,0,0,0 };         // Statuswechsel erkennen
int laiZl1[4] = { 0,0,0,0 };	            //Timer 2.te Schaltung
int laiZl2[4] = { 0,0,0,0 };	            //Timer 3.te Schaltung Alles aus
int laiZl3[4] = { 0,0,0,0 };	            //Timer 4.te Schaltung Random
int laiZl10[4] = { 0,0,0,0 };  	          // Wert Timer Zeit Flurlicht
int laiTimer[4] = { 1200,0,1200,0 };	      // Zeit Timer erste Schaltung
int laiZlOnNextOn[4] = { 0,0,0,0 };	      // Zähler für 2.te Schaltung On On
int laiTimerOnNextOn[4] = { 1200,0,1200,0 };		// Flurlicht Zeitsteuerung Wert = Sekunde *10
int laiZlOffNextOn[4] = { 0,0,0,0 };	    // Zähler für 2.te Schaltung Off On
int laiTimerOffNextOn[4] = { 1200,1200,0,1200 };
int laiStateWifi[4] = { 0,0,0,0 }; 	      // Speichern Status Wifi Ausgang 0 = Stehlampe WZM
int liToggle[4] = { 0,0,0,0 };

// Mqtt String trimmen
char *trim(char *s) {
	char *ptr;
	if (!s)
		return NULL;   // handle NULL string
	if (!*s)
		return s;      // handle empty string
	for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
	ptr[1] = '\0';
	return s;
}

// Mqtt Connect
void connect() {
	Serial.print("connecting...");
	while (!client.connect(mqttClientId, mqttUsername, mqttPassword)) {
		Serial.print(".");
		delay(1000);
	}
	Serial.println("\nconnected!");
	client.subscribe("100"); //subscribe to a topic
	delay(1000);
}

// Mqtt Nachricht erhalten
void messageReceived(String &topic, String &payload) {

	const char* myPayload = (const char*)payload.c_str();

	const char* myPayload2 = trim(myPayload);
	// Serial.println(myPayload2);
	if (strcmp(myPayload, "reconnect") == 0)
	{
		Serial.println("\nreconnect gesetzt");
		reconnect = 1;
	}
	// Flur unten Ein
	if (strcmp(myPayload2, "FLU-EIN") == 0)
	{
		Serial.println("\nFLU EIN");
		laiOutState[0] = On;
		digitalWrite(OUTFLURUNTEN, On);
	}
	// Flur unten Aus   
	if (strcmp(myPayload2, "FLU-AUS") == 0)
	{
		Serial.println("\nFLU AUS");
		laiOutState[0] = Off;
		digitalWrite(OUTFLURUNTEN, Off);
	}
	// Wohnzimmer Ein
	if (strcmp(myPayload2, "WZM-EIN") == 0)
	{
		Serial.println("\nWZM EIN");
		laiOutState[1] = On;
		digitalWrite(OUTWOHNZIMMER, On);
	}
	// Wohnzimmer Aus   
	if (strcmp(myPayload2, "WZM-AUS") == 0)
	{
		Serial.println("\nWZM AUS");
		laiOutState[1] = Off;
		digitalWrite(OUTWOHNZIMMER, Off);
	}
	// Flur Oben Ein
	if (strcmp(myPayload2, "FLO-EIN") == 0)
	{
		Serial.println("\nFLO EIN");
		laiOutState[2] = On;
		digitalWrite(OUTFLUROBEN, On);
	}
	// Flur Oben Aus   
	if (strcmp(myPayload2, "FLO-AUS") == 0)
	{
		Serial.println("\nFLO AUS");
		laiOutState[2] = Off;
		digitalWrite(OUTFLUROBEN, Off);
	}
	// Küche Ein
	if (strcmp(myPayload2, "KUE-EIN") == 0)
	{
		Serial.println("\nKUE EIN");
		laiOutState[3] = On;
		digitalWrite(OUTKUECHE, On);
	}
	// Küche Aus   
	if (strcmp(myPayload2, "KUE-AUS") == 0)
	{
		Serial.println("\nKUE AUS");
		laiOutState[3] = Off;
		digitalWrite(OUTKUECHE, Off);
	}
	// Automatik Ein
	if (strcmp(myPayload2, "AUT-EIN") == 0)
	{
		Serial.println("\nAUT EIN");
		digitalWrite(OUTAUTOMATIK, On);
	}
	// Automatik Aus   
	if (strcmp(myPayload2, "AUT-AUS") == 0)
	{
		Serial.println("\nAUT AUS");
		digitalWrite(OUTAUTOMATIK, Off);
	}
}

void setup() {
	Serial.begin(9600);
	Ethernet.begin(mac, ip);

	// Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
	// by Arduino. You need to set the IP address directly.
	client.begin("192.168.178.44", net);  // HomeAssistant
	client.onMessage(messageReceived);
	connect();
	//  Definition der Ausgänge
	pinMode(OUTFLURUNTEN, OUTPUT);		        //5   4 = GPIO 23 Flur unten
	pinMode(OUTWOHNZIMMER, OUTPUT);  	        //6 	7 =	GPIO 4  Wohnzimmer
	pinMode(OUTFLUROBEN, OUTPUT);		          //7 	0 =	GPIO 17 Flur oben  
	pinMode(OUTKUECHE, OUTPUT);  		          //8 	2 =	GPIO 27 Küche
	pinMode(OUTAUTOMATIK, OUTPUT);   	        //9 	3 =	GPIO 22 Umschaltung Automatik Hand
	// Definition der Eingänge
	pinMode(INFLURUNTEN, INPUT_PULLUP);    	  //0 21 = GPIO 5  Flur unten
	pinMode(INWOHNZIMMER, INPUT_PULLUP);    	//1 22 = GPIO 6  Wohnzimmer
	pinMode(INFLUROBEN, INPUT_PULLUP);    		//2 23 = GPIO 13 Flur oben
	pinMode(INKUECHE, INPUT_PULLUP);    		  //3 25 = GPIO 26 Küche

	// Initialisierung Ausgänge
	digitalWrite(OUTFLURUNTEN, Off);  	// Flur unten
	digitalWrite(OUTWOHNZIMMER, Off);	  // Wohnzimmer
	digitalWrite(OUTFLUROBEN, Off);		  // Flur oben
	digitalWrite(OUTKUECHE, Off);		    // Küche
	digitalWrite(OUTAUTOMATIK, On);	    // Automatik immer an
}

void loop() {
	client.loop();

	if (!client.connected() && reconnect == 0) {
		connect();
	}
	if (reconnect == 1)
	{
		Serial.println("\nChoosenreconnected!");
		client.disconnect();
		delay(10);
		reconnect = 0;
	}
	// ---------------------Steuerungsteil-----------------------------------
	liToggle[ii] = 0;
	laiIn[ii] = Aus;

	switch (ii) {
	case 0:
		laiIn[ii] = digitalRead(INFLURUNTEN);
		break;
	case 1:
		laiIn[ii] = digitalRead(INWOHNZIMMER);
		break;
	case 2:
		laiIn[ii] = digitalRead(INFLUROBEN);
		break;
	case 3:
		laiIn[ii] = digitalRead(INKUECHE);
		break;
	}

	//sprintf(buf,"Eingang gelesen %d %d \n", laiIn[ii],ii);      
	//sprintf(buf,"Eingang gelesen %d %d \n", liInState, ii);      
	//Serial.println(buf);

	// Wenn ein Taster gedrückt wurde
	if (laiIn[ii] == Ein)
	{
		// Normale Schaltung
		if (laiZl1[ii] == 0)
		{
			// Timer anstoßen für weitere Schaltungen
			laiZl1[ii]++;
			laiZl2[ii]++;
			laiZl3[ii]++;

			// client.publish("100", "Taster betaetigt");
			// sprintf(buf,"Taster Nr %d  betaetigt\n", laiIn[ii]);
			// Serial.println(buf);

			// Zustand des Ausgangs lesen
			laiOutState[ii] = digitalRead(laiOut[ii]);

			// Neuer Zustand soll werden : Aus---------------------------------------------------------------------------
			if (laiOutState[ii] == On)
			{
				laiOutStateNew[ii] = Off;
				// client.publish("100", "Ausgang Aus");
				// sprintf(buf, "Ausgang aus %d \n",laiOutOnAndOff[ii]);
				// Serial.println(buf);
				// Timer aus
				laiZl10[ii] = 0;
			}
			// Neuer Zustand soll werden : Ein--------------------------------------------------------------------------
			if (laiOutState[ii] == Off)
			{
				laiOutStateNew[ii] = On;
				// client.publish("100", "Ausgang Ein");

				// Mqtt Schalter Ein
				// Ausgang Soll Ein werden, anderer Ausgang sofort auf Ein (z.B. Wifi Lampe Wohnz.)
				if (laiOutOnAndOff[ii] < 999 && laiOutOnAndOff[ii]>100 && laiStateWifi[1] == 0 && liToggle[ii] == 0)
				{
					// sprintf(buf, "Ausgang setzen %d \n",laiOutOnAndOff[ii]);
					// Serial.println(buf);
					// Wohnzimmer Stehlampe Aus
					laiStateWifi[1] = 1;
					client.publish("100", "WZM-STL-EIN");
					liToggle[ii] = 1;
				}
				// MQtt Schalter Aus
				// Ausgang Soll Ein werden, anderer Ausgang sofort auf Aus (z.B. Wifi Lampe Wohnz.)
				if (laiOutOnAndOff[ii] < 999 && laiOutOnAndOff[ii]>100 && laiStateWifi[1] == 1 && liToggle[ii] == 0)
				{
					// sprintf(buf, "Ausgang setzen %d \n",laiOutOnAndOff[ii]);
					// Serial.println(buf);
					// Wohnzimmer Stehlampe Aus
					laiStateWifi[1] = 0;
					liToggle[ii] = 1;
					client.publish("100", "WZM-STL-AUS");
				}

				// Timer anstoßen für Zeit Treppenhaus
				if (laiTimer[ii] > 0)
				{
					laiZl10[ii] = 1;  // Zählt dann weiter siehe unten
				}
			}

			// printf("Zustand soll %d \n",laiOutStateNew[ii]);					
			if (laiOutState[ii] != laiOutStateNew[ii])
			{
				// Ausgang setzen
				digitalWrite(laiOut[ii], laiOutStateNew[ii]);
				// sprintf(buf,"Ausgang %d \n",laiOut[ii]);
				// Serial.println(buf);
				// sprintf(buf,"Zustand gesetzt %d \n",laiOutStateNew[ii]);
				// Serial.println(buf);
			}
		}

		// Zähler 2te Schaltung
		if (laiZl1[ii] > 0)
		{
			laiZl1[ii]++;
			// if(laiZl1[ii] == 5)
			// {
			// Serial.println(buf);
			// //printf("Zähler in Schleife %d \n",laiZl1[ii]);
			// }
		}

		// zweite Schaltung				
		if (laiZl1[ii] == 6)
		{
			//Serial.println("2te Schaltung erreicht");
			// Ausgang Ein, nächster Ausgang Ein
			if (laiOutOnNextOn[ii] < 999 && laiOutState[ii] == Off)
			{
				// Timer 2.te Schaltung anstoßen für Zeit Treppenhaus
				if (laiTimerOnNextOn[ii] > 0)
				{
					laiZlOnNextOn[ii] = 0;
					laiZlOnNextOn[ii]++;

					// Timer wieder löschen für Zeit Treppenhaus
					// In der zweiten Schaltung wird der Timer hochgesetzt auf Dauerlicht
					laiZl10[ii] = 0;
				}
				// Digitalausgänge < 100
				if (laiOutOnNextOn[ii] <= 100)    // nur Hardwareschalter (<100)
				{
					// printf("Ausgang setzen Ein Ein %d \n",laiOutOnNextOn[ii]);
					// sprintf(buf,"zweite Schaltung Zustand gesetzt %d \n",laiOutOnNextOn[ii]);
					// Serial.println(buf);
					// client.publish("100", "2te Schaltung Treppenhaus Ein");
					digitalWrite(laiOutOnNextOn[ii], On);
				}
				laiZl1[ii]++;
			}

			// Ausgang Ein, nächster Ausgang Aus
			if (laiOutOnNextOff[ii] < 999 && laiOutState[ii] == Off)
			{
				// printf("Ausgang setzen Ein Aus %d \n",laiOutOnNextOff[ii]);																						
				digitalWrite(laiOutOnNextOff[ii], Off);
				laiZl1[ii]++;
				// Timer aus
				laiZl10[ii] = 0;
			}
			// Ausgang Aus, nächster Ausgang Ein
			if (laiOutOffNextOn[ii] < 999 && laiOutState[ii] == On)
			{
				// Timer 2.te Schaltung anstoßen für Zeit Treppenhaus
				if (laiTimerOffNextOn[ii] > 0)
				{
					laiZlOffNextOn[ii] = 0;
					laiZlOffNextOn[ii]++;
				}
				// printf("Ausgang setzen Aus Ein %d \n",laiOutOffNextOn[ii]);						
				digitalWrite(laiOutOffNextOn[ii], On);
				laiZl1[ii]++;
			}
			// Ausgang Aus, nächster Ausgang Aus
			// printf("Ausgang vor  setzen Aus Aus %d \n",laiOutOffNextOff[ii]);																
			if (laiOutOffNextOff[ii] < 999 && laiOutState[ii] == On)
			{
				// printf("Ausgang in setzen Aus Aus %d \n",laiOutOffNextOff[ii]);												
				digitalWrite(laiOutOffNextOff[ii], Off);
				laiZl1[ii]++;
				// Timer Aus
				laiZl10[ii] = 0;
			}
		}
		// Zähler 3te Schaltung > Alles aus
		if (laiZl2[ii] > 0)
		{
			laiZl2[ii]++;
			if (laiZl2[ii] == 12)
			{
				// client.publish("100", "3te Schaltung erreicht Alles Aus");
				for (j = 0; j < 4; j++)
				{
					digitalWrite(laiOut[j], Off);
					// printf("Zähler 3te Schaltung %d \n",laiZl2[ii]);
					// Timer wieder löschen für Zeit Treppenhaus
					laiZl10[j] = 0;
				}
			}
		}

	}
	else
	{
		laiZl1[ii] = 0;	// 2.te Schaltung
		laiZl2[ii] = 0;	// 3.te Schaltung alles aus
		laiZl3[ii] = 0;	// 4.te Schaltung Random Tageszeitabhängig -> außer Betrieb
	}

	// Timer wurden angestoßen
	// 1.te Schaltung Nach Ablauf wird Abgeschaltet
	if (laiTimer[ii] > 0 && laiZl10[ii] > 0)
	{
		// Hochzaehlen
		laiZl10[ii] ++;

		// sprintf(buf,"Zaehler %d \n",laiZl10[ii]);
		// Serial.println(buf);

		// Warnung für Aus Ausschalten
		if (laiZl10[ii] == (laiTimer[ii] - 100))
		{
			digitalWrite(laiOut[ii], Off);
		}
		// Warnung für Aus Einschalten
		if (laiZl10[ii] == (laiTimer[ii] - 50))
		{
			digitalWrite(laiOut[ii], On);
		}

		if (laiZl10[ii] == laiTimer[ii])
		{
			digitalWrite(laiOut[ii], Off);
			// Zähler zurücksetzen
			laiZl10[ii] = 0;
		}

	}
	// 2.te Schaltung Nach Ablauf wird Abgeschaltet
	if (laiTimerOnNextOn[ii] > 0 && laiZlOnNextOn[ii] > 0)
	{
		laiZlOnNextOn[ii] ++;

		if (laiZlOnNextOn[ii] == laiTimerOnNextOn[ii])
		{
			digitalWrite(laiOut[ii], Off);
			// Zähler zurücksetzen
			laiZlOnNextOn[ii] = 0;
		}
	}
	// 2.te Schaltung Nach Ablauf wird Abgeschaltet
	if (laiTimerOffNextOn[ii] > 0 && laiZlOffNextOn[ii] > 0)
	{
		laiZlOffNextOn[ii] ++;

		if (laiZlOffNextOn[ii] == laiTimerOffNextOn[ii])
		{
			digitalWrite(laiOut[ii], Off);
			// Zähler zurücksetzen
			laiZlOffNextOn[ii] = 0;
		}
	}

	// if(laiZl10[0] > 0)
	// {
	//   sprintf(buf,"Zaehler %d \n",laiZl10[0]);
	//   Serial.println(buf);
	// }

	delay(50);

	// Zähler
	ii++;
	if (ii > 3) ii = 0;
}


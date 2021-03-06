// WiringPi-Api einbinden
#include <wiringPi.h>

// C-Standardbibliothek einbinden
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
// libcurl HTTP Aufrufe
#include <arm-linux-gnueabihf/curl/curl.h>
#include <arm-linux-gnueabihf/curl/easy.h>
// Sundown
#include <math.h>
#define PI 3.1415926
#define ZENITH -.83
// Ios
#define AUS 1
#define EIN 0
#define WIFIEIN 1
#define WIFIAUS 0
#define GETEIN 1
#define GETAUS 0

void countTimerUp();		// Timer für Flurlicht
void randomDigitalWrite();	// Random-Modus ausführen Extra Schleife
void showQuit();			// Quittierung für RandomModus
int  getHome(int);
int  WifiSwitch(int, int);   // Schalten eines Wifi-Verbrauchers ab Nr. 100
int  WifiSwitchMosquitto(int, int); // Schalten eines Wifi Verbrauchers mit z.B. mosquitto_pub -h localhost -t switch001/off -m 1
int  espWakeUp(int);
int  eveningLight(int);		// Abendlicht Weihnachten
float calculateSunset(int, int, int, float, float, int, int);   // Sonnenuntergang berechnen
float calculateSunrise(int, int, int, float, float, int, int);   // Sonnenaufgang berechnen

int main() {
	int i = 1;			// Hinein in die Hauptschleife
	int ii = 0;
	int j = 0;
	int zlWkp = 0;			// Zähler WakeUp
	int liInState = 0;
	int liFlagFone = 0;		// Bekanntes Handy ermittelt
	int liStateWifi = 0;	// Abendlicht Weihnachten

	// Array Zuordnungen
	int laiOut[4] = { 24,7,0,2 };
	int laiOutOnNextOn[4] = { 0,101,24,24 }; 		// Ausgang Ein, nächster Ausgang Ein (999 ist nix))
	int laiOutOnNextOff[4] = { 999,999,999,999 };	// Ausgang Ein, nächster Ausgang Aus (ab 100 ist Wifi)
	int laiOutOffNextOn[4] = { 999,24,999,24 };		// Ausgang Aus, nächster Ausgang Ein
	int laiOutOffNextOff[4] = { 0,999,24,999 }; 		// Ausgang Aus, nächster Ausgang Aus
	int laiOutOnAndOff[4] = { 999,101,999,999 };	    // Ausgang Ein, sofort anderen Ausgang auf aus (z.B. Wifilampe Wohnz.)
	int laiOutRandom[4] = { 24,7,0,2 };				// Ausgang für Random
	int laiOutState[4] = { AUS,AUS,AUS,AUS };
	int laiOutStateNew[4] = { EIN,EIN,EIN,EIN };
	// hier nur die Hardware (nur 4)
	int laiIn[4] = { 27,29,28,6 };
	int laiZl1[4] = { 0,0,0,0 };	//Timer 2.te Schaltung
	int laiZl2[4] = { 0,0,0,0 };	//Timer 3.te Schaltung Alles aus
	int laiZl3[4] = { 0,0,0,0 };	//Timer 4.te Schaltung Random
	int laiZl10[4] = { 0,0,0,0 };  	// Wert Timer Zeit Flurlicht
	int laiTimer[4] = { 7000,0,4000,0 };	// Diese Ausgänge gehen mit diesen Zeiten auf den Timer
	int laiZlOnNextOn[4] = { 0,0,0,0 };	// Zähler für 2.te Schaltung On On
	int laiTimerOnNextOn[4] = { 5000,0,4000,0 };		// Flurlicht Zeitsteuerung Wert = Sekunde
	int laiZlOffNextOn[4] = { 0,0,0,0 };	// Zähler für 2.te Schaltung Off On
	int laiTimerOffNextOn[4] = { 5000,4000,0,2000 };
	int laiStateWifi[4] = { 0,0,0,0 }; 	// Speichern Status Wifi Ausgang

	// Starte die WiringPi-Api (wichtig)
	if (wiringPiSetup() == -1)
		return 1;

	// Definition der Ausgänge
	// Wichtig: Hier wird das WiringPi Layout verwendet
	pinMode(24, OUTPUT);	// Flur unten  
	pinMode(7, OUTPUT);  	// Wohnzimmer
	pinMode(0, OUTPUT);		// Flur oben  
	pinMode(2, OUTPUT);  	// Küche
	pinMode(3, OUTPUT);   	// Umschaltung Automatik Hand
	// Definition der Eingänge
	pinMode(27, INPUT);    	// Flur unten
	pinMode(29, INPUT);    	// Wohnzimmer
	pinMode(28, INPUT);    	// Flur oben
	pinMode(6, INPUT);     	// Küche
	// Initialisierung Ausgänge Test
	// digitalWrite(24, EIN);  // Flur unten
	// digitalWrite(7, EIN);	// Wohnzimmer
	// digitalWrite(0, EIN);	// Flur oben
	// digitalWrite(2, EIN);	// Küche
	// digitalWrite(3, EIN);	// Automatik // Außer Betrieb Ulf!
	// Initialisierung Ausgänge
	digitalWrite(24, AUS);  // Flur unten
	digitalWrite(7, AUS);	// Wohnzimmer
	digitalWrite(0, AUS);	// Flur oben
	digitalWrite(2, AUS);	// Küche
	digitalWrite(3, EIN);	// Automatik EIN

	// Initialisierung der Eingänge
	digitalWrite(27, 0);
	digitalWrite(28, 0);
	digitalWrite(29, 0);
	digitalWrite(6, 0);
	delay(1000);
	printf("Hauptschleife läuft\n");

	// i=0;			// Außer Betrieb Ulf
	// Hauptschleife
	while (i == 1)
	{
		ii = 0;

		// Weihnachtslicht
		liStateWifi = eveningLight(laiStateWifi[2]);
		laiStateWifi[2] = liStateWifi;

		// ESPs wach halten
		zlWkp++;

		if (zlWkp == 60)
		{
			zlWkp = espWakeUp(zlWkp);
		}

		while (ii <= 3) 		// Schleife durch alle Eingänge (nur Digitaleingänge)
		{
			// Staus der Eingänge lesen
			liInState = digitalRead(laiIn[ii]);

			// Wenn ein Taster gedrückt ist oder jemand nach Hause kommt
			if (liInState == GETEIN || liFlagFone == 1)
			{
				// Normale Schaltung
				if (laiZl1[ii] == 0)
				{
					// Timer anstoßen für weitere Schaltungen
					laiZl1[ii]++;
					laiZl2[ii]++;
					laiZl3[ii]++;

					// Zustand des Ausgangs lesen
					laiOutState[ii] = digitalRead(laiOut[ii]);
					// printf("Zustand %d \n",laiOutState[ii]);

					// Neuer Zustand soll werden : AUS
					if (laiOutState[ii] == EIN)
					{
						laiOutStateNew[ii] = AUS;
						// Zeit aus
						laiZl10[ii] = 0;
					}
					// Neuer Zustand soll werden : EIN
					if (laiOutState[ii] == AUS)
					{
						laiOutStateNew[ii] = EIN;

						// Ausgang Soll Ein werden, anderer Ausgang sofort auf Aus (z.B. Wifi Lampe Wohnz.)
						if (laiOutOnAndOff[ii] < 999 && laiOutOnAndOff[ii]>100 && laiStateWifi[1] == 1)
						{
							// printf("Ausgang setzen Ein AUS %d \n",laiOutOnAndOff[ii]);
							laiStateWifi[1] = WifiSwitch(laiOutOnNextOn[ii], WIFIAUS);
						}

						// Timer anstoßen für Zeit Treppenhaus
						if (laiTimer[ii] > 0)
						{
							laiZl10[ii] = 0;
							laiZl10[ii]++;
						}
					}
					// printf("Zustand soll %d \n",laiOutStateNew[ii]);					
					if (laiOutState[ii] != laiOutStateNew[ii])
					{
						// Ausgang setzen
						digitalWrite(laiOut[ii], laiOutStateNew[ii]);
						// printf("Ausgang %d \n",laiOut[ii]);
						// printf("Zustand gesetzt %d \n",laiOutState[ii]);

					}
				}

				// Zähler 2te Schaltung
				if (laiZl1[ii] > 0)
				{
					laiZl1[ii]++;
					//printf("Zähler in Schleife %d \n",laiZl1[ii]);
				}

				// zweite Schaltung				
				if (laiZl1[ii] == 8)
				{
					// Ausgang Ein, nächster Ausgang Ein
					if (laiOutOnNextOn[ii] < 999 && laiOutState[ii] == AUS)		// nur Hardwareschalter (<100)
					{
						// Timer 2.te Schaltung anstoßen für Zeit Treppenhaus
						if (laiTimerOnNextOn[ii] > 0)
						{
							laiZlOnNextOn[ii] = 0;
							laiZlOnNextOn[ii]++;

							// Timer wieder löschen für Zeit Treppenhaus
							// In der zweiten Schaltung wird auf Dauerlicht geschaltet
							laiZl10[ii] = 0;
						}
						// Digitalausgänge < 100
						if (laiOutOnNextOn[ii] <= 100)
						{
							// printf("Ausgang setzen Ein Ein %d \n",laiOutOnNextOn[ii]);
							digitalWrite(laiOutOnNextOn[ii], EIN);
						}
						// Wifi-Ausgänge
						if (laiOutOnNextOn[ii] > 100 && laiOutOnNextOn[ii] < 999)
						{
							// printf("Hauptschleife Ausgang setzen Ein Ein %d \n",laiOutOnNextOn[ii]);
							laiStateWifi[1] = WifiSwitch(laiOutOnNextOn[ii], WIFIEIN);
						}
						laiZl1[ii]++;
					}
					// Ausgang Ein, nächster Ausgang Aus
					if (laiOutOnNextOff[ii] < 999 && laiOutState[ii] == AUS)
					{
						// printf("Ausgang setzen Ein Aus %d \n",laiOutOnNextOff[ii]);																						
						digitalWrite(laiOutOnNextOff[ii], AUS);
						laiZl1[ii]++;
						// Timer aus
						laiZl10[ii] = 0;
					}
					// Ausgang Aus, nächster Ausgang Ein
					if (laiOutOffNextOn[ii] < 999 && laiOutState[ii] == EIN)
					{
						// Timer 2.te Schaltung anstoßen für Zeit Treppenhaus
						if (laiTimerOffNextOn[ii] > 0)
						{
							laiZlOffNextOn[ii] = 0;
							laiZlOffNextOn[ii]++;
						}
						// printf("Ausgang setzen Aus Ein %d \n",laiOutOffNextOn[ii]);						
						digitalWrite(laiOutOffNextOn[ii], EIN);
						laiZl1[ii]++;
					}
					// Ausgang Aus, nächster Ausgang Aus
					// printf("Ausgang vor  setzen Aus Aus %d \n",laiOutOffNextOff[ii]);																
					if (laiOutOffNextOff[ii] < 999 && laiOutState[ii] == EIN)
					{
						// printf("Ausgang in setzen Aus Aus %d \n",laiOutOffNextOff[ii]);												
						digitalWrite(laiOutOffNextOff[ii], AUS);
						laiZl1[ii]++;
						// Timer Aus
						laiZl10[ii] = 0;
					}
				}

				// Zähler 3te Schaltung
				if (laiZl2[ii] > 0)
				{
					laiZl2[ii]++;
					if (laiZl2[ii] == 35)
					{
						// printf("Zähler 3te Schaltung %d \n",laiZl2[ii]);
						for (j = 0; j < 4; j++)
						{
							digitalWrite(laiOut[j], AUS);
							// printf("Zähler 3te Schaltung %d \n",laiZl2[ii]);
							// Timer wieder löschen für Zeit Treppenhaus
							laiZl10[j] = 0;
						}
					}
				}
				// Zähler 4te Schaltung Random Programm, Tageszeitabhängig
				if (laiZl3[ii] > 0)
				{
					laiZl3[ii]++;
					if (laiZl3[ii] == 50)
					{
						// Alle Zähler auf 0 setzen							
						laiZl1[ii] = 0;
						laiZl2[ii] = 0;
						laiZl3[ii] = 0;
						// printf("Zähler 4te Schaltung Random start %d \n",laiZl3[ii]);						
						randomDigitalWrite(laiOutRandom, 4);
						// printf("Zähler 4te Schaltung Random beendet %d \n",laiZl3[ii]);												
						// Timer wieder löschen für Zeit Treppenhaus
						laiZl10[ii] = 0;
						// Aber Flur unten soll anbleiben (mit Zeit)
						digitalWrite(24, EIN);
						laiZl10[0] = 6000;
					}
				}
			}
			else
			{
				laiZl1[ii] = 0;	// 2.te Schaltung
				laiZl2[ii] = 0;	// 3.te Schaltung alles aus
				laiZl3[ii] = 0;	// 4.te Schaltung Random Tageszeitabhängig (Quittierung 3*blinken)
			}

			ii++;
		}

		// Timer wurden angestoßen
		// Nach Ablauf wird Abgeschaltet
		countTimerUp(laiZl10, 4, laiTimer, 4, laiOut, 4); // Normale Schaltung
		countTimerUp(laiZlOnNextOn, 4, laiTimerOnNextOn, 4, laiOutOnNextOn, 4); // 2te Schaltung
		countTimerUp(laiZlOffNextOn, 4, laiTimerOffNextOn, 4, laiOutOffNextOn, 4); //2te Schaltung						

		delay(100);
	}
}

// Die Zähler für die Zeitsteuerung bearbeiten
void countTimerUp(int aaiZl[], int zl1, int aaiTimer[], int zl2, int laiOut[], int zl3)
{
	int i = 0;

	for (i = 0; i < 4; i++)
	{
		if (aaiZl[i] > 0)
		{
			aaiZl[i]++;
			// Vorwarnung ausschalten
			if (aaiZl[i] == (aaiTimer[i] - 100))
			{
				digitalWrite(laiOut[i], AUS);
			}
			if (aaiZl[i] == (aaiTimer[i] - 95))
			{
				digitalWrite(laiOut[i], EIN);
			}

			if (aaiZl[i] >= aaiTimer[i])
			{
				digitalWrite(laiOut[i], AUS);
				aaiZl[i] = 0;
			}
			// printf("Zähler 0 %d \n",aaiZl[i]);		
			// printf("Ausgang 0 %d \n",laiOut[i]);					
		}
	}
}

// ESPs wach halten (mit ping)
int espWakeUp(int aiWkp)
{

	// printf("Vor dem system()-Aufruf\n");
	system("ping -c 1 192.168.178.46&");
	system("ping -c 1 192.168.178.43&");
	// printf("Nach dem system()-Aufruf\n");
	// printf("Return %d \n",  EXIT_SUCCESS);
	// printf("Wkp %d \n",aiWkp);
	aiWkp = 0;
	return aiWkp;
}

// Abendlicht Wifi Steckdose Weihnachtslicht
int eveningLight(int aiStateWifi)
{
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	int year = aTime->tm_year + 1900;
	int month = aTime->tm_mon + 1;
	int day = aTime->tm_mday;
	int hour = aTime->tm_hour;
	int min = aTime->tm_min;
	// int sec=aTime->tm_sec;

	// Sonnenuntergang
	// 53.18619,8.60846 zu Hause
	float localTs = calculateSunset(year, month, day, 53.18619, 8.60846, 1, 0);
	double sunset_hr = fmod(24 + localTs, 24.0);
	double sunset_min = modf(fmod(24 + localTs, 24.0), &sunset_hr) * 60;
	int ss_hr = (int)sunset_hr;
	int ss_min = (int)sunset_min;

	// Sonnenaufgang
	// 53.18619,8.60846 zu Hause
	float localTr = calculateSunrise(year, month, day, 53.18619, 8.60846, 1, 0);
	double sunrise_hr = fmod(24 + localTr, 24.0);
	double sunrise_min = modf(fmod(24 + localTr, 24.0), &sunrise_hr) * 60;
	int sr_hr = (int)sunrise_hr;
	int sr_min = (int)sunrise_min;

	//printf("%u\n",year);
	//printf("%u\n",month);
	//printf("%u\n",day);
	//printf("%u:%u\n",ss_hr,ss_min);    
	//printf("%u:%u\n",sr_hr,sr_min);    
	//printf("%u:%u\n",hour,min);    

	// Abends
	if (hour == ss_hr && min == ss_min)
	{
		// printf("Stunde ein %d \n",hour);
		if (aiStateWifi == 0)
		{
			aiStateWifi = WifiSwitch(102, WIFIEIN);
		}
	}
	if (hour == 23 && min == 23)
	{
		// printf("Stunde aus %d \n",hour);
		if (aiStateWifi == 1)
		{
			aiStateWifi = WifiSwitch(102, WIFIAUS);
		}
	}

	// Morgens, aber nur wenn um 7 noch nicht die Sonne an ist
	if (sr_hr >= 7 && sr_min >= 2)
	{
		if (hour == 7 && min == 2)
		{
			// printf("Stunde aus %d \n",hour);
			if (aiStateWifi == 1)
			{
				aiStateWifi = WifiSwitch(102, WIFIEIN);
			}
		}

		if (hour == sr_hr && min == sr_min)
		{
			// printf("Stunde ein %d \n",hour);
			if (aiStateWifi == 0)
			{
				aiStateWifi = WifiSwitch(102, WIFIAUS);
			}
		}
	}



	return aiStateWifi;
}


// RandomModus durchführen
void randomDigitalWrite(int laiOutRandom[], int anz)
{
	int i = 1;
	int ii = 0;
	int zl = 0;

	showQuit(laiOutRandom, 4);

	// printf("in 4.er Te Schaltung");

	while (i == 1)
	{

		time_t theTime = time(NULL);
		struct tm *aTime = localtime(&theTime);
		int hour = aTime->tm_hour;
		int min = aTime->tm_min;

		if ((hour >= 17 && hour <= 23) || (hour > 6 && hour < 8))
		{
			if (min > 1 && min < 13)
			{
				zl = 0;
			}
			if (min > 13 && min < 15)
			{
				zl = 99;
			}
			if (min > 15 && min < 28)
			{
				zl = 1;
			}
			if (min > 28 && min < 30)
			{
				zl = 99;
			}
			if (min > 30 && min < 43)
			{
				zl = 2;
			}
			if (min > 43 && min < 45)
			{
				zl = 99;
			}
			if (min > 45 && min < 57)
			{
				zl = 2;
			}
			if (min > 57 && min < 59)
			{
				zl = 99;
			}

			if ((zl >= 0) && (zl <= 3))
			{
				digitalWrite(laiOutRandom[zl], EIN);
			}
			if (zl == 99)
			{
				for (ii = 0; ii < 5; ii++)
				{
					digitalWrite(laiOutRandom[ii], AUS);
				}
			}
		}

		// Random-Modus beenden mit Flurtaster
		if (digitalRead(27) == 1 || digitalRead(28) == 1)
		{
			showQuit(laiOutRandom, 4);
			i = 0;
		}
		delay(1000);
	}
}

// Quittierung für Random-Modus durchführen
void showQuit(int laiOutRandom[], int zl)
{
	int i = 0;

	for (i = 0; i < 3; i++)
	{
		digitalWrite(laiOutRandom[i], EIN);
		delay(1000);

	}
	for (i = 0; i < 3; i++)
	{
		digitalWrite(laiOutRandom[i], AUS);
		delay(1000);
	}
}

// Wifi-Schalter betätigen Aufruf der Homepage des ESP SonOff S20
int WifiSwitch(int aiSchalternummer, int aiState)
{
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();

	if (curl)
	{
		if (aiSchalternummer == 101)
		{
			if (aiState == 1)
			{
				printf("Ausgang setzen %d \n", aiSchalternummer);
				curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.178.43/ein");
			}
			if (aiState == 0)
			{
				printf("Ausgang setzen %d \n", aiSchalternummer);
				curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.178.43/aus");
			}
		}
		if (aiSchalternummer == 102)
		{
			if (aiState == 1)
			{
				printf("Ausgang setzen %d \n", aiSchalternummer);
				curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.178.46/ein");
			}
			if (aiState == 0)
			{
				printf("Ausgang setzen %d \n", aiSchalternummer);
				curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.178.46/aus");
			}
		}
	}

	/* Perform the request, res will get the return code */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
	res = curl_easy_perform(curl);
	// printf("Ausgang setzen CHECK %d \n",aiSchalternummer);

	/* Check for errors */
	if (res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));

	/* always cleanup */
	curl_easy_cleanup(curl);

	return(aiState);
}

// Wifi-Schalter betätigen mit Systemaufruf mosquitto_pub
int WifiSwitchMosquitto(int aiSchalternummer, int aiState)
{

	return(aiState);
}

// Sonnenuntergang
float calculateSunset(int year, int month, int day, float lat, float lng, int localOffset, int daylightSavings) {
	//1. first calculate the day of the year
	float N1 = floor(275 * month / 9);
	float N2 = floor((month + 9) / 12);
	float N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
	float N = N1 - (N2 * N3) + day - 30;

	//2. convert the longitude to hour value and calculate an approximate time
	float lngHour = lng / 15.0;
	float t = N + ((18 - lngHour) / 24);   // if setting time is desired:

	//3. calculate the Sun's mean anomaly
	float M = (0.9856 * t) - 3.289;

	//4. calculate the Sun's true longitude
	float L = fmod(M + (1.916 * sin((PI / 180)*M)) + (0.020 * sin(2 * (PI / 180) * M)) + 282.634, 360.0);

	//5a. calculate the Sun's right ascension
	float RA = fmod(180 / PI * atan(0.91764 * tan((PI / 180)*L)), 360.0);

	//5b. right ascension value needs to be in the same quadrant as L
	float Lquadrant = floor(L / 90) * 90;
	float RAquadrant = floor(RA / 90) * 90;
	RA = RA + (Lquadrant - RAquadrant);

	//5c. right ascension value needs to be converted into hours
	RA = RA / 15;

	//6. calculate the Sun's declination
	float sinDec = 0.39782 * sin((PI / 180)*L);
	float cosDec = cos(asin(sinDec));

	//7a. calculate the Sun's local hour angle
	float cosH = (sin((PI / 180)*ZENITH) - (sinDec * sin((PI / 180)*lat))) / (cosDec * cos((PI / 180)*lat));
	/*
	if (cosH >  1)
	the sun never rises on this location (on the specified date)
	if (cosH < -1)
	the sun never sets on this location (on the specified date)
	*/

	//7b. finish calculating H and convert into hours
	float H = (180 / PI)*acos(cosH);    // if setting time is desired:
	H = H / 15;

	//8. calculate local mean time of rising/setting
	float T = H + RA - (0.06571 * t) - 6.622;

	//9. adjust back to UTC
	float UT = fmod(T - lngHour, 24.0);

	//10. convert UT value to local time zone of latitude/longitude
	return UT + localOffset + daylightSavings;
}

float calculateSunrise(int year, int month, int day, float lat, float lng, int localOffset, int daylightSavings) {
	//1. first calculate the day of the year
	float N1 = floor(275 * month / 9);
	float N2 = floor((month + 9) / 12);
	float N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
	float N = N1 - (N2 * N3) + day - 30;

	//2. convert the longitude to hour value and calculate an approximate time
	float lngHour = lng / 15.0;
	float t = N + ((6 - lngHour) / 24);   //if rising time is desired:

	//3. calculate the Sun's mean anomaly
	float M = (0.9856 * t) - 3.289;

	//4. calculate the Sun's true longitude
	float L = fmod(M + (1.916 * sin((PI / 180)*M)) + (0.020 * sin(2 * (PI / 180) * M)) + 282.634, 360.0);

	//5a. calculate the Sun's right ascension
	float RA = fmod(180 / PI * atan(0.91764 * tan((PI / 180)*L)), 360.0);

	//5b. right ascension value needs to be in the same quadrant as L
	float Lquadrant = floor(L / 90) * 90;
	float RAquadrant = floor(RA / 90) * 90;
	RA = RA + (Lquadrant - RAquadrant);

	//5c. right ascension value needs to be converted into hours
	RA = RA / 15;

	//6. calculate the Sun's declination
	float sinDec = 0.39782 * sin((PI / 180)*L);
	float cosDec = cos(asin(sinDec));

	//7a. calculate the Sun's local hour angle
	float cosH = (sin((PI / 180)*ZENITH) - (sinDec * sin((PI / 180)*lat))) / (cosDec * cos((PI / 180)*lat));
	/*
	if (cosH >  1)
	the sun never rises on this location (on the specified date)
	if (cosH < -1)
	the sun never sets on this location (on the specified date)
	*/

	//7b. finish calculating H and convert into hours
	float H = 360 - (180 / PI)*acos(cosH);   //   if if rising time is desired:
	H = H / 15;

	//8. calculate local mean time of rising/setting
	float T = H + RA - (0.06571 * t) - 6.622;

	//9. adjust back to UTC
	float UT = fmod(T - lngHour, 24.0);

	//10. convert UT value to local time zone of latitude/longitude
	return UT + localOffset + daylightSavings;
}

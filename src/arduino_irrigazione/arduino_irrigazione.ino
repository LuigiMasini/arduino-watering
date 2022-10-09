#include "RTClib.h"

#define A1 4
#define A2 5
#define VALVE_RESPONSE_DELAY 500


RTC_DS3231 rtc;
DateTime now;


Memory mem;

/*
Watering process

Start Water:
 * Alarm2 rings
 * set alarm 1
 * start water
 * clear alarm 2
 * write lastw
 * set alarm 2
 * update server

Stop water:
 * Alarm 1 rings
 * stop water
 * clear alarm1
 * update server

 */


void startWater() {

	DateTime Alarm2 = rtc.getAlarm(2);

	//set end of watering alarm
	rtc.setAlarm1( rtc.now() + TimeSpan( mem.getDuration()*60 ), DS3231_A1_Minute );

	//open valve
	digitalWrite(A1, HIGH);
	digitalWrite(A2, LOW);

	delay( VALVE_RESPONSE_DELAY );

	//reset
	digitalWrite(A1, LOW);
	digitalWrite(A2, LOW);


	rtc.clearAlarm(2);

	if ( mem.setLastWatering( rtc.now().unixtime() ) < 0){
		//TODO avvisa server che è finito spazio su eeprom
	}

	//schedule next watering
	rtc.setAlarm2( Alarm2 + TimeSpan( mem.getFrequency()*60*60 ), DS3231_A2_Hour );

	/*TODO update server:
	 * watering scheduled for [Alarm2]
	 * started at [mem.getLastWatering()]
	 * watering will stop @ [rtc.getAlarm(1)]
	 * next watering scheduled for [rtc.getAlarm(2)]
	 */
}

void stopWater() {

	//close valve
	digitalWrite(A1, LOW);
	digitalWrite(A2, HIGH);

	delay( VALVE_RESPONSE_DELAY );

	//reset
	digitalWrite(A1, LOW);
	digitalWrite(A2, LOW);

	rtc.clearAlarm(1);

	/*TODO update server:
	 * watering started at [mem.getLastWatering()]
	 * with stop scheduled for [rtc.getAlarm(1)]
	 * stopped at [rtc.now()]
	 */
}

void setup() {

	pinMode(A1, OUTPUT);
	pinMode(A2, OUTPUT);

	Serial.begin(9600);

	while ( !rtc.begin() ) delay(10);
	rtc.disable32K();
	rtc.writeSqwPinMode(DS3231_OFF);


	//TODO setup connessione con ESP


	//se RTC ha perso l'ora
	//chiedila a ESP, sincronizzato con ntp
	//e resetta RTC
	if ( rtc.lostPower() ){

		Serial.println("TIME");
		uint8_t i=0;  //aspetta max 5 sec
		while(!Serial.available() && i++<50) delay(100);

		if (i<=10){
			String epochTime = Serial.readString();
			rtc.adjust( DateTime ((uint32_t) epochTime.toDouble()) );  //converti String -> double -> uint32_t -> DateTime e setta l'ora su RTC
		}
	}


	DateTime A2Alarm = rtc.getAlarm(2);
	now = rtc.now();


	//WARNING unfinished
	if ( now >= A2Alarm ){

		startWater();

		unsigned skipped_n = (now.unixtime() - A2Alarm.unixtime()) / frequency*60*60

		uint32_t skipped_dates[skipped_n];

		uint8_t i=0;
		while (i < skipped_n){
			skipped_dates[i++] = A2Alarm.unixtime();

			A2Alarm = DateTime( A2Alarm + TimeSpan( frequency*60*60 ) );  // increment A2Alarm by 'frequency', which is the interval between waterings
		}



	}

}

void loop() {

	if  (rtc.alarmFired(1))
		stopWater();

	if (rtc.alarmFired(2))
		startWater();

	if (Serial.available()){

		//TODO handle status request
		//TODO handle save new params & respond
	}


	delay(1000);

}

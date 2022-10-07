#include "RTClib.h"

#define A1 4
#define A2 5


RTC_DS3231 rtc;
DateTime now;

void startWater() {
	digitalWrite(A1, HIGH);
	digitalWrite(A2, LOW);

	rtc.setAlarm1(rtc.now().unixtime()+duration*60);

	//TODO scrivi su eeprom last = rtc.now()
}

void setup() {

	pinMode(A1, OUTPUT);
	pinMode(A2, OUTPUT);

	Serial.begin(9600);

	while ( !rtc.begin() ) delay(10);


	//TODO setup connessione con ESP

	//TODO load params from EEPROM


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

	now = rtc.now();
	Serial.println(now.timestamp());

}

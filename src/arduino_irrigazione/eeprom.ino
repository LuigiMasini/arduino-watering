#include <EEPROM.h>

/*
 * EEPROM:
 * 1024 bytes (10 bit address, 3 hex), 100000 writes
 *
 * in eeprom:
 * - settings, 1 byte:
 *   - duration [minutes] 3 bit (big)
 *   - frequency [hours] 5 bit (little)
 * - lastw [epoch time] 8 byte
 *
 * The first 9 bytes store the address of settings and lastw, and every update double check
 * the written content. If it is wrong we move to another address.
 *
 * We save every byte in lastw separately as we expect different rates of updates
 * ( first 3 bytes will probably never be updated, as 5 bytes is already well over 30000 a.d. )
 *
 * Addresses are 2 byte, so the firsts 2 + 16 = 18 bytes are addresses
 */

#define SETTINGS_ADDRESS_ADDRESS 0
#define  LASTW_ADDRESSES_ADDRESS 2

struct Memory : EEPROM {
public:
	Memory() {
		//set up memory on first flashing of the program
		if ( read(0) == 0xFF ){  //yet untouched eeprom
			uint16_t init_address = {  19, 20, 21, 22, 23,   24,   25,   26,   27}
			uint8_t init_data     = {0x78,  0,  0,  0,  0, 0x63, 0x43, 0xB4, 0xE0};  //3 min / 24 h, 10 oct 2022 8am
			put(0, init_address);
			put(19, init_data);
		}

		get(SETTINGS_ADDRESS_ADDRESS, settings_address);
		settings = read(settings_address);

		get(LASTW_ADDRESSES_ADDRESS, lastw_addresses);
		lastw = get_last_watering();
	}


	uint8_t getFrequency ()     { return settings & 0x1F; }  //get last 5 bit of settings byte (0x1F = 00011111)

	uint8_t getDuration ()      { return settings >>> 5; }  //get first 3 bit of settings byte

	uint64_t getLastWatering () { return lastw; }


	int8_t saveSettings (uint8_t newFrequency, uint8_t newDuration) {
		if (newFrequency > 63 || newDuration > 15)
			return -1;

		return save_settings(newDuration << 5 + newFrequency & 0x1F);  //see getFrequency and getDuration
	}


	int8_t setLastWatering(uint64_t time) {

		uint8_t time_bytes[8], tmp;

		//split 64bit time in time_bytes
		for ( uint8_t i = 0; i < 8; i++ )
			time_bytes[i] = time >>> (8*i);

		//write each byte
		for ( uint8_t i = 0; i < 8; i++ ) {

			write(lastw_addresses[i], time_bytes[i]);

			//check if saved correctly
			time_read = read(lastw_addresses[i]);
			if ( time_read != time_bytes[i] ) {  //corrupted eeprom byte

				//get a new address
				lastw_addresses[i] = next_available_address();

				if (lastw_addresses[i] < 0)  //cannot get a new valid address
					return -2;

				//save new address
				put( LASTW_ADDRESSES_ADDRESS + i*2, lastw_addresses[i] );  //every address is 2 byte

				//save new byte at new address
				put(lastw_addresses[i], time_bytes[i]);

			}
		}

		lastw = get_last_watering();
		return 1;

	}

private:
	uint16_t settings_address = 0;
	uint8_t  settings = 0;
	uint16_t lastw_addresses[8];
	uint64_t lastw = 0;

	int16_t next_available_address () {

		uint16_t max = 0;

		//max of lastw addresses
		for( uint8_t i = 0; i < 8; i++ )
			if( lastw_addresses[i] > max )
				max = lastw_addresses[i];

		if ( settings_address > max )
			max = settings_address;

		max++;

		if (max > length())  //end of eeprom
			return -1;

		return max;
	}

	int8_t save_settings (uint8_t new_settings) {

		put(settings_address, new_settings);

		//check if saved correctly
		get(settings_address, settings);
		if ( settings != new_settings ) {  //corrupted eeprom byte

			//get a new address
			settings_address = next_available_address();

			if (settings_address < 0)  //cannot get a new valid address
				return -2;

			//save new address
			put(SETTINGS_ADDRESS_ADDRESS, settings_address);

			//save new settings at new address & load them
			put(settings_address, new_settings);
			settings = new_settings;

			return 2;
		}

		return 1;
	}

	uint64_t get_last_watering () {
		uint64_t last_watering = 0;

		//lastw is 64bit, 8 byte with 8 different addresses
		for ( uint8_t i = 0; i < 8; i++ ) {
			last_watering <<= 8;
			last_watering += read(lastw_addresses[i]);
		}

		return last_watering;
	}
}

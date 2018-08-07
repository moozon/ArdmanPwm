#include "EEPROM.h"
#include "MsTimer2.h"

#define PWM_PIN 9
#define TIMER1_RESOLUTION 65536UL  // Timer1 is 16 bit


unsigned long lastTime;
uint8_t counterSendData;
uint8_t timeOutSendData = 1;
uint8_t duty = 50;
uint8_t dutyReal = 0;
uint16_t dutyVal = 512;
uint32_t freq = 1000;
uint32_t freqReal = 0;
uint32_t freqVal = 0;
//String variables
String debugString = "|";
//EEPROM variables
uint8_t EEMEM dutyEE = 50;
uint32_t EEMEM freqEE = 1000;
uint8_t EEMEM logEE = 1;
uint8_t dutyAddrEeprom = 0;
uint8_t freqAddrEeprom = 1;
//Boolean variables
bool goodRecive = true;
bool isSendData = true;
bool isDebug = false;
bool isLogging = true;
//uint8_t isLogging = 1;
static boolean gen_mode; //флаг режима управления


void setup()
{
	Serial.begin(9600);
	while (!Serial) {}
	lastTime = millis();
	pinMode(PWM_PIN, OUTPUT);							// выход генератора
	initPwm(freq, duty);							//Инициализация PWM на Timer1(Режим Phase and Frequency Correct PWM) 
													//Чтение из EEPROM
													//EEPROM.get(dutyAddrEeprom, duty);
													//EEPROM.get(freqAddrEeprom, freq);	
	duty = eeprom_read_byte(&dutyEE);
	freq = eeprom_read_dword(&freqEE);
	isLogging = eeprom_read_byte(&logEE);

	if (freq>2848) gen_mode = 1; //переключение режима управления по OCR
	else gen_mode = 0; //переключение режима управления по частоте

					   //Сохранение настроек в EEPROM по таймеру
					   //MsTimer2::set(60000, flashEeprom); // 1 min period
					   //MsTimer2::start();
}

//Инициализация PWM на Timer1(Режим Phase and Frequency Correct PWM) 
void initPwm(uint32_t _freq, uint8_t _duty) {
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	TCCR1A = (1 << COM1A1);
	TCCR1B = (1 << WGM13) | (1 << CS10);
	setFreq(_freq);
	//setDuty(_duty);
	//ICR1 = 30000;
	//OCR1A = 15000;
}

//Методы для изменения частоты и шим
void upDownDuty(String mode) {
	if (mode == "up") {
		// если возможна регулировка с точностью  не хуже 1%, то:
		if (ICR1 > 100) {
			if (duty < 100) {
				duty++;
				setDuty();
			}
		}
		else {// если точность 1% уже невозможна, то будем крутить регистр OCR2A напрямую
			if (OCR1A < ICR1) {
				OCR1A++;
			}
			if (OCR1A > ICR1) {
				//OCR1A = (uint32_t)ICR1 / 2;
				OCR1A = ICR1 - 1;
			}
			duty = (uint32_t)100 * OCR1A / ICR1;
		}

	}
	if (mode == "down") {
		// если возможна регулировка с точностью  не хуже 1%, то:
		if (ICR1 > 100) {
			if (duty > 0) {
				duty--;
				setDuty();
			}
		}
		else {
			if (OCR1A > 1) {
				OCR1A--;
			}
			if (OCR1A > ICR1) {
				OCR1A = ICR1 - 1;
			}
			duty = (uint32_t)100 * OCR1A / ICR1;
		}

	}
}
void upDownFreq(String mode) {
	uint32_t icr = ICR1;
	uint16_t divider = 1; //переменная коэфф. деления прескалера

	if (mode == "up") {
		if (icr > 2) {
			icr--;
			//freq++;
			if (gen_mode) {
				ICR1 = icr;
				setDuty();
				freq = (uint32_t)F_CPU / 2 / ICR1;
				//freqReal = F_CPU / 2 / ICR1;
			}
			else { //расчёт прескалера и ICR по нужной частоте
				icr = (F_CPU / 2 / (freq + 1) / divider);
				byte shifts[] = { 3,3,2,2 };
				for (byte i = 0; i < 4; i++) {
					if (icr > 65536) {
						divider <<= shifts[i];
						icr = F_CPU / 2 / (freq + 1) / divider;
					}
					else {  //запись в регистр прескалера
						TCCR1B = (i + 1) | (1 << WGM13);
						break;
					}
				}
				//printDebug((String)icr);
				ICR1 = (icr - 1);
				setDuty();
				freq = (uint32_t)F_CPU / 2 / (ICR1 + 1) / divider;
				//freqReal = F_CPU / 2 / (ICR1 + 1) / divider;
			}
		}
	}
	//setFreq(freq + 1);
	if (mode == "down") {
		if (icr < 65535) {
			icr++;
			//freq--;
			if (gen_mode) {
				ICR1 = icr;
				setDuty();
				freq = (uint32_t)F_CPU / 2 / ICR1;
				//freqReal = F_CPU / 2 / ICR1;
			}
			else { //расчёт прескалера и ICR по нужной частоте
				icr = (F_CPU / 2 / (freq - 1) / divider);
				byte shifts[] = { 3,3,2,2 };
				for (byte i = 0; i < 4; i++) {
					if (icr > 65536) {
						divider <<= shifts[i];
						icr = F_CPU / 2 / (freq - 1) / divider;
					}
					else {  //запись в регистр прескалера
						TCCR1B = (i + 1) | (1 << WGM13);
						break;
					}
				}
				//printDebug((String)icr);
				//ICR1 = (icr - 1); 
				ICR1 = (icr - 1);
				setDuty();
				freq = (uint32_t)F_CPU / 2 / (ICR1 + 1) / divider;
				//freqReal = F_CPU / 2 / (ICR1 + 1) / divider;
			}
		}
	}

	//конец "если  частота менее 2848 герц)
	if (freq>2848)
		gen_mode = 1; //переключение режима управления по OCR
	else
		gen_mode = 0; //переключение режима управления по частоте
}
void setDuty() {
	OCR1A = (uint32_t)ICR1*duty / 100;
	//if (ICR1 < 100) {
	//	OCR1A = ICR1 / 2;
	//	duty = 50;
	//	return;
	//}
	//static uint16_t ocr;
	//ocr = (uint32_t)(ICR1 * duty / 100);
	//if (ocr < 1) { // если значение оср менее 1, то
	//	while (ocr<1) { // менять скважность пока оср не станет больше ноля
	//		(duty>50) ? duty-- : duty++;
	//		ocr = (uint32_t)ICR1 * duty / 100;
	//	}
	//}
	//OCR1A = ocr;
	//duty = (uint32_t)(OCR1A * 100) / ICR1;
}
void setDuty(uint16_t _duty) {
	duty = _duty;
	OCR1A = (uint32_t)ICR1 * duty / 100;
	//if (ICR1 < 100) {
	//	OCR1A = (uint32_t)ICR1 * _duty / 100;
	//	return;
	//}
	//static uint16_t ocr;
	//ocr = (uint32_t)ICR1 * _duty / 100;
	//if (ocr < 1) { // если значение оср менее 1, то
	//	while (ocr<1) { // менять скважность пока оср не станет больше ноля
	//		(duty>50) ? duty-- : duty++;
	//		ocr = (uint32_t)ICR1 * duty / 100;
	//	}
	//}
	//OCR1A = ocr;
	//duty = _duty;
}
void setFreq(uint32_t _freq) {
	unsigned short icr = 0;
	unsigned char csBits = 0;
	const unsigned long cycles = (F_CPU / 2000000) * (1000000 / _freq);
	if (cycles < TIMER1_RESOLUTION) {
		csBits = _BV(CS10);
		icr = cycles;
	}
	else if (cycles < TIMER1_RESOLUTION * 8) {
		csBits = _BV(CS11);
		icr = cycles / 8;
	}
	else if (cycles < TIMER1_RESOLUTION * 64) {
		csBits = _BV(CS11) | _BV(CS10);
		icr = cycles / 64;
	}
	else if (cycles < TIMER1_RESOLUTION * 256) {
		csBits = _BV(CS12);
		icr = cycles / 256;
	}
	else if (cycles < TIMER1_RESOLUTION * 1024) {
		csBits = _BV(CS12) | _BV(CS10);
		icr = cycles / 1024;
	}
	else {
		csBits = _BV(CS12) | _BV(CS10);
		icr = TIMER1_RESOLUTION - 1;
	}
	ICR1 = icr;
	TCCR1B = _BV(WGM13) | csBits;
	setDuty();
	//Вычисление реальной частоты	
	if (TCCR1B == 0b00010101) freq = F_CPU / 2 / ICR1 / 1024;		//7812
	if (TCCR1B == 0b00010100) freq = F_CPU / 2 / ICR1 / 256;		//31250
	if (TCCR1B == 0b00010011) freq = F_CPU / 2 / ICR1 / 64;			//125000
	if (TCCR1B == 0b00010010) freq = F_CPU / 2 / ICR1 / 8;			//1000000
	if (TCCR1B == 0b00010001) freq = F_CPU / 2 / ICR1;				//8000000	
}


//Перезапуск контроллера
void(*rebootAsm) (void) = 0;
void reboot() {
	Serial.end();
	rebootAsm();
	//wdt_disable();
	//wdt_enable(WDTO_1S);
	//while (true)
	//{
	//
	//}
}
//Запись в EEPROM по таймеру
void flashEeprom() {
	eeprom_update_byte(&dutyEE, duty);
	eeprom_update_dword(&freqEE, freq);
	eeprom_update_byte(&logEE, isLogging);

	//EEPROM.put(dutyAddrEeprom, duty);
	//EEPROM.put(freqAddrEeprom, freq);

	//EEPROM.update(dutyAddrEeprom, duty);
	//EEPROM.update(freqAddrEeprom, freq);
}
//Отправка системных данных
void sendDataToSerial() {
	//counterSendData++;

	//Вычисление реальной частоты	
	if (TCCR1B == 0b00010101) freqReal = F_CPU / 2 / ICR1 / 1024;		//7812
	if (TCCR1B == 0b00010100) freqReal = F_CPU / 2 / ICR1 / 256;		//31250
	if (TCCR1B == 0b00010011) freqReal = F_CPU / 2 / ICR1 / 64;			//125000
	if (TCCR1B == 0b00010010) freqReal = F_CPU / 2 / ICR1 / 8;			//1000000
	if (TCCR1B == 0b00010001) freqReal = F_CPU / 2 / ICR1;				//8000000	

																		//Вычисление реальной скважности
	dutyReal = (uint32_t)OCR1A * 100 / ICR1;

	//Отправка Частоты и ШИМ
	Serial.print('l');
	Serial.println(freq);
	Serial.println(duty);
	Serial.println(freqReal);
	Serial.println(dutyReal);

	//if (isDebug)
	//Serial.println(debugString);

	/*if (counterSendData == 10) {
	counterSendData = 0;
	}*/

	//dutyReal = OCR1A <<= ;
}
//Отправка Logs
void sendLogsToSerial() {
	//Вычисление реальной частоты	
	if (TCCR1B == 0b00010101) freqReal = F_CPU / 2 / ICR1 / 1024;	//7812
	if (TCCR1B == 0b00010100) freqReal = F_CPU / 2 / ICR1 / 256;	//31250
	if (TCCR1B == 0b00010011) freqReal = F_CPU / 2 / ICR1 / 64;		//125000
	if (TCCR1B == 0b00010010) freqReal = F_CPU / 2 / ICR1 / 8;		//1000000
	if (TCCR1B == 0b00010001) freqReal = F_CPU / 2 / ICR1;			//8000000	
																	//Serial.println(dutyReal);
	Serial.println(freqReal);
}
//Отправка Dubug
void sendDebugToSerial() {
	Serial.print('d');
	Serial.print("Debug: ");
	Serial.println(debugString);
}
//Принудительная отправка Debug в Serial
void printDebug(String s) {
	Serial.print('d');		//Отправка флага отладки	
	Serial.print("Debug: ");
	Serial.println(s);
	//if (isLogging) {
	//	isLogging = false;
	//	//delay(1000);
	//	Serial.println('d');	//Отправка флага отладки
	//	Serial.println(s);
	//	isLogging = true;
	//}
	//else{
	//	Serial.println('d');	//Отправка флага отладки
	//	Serial.println(s);
	//}
}
//Добавление отладочных данных в строку отладки
void addToDebug(String s) {
	debugString += " " + s + " |";
	//debugString = String(debugString + s);
}
void addToDebug(String s1, String s2) {
	debugString += " " + s1 + " | " + s2 + " |";
	//debugString = String(debugString + s);
}

void loop()
{
	//String incomingString;
	//String debugStringTemp;
	//uint8_t incomingByte;

	//Считывание данных из COM порта
	if (Serial.available() > 0) {
		uint8_t incomingByte;

		uint8_t value = Serial.read();		
		switch (value) {
		case '1':	//Duty up
			upDownDuty("up");
			//duty++;
			break;
		case '2':	//Duty down
			upDownDuty("down");
			//duty--;
			break;
		case '3':	//Freq up
			upDownFreq("up");
			//freq++;
			break;
		case '4':	//Freq down
			upDownFreq("down");
			//freq--;
			break;
		case 'd':	//Set duty
			duty = Serial.parseInt();
			Serial.read();  //!!!Чтение остаточного байта. Необходимо для корректного считывания
			setDuty(duty);
			break;
		case 'f':	//Set freq		
			freq = Serial.parseInt();
			Serial.read();  //!!!Чтение остаточного байта. Необходимо для корректного считывания
			setFreq(freq);
			break;
		case 'g':	//Debugging
			incomingByte = Serial.parseInt();
			if (incomingByte == 1) {
				isDebug = true;
				printDebug("Debug enabled");
			}
			else {
				isDebug = false;
				printDebug("Debug disabled");
			}
			break;
		case 'l':	//Logging
			incomingByte = Serial.parseInt();
			if (incomingByte == 1) {
				isLogging = true;
				printDebug("Logging enabled");
			}
			else {
				isLogging = false;
				printDebug("Logging disabled");
			}
			break;
		case 'r':	//Reboot board
			printDebug("Rebooting...");
			reboot();
			break;


			//Run commands
		case 'c':

			//Вариант со строками, Работает медленнее!!!
			/*incomingString = Serial.readString();
			if (incomingString == "reboot") {
			printDebug("Rebooting...");
			reboot();
			return;
			}else if (incomingString == "eeprom") {
			flashEeprom();
			printDebug("Flash to EEPROM");
			return;
			}
			else if (incomingString == "sleep") {
			printDebug("Sleeping...");
			return;
			}
			else if (incomingString == "test") {
			printDebug("Testing...");
			return;
			}*/
			char incomingChar[1];
			Serial.readBytes(incomingChar, 1);
			switch (incomingChar[0])
			{
			case 'r':	//Reboot
				printDebug("Rebooting...");
				reboot();
				break;
			case 'e':	//Flash to EEPROM
				flashEeprom();
				printDebug("Flash to EEPROM");
				break;
			case 's':	//Sleeping
				printDebug("Sleeping...");
				break;
			case 't':	//Logging timeout 
				timeOutSendData = Serial.parseInt();
				Serial.read();  //!!!Чтение остаточного байта. Необходимо для корректного считывания
				printDebug("Logging timeout set to: " + String(timeOutSendData) + " seconds");
				break;
			default:
				printDebug("Bad command");
				break;
			}
			break;
		default:
			//Если ошибки чтения из буфера Com порта
			goodRecive = false;
			//addToDebug("Error recive");
			break;
		}

	}

	//Отправка данных в COM порт через определенный промежуток времени
	if (millis() - lastTime >= (timeOutSendData * 1000)) {
		if (isLogging)
			sendDataToSerial();

		//Разделенние отправляемых данных в разные методы
		//Здесь и на хосте нужно определить триггер для разделения данных
		/*if (isLogging)
		sendLogsToSerial();
		*/
		if (isDebug)
			sendDebugToSerial();

		lastTime = millis();
	}
}
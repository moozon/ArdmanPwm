#include "TimerOne.h"
#include "EEPROM.h"
#include "MsTimer2.h"
#define PWM_PIN 9

uint32_t lastTime;
uint8_t counterSendData;
uint16_t duty = 50, dutyReal, dutyVal = 512;
uint32_t freq = 100, freqReal, freqVal;
uint8_t incomingByte;
String incomingString;
String test;
String debugString = "|";
uint8_t dutyEeprom EEMEM = 50;
uint32_t EEMEM freqEeprom = 1000;
uint8_t dutyAddrEeprom = 0;
uint8_t freqAddrEeprom = 3;
//String debugString2 = "";
//uint8_t debugInt = 123;
bool goodRecive = true;
bool isSendData = true;
bool isDebug = false;
bool isLogging = true;

void setup()
{
	Serial.begin(9600);
	while (!Serial) {}
	lastTime = millis();
	//pinMode(9, OUTPUT);							// выход генератора
	
	//Чтение из EEPROM
	//EEPROM.get(dutyAddrEeprom, duty);
	//EEPROM.get(freqAddrEeprom, freq);
	//if (duty > 100)
		//duty = 50;
	//if (freq > 1000000)
		//freq = 100;
	duty = eeprom_read_byte(&dutyEeprom);
	freq = eeprom_read_dword(&freqEeprom);


	Timer1.initialize(10000);
	Timer1.pwm(PWM_PIN, dutyVal);
	Timer1.start();
	//debugString = debugString + "123";

	//Init MsTimer2
	MsTimer2::set(6000, flashEeprom); // 1 min period
	MsTimer2::start();
}

//Запись в EEPROM по таймеру
void flashEeprom() {
	eeprom_update_byte(&dutyEeprom, duty);
	eeprom_update_dword(&freqEeprom, freq);
	//EEPROM.put(dutyAddrEeprom, duty);
	//EEPROM.put(freqAddrEeprom, freq);
	
	//EEPROM.update(dutyAddrEeprom, duty);
	//EEPROM.update(freqAddrEeprom, freq);
}

//Методы для изменения частоты и шим
void upDownDuty(String mode) {
	if (mode == "up")
		setDuty(duty + 1);
	if (mode == "down")
		setDuty(duty - 1);
}
void upDownFreq(String mode) {
	if (mode == "up")
		setFreq(freq + 1);
	if (mode == "down")
		setFreq(freq - 1);
}
void setDuty(uint16_t _duty) {
	dutyVal = map(_duty, 0, 100, 0, 1023);
	//duty = _duty;
	Timer1.setPwmDuty(PWM_PIN, dutyVal);
}
void setFreq(uint32_t _freq) {
	//freq = _freq;
	Timer1.setPeriod(_freq);
	Timer1.setPwmDuty(PWM_PIN, dutyVal);
}

//Перезапуск контроллера
void(*rebootAsm) (void) = 0;
void reboot() {
	Serial.end();
	//vol.noTone();
	rebootAsm();
	//wdt_disable();
	//wdt_enable(WDTO_1S);
	//while (true)
	//{
	//
	//}
}

//Отправка системных данных
void sendDataToSerial() {
	//counterSendData++;

	//Вычисление реальной частоты	
	if (TCCR1B == 0b00010101) freqReal = F_CPU / 2 / ICR1 / 1024;	//7812
	if (TCCR1B == 0b00010100) freqReal = F_CPU / 2 / ICR1 / 256;	//31250
	if (TCCR1B == 0b00010011) freqReal = F_CPU / 2 / ICR1 / 64;		//125000
	if (TCCR1B == 0b00010010) freqReal = F_CPU / 2 / ICR1 / 8;		//1000000
	if (TCCR1B == 0b00010001) freqReal = F_CPU / 2 / ICR1;			//8000000	
	
	//Отправка Частоты и ШИМ
	Serial.println('l');
	Serial.println(duty);
	Serial.println(freq);
	Serial.println(dutyReal);
	Serial.println(freqReal);
	Serial.println(test);
	if (isDebug)
		Serial.println(debugString);

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
	Serial.println(dutyReal);
	Serial.println(freqReal);
	Serial.println(test);
}
//Отправка Dubug
void sendDebugToSerial() {
	Serial.println(debugString);
}
//Отправка отладочных данных в Serial
void printDebug(String s){
	if (isLogging) {
		isLogging = false;
		//delay(1000);
		Serial.println('d');	//Отправка флага отладки
		Serial.println(s);
		isLogging = true;
	}
	else{
		Serial.println('d');	//Отправка флага отладки
		Serial.println(s);
	}
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
	String debugStringTemp;
	uint8_t debugIntTemp;
	
	//Считывание данных из COM порта
	if (Serial.available() > 0) {
		uint8_t value = Serial.read();
		switch (value) {
		case '1':	//Duty up
			upDownDuty("up");
			debugStringTemp = Serial.readString();
			//debugString += String("222");
			addToDebug(debugStringTemp);
			//debugString = debugString + "321";
			//setDuty(duty + 1);
			//EncDuty("up");
			//Serial.println("up duty");
			duty++;
			break;
		case '2':	//Duty down
			upDownDuty("down");
			debugStringTemp = Serial.readString();
			addToDebug(debugStringTemp, "111");
			//EncDuty("down");
			//Serial.println("down duty");
			duty--;
			break;
		case '3':	//Freq up
			upDownFreq("up");
			//UpDownFreq("up");
			//EncFreq("up");
			//Serial.println("up freq");
			freq++;
			break;
		case '4':	//Freq down
			upDownFreq("down");
			//UpDownFreq("down");
			//EncFreq("down");
			//Serial.println("down freq");
			freq--;
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
				//debugString = String(debugString + (String)incomingByte);
				addToDebug("Debug turn ON");
			}
			else {
				isDebug = false;
				//debugString = String(debugString + "asd");
				//test = "qweewq";
				//StringSumHelper()
				addToDebug("Debug turn OFF");
			}
			break;
		case 'l':	//Logging
			incomingByte = Serial.parseInt();
			if (incomingByte == 1) {
				isLogging = true;
			}
			else {
				isLogging = false;
			}
			break;
		case 'r':	//Reboot board
			reboot();
			break;
		case 'c':	//Run commands
			char c[1];
			Serial.readBytes(c, 1);
			switch (c[0])
			{
			case 'r':	//Reboot
				reboot();
				break;
			default:
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
	if ((millis() - lastTime) >= 1000) {
		if (isLogging)
			sendDataToSerial();

		//Разделенние отправляемых данных в разные методы
		//Здесь и на хосте нужно определить триггер для разделения данных
		/*if (isLogging)
		sendLogsToSerial();
		if (isDebug)
		sendDebugToSerial();*/

		lastTime = millis();
	}
}
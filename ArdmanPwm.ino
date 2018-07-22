#define PWM_PIN 9

uint32_t lastTime;
uint8_t counterSendData;
uint8_t logging = 1;
uint16_t duty = 512, dutyReal, dutyVal = 512;
uint32_t freq = 100, freqReal, freqVal;
uint8_t incomingByte;
String test;
String incomingString;
char debugStrin[] = "Debug: ";
String debugString = "Debug: ";
bool isSendData = true;
bool goodRecive = true;
bool isDebug = false;


void setup()
{
	Serial.begin(9600);
	lastTime = millis();
	pinMode(9, OUTPUT);							// выход генератора

}

//Методы для изменения частоты и шим
void UpDownDuty(String mode) {
	if (mode == "up")
		setDuty(duty + 1);
	if (mode == "down")
		setDuty(duty - 1);
}
void UpDownFreq(String mode) {
	if (mode == "up")
		setFreq(freq + 1);
	if (mode == "down")
		setFreq(freq - 1);
}
void setDuty(uint16_t _duty) {
	duty = _duty;
	//vol.tone(PWM_PIN, freq, duty);
}
void setFreq(uint32_t _freq) {
	freq = _freq;
	//vol.tone(PWM_PIN, freq, duty);
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

// Метод для запуска по таймеру
void SendDataToSerial() {
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


	//unsigned long dutyCycle
	//dutyReal = OCR1A <<= ;
}

//Добавление отладочных данных в строку отладки
void addToDebug(const char s[]) {
	debugString = debugString + s;
	//debugString = String(debugString + s);
}

void loop()
{
	//Считывание данных из COM порта
	if (Serial.available() > 0) {		//TODO: »зменить условие на > 0
		uint8_t value = Serial.read();
		switch (value) {
		case '1':
			UpDownDuty("up");
			//setDuty(duty + 1);
			//EncDuty("up");
			//Serial.println("up duty");
			duty++;
			break;
		case '2':
			UpDownDuty("down");
			//EncDuty("down");
			//Serial.println("down duty");
			duty--;
			break;
		case '3':
			UpDownFreq("up");
			//UpDownFreq("up");
			//EncFreq("up");
			//Serial.println("up freq");
			freq++;
			break;
		case '4':
			UpDownFreq("down");
			//UpDownFreq("down");
			//EncFreq("down");
			//Serial.println("down freq");
			freq--;
			break;
		case 'd':
			duty = Serial.parseInt();
			Serial.read();  //!!!Чтение остаточного байта. Необходимо для корректного считывания
			setDuty(duty);
			break;
		case 'f':
			//ParseInt recive method			
			freq = Serial.parseInt();
			Serial.read();  //!!!Чтение остаточного байта. Необходимо для корректного считывания
			setFreq(freq);
			break;
		case 'g':
			incomingByte = Serial.parseInt();
			if (incomingByte == 1) {
				isDebug = true;
				//debugString = String(debugString + (String)incomingByte);
				//addToDebug("Debug turn ON");
			}
			else {
				isDebug = false;

				//debugString = String(debugString + "asd");
				test = "qweewq";
				//StringSumHelper()
				//addToDebug("Debug turn OFF");
			}

			break;
		case 'l':
			incomingByte = Serial.parseInt();
			//incomingByte = Serial.read();
			if (incomingByte == 1) {
				logging = 1;
			}
			else {
				logging = 0;
			}
			break;
		case 'r':
			reboot();
			break;
		case 'c':
			char c[1];
			Serial.readBytes(c, 1);
			switch (c[0])
			{
			case 'r':
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
		//TIMER1_RESOLUTION
		if (logging) {
			SendDataToSerial();
			lastTime = millis();
		}
	}
}
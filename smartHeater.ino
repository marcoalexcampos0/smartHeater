
  #define STATUS_LED_PIN  5
  #define RELE_PWR_PIN    4
  #define RELE_LOW_PIN    3
  #define RELE_HIGH_PIN   2
  #define BUTTON_UP_PIN   8
  #define BUTTON_DOWN_PIN 9

  const int speedCursor = 10;             // Speed for the rolling cursor (in ms)
    
  int debug = 2;                          // 0: GRAPH, 1: ONLY ERRORS, 2: NORMAL, 3: DETAILED
  float setTemp = 22.0;
  float riseTemp = 0.0;
  float fallTemp = 0.0;
  unsigned long raiseTime = 0;
  unsigned long fallTime = 0;
  boolean buttonUpStatus = false;
  boolean buttonDownStatus = false;
  boolean heaterStatus = false;
  unsigned long last_millis = 4294967290;
  unsigned long network_time = 0;
  unsigned long heaterOnTime = 0;
  unsigned long heaterOffTime = 0;
  unsigned long heaterLastChange = 0;
  int heaterOnPercent = 0;
  
// Usadas pelo sensor DS18B20
  #include <OneWire.h>                    // https://github.com/PaulStoffregen/OneWire
  #include <DallasTemperature.h>          // https://github.com/milesburton/Arduino-Temperature-Control-Library
  // Armazena temperaturas minima e maxima
  // Define uma instancia do oneWire para comunicacao com o sensor
  #define ONE_WIRE_BUS 7
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
  const short int numeroMaxSensoresDS18B20 = 2;
  DeviceAddress sensorsDS18B20[numeroMaxSensoresDS18B20];
  int numeroSensoresDS18B20;
  float DS18B20_temp[numeroMaxSensoresDS18B20] = {};

  #include <EEPROM.h>
  int EEPROMaddress = 0;
  const int EEPROMaddressMax = 199;

void setup() {

  // Incia LED de status do equipamento
  pinMode(STATUS_LED_PIN,OUTPUT);
  pinMode(BUTTON_UP_PIN,INPUT);
  pinMode(RELE_PWR_PIN,OUTPUT);

  // Inicia porta série
  digitalWrite(STATUS_LED_PIN,LOW);
  Serial.begin(9600);
  digitalWrite(STATUS_LED_PIN,HIGH);

  if (!debug) { Serial.println("currentTemp setTemp heaterStatus heaterOnTime"); }

  if (debug > 0) {
    Serial.println();
    Serial.println(F("*********************************************"));
    Serial.println(F("* 2020 smartHeater™ - basic (0.0.1)         *"));
    Serial.println(F("* by Marco Campos <mcampos@marcocampos.com> *"));
    Serial.println(F("*********************************************"));
    Serial.println();
  }

  Serial.print(getTime()); Serial.println(F(": INFO - To calibrate, press both buttons at start.")); Serial.println();

  // Read from EEPROM the setTemp value
  if (debug>1) { Serial.print(getTime()); Serial.print(F(": INFO - Stored setTemp = ")); }
  while (EEPROM.read(EEPROMaddress+1)!=0 && EEPROMaddress < EEPROMaddressMax) {
    EEPROMaddress += 1;
  }

  if (EEPROMaddress != EEPROMaddressMax) {
    setTemp = EEPROM.read(EEPROMaddress); 
  } else {
      EEPROMaddress = 0;
      EEPROM.update(EEPROMaddress, int(setTemp));
      EEPROM.update(EEPROMaddress+1, 0);
  }
  if (debug>1) { Serial.print(setTemp); Serial.print("ºC (#"); Serial.print(EEPROMaddress); Serial.println(")"); }
  if (setTemp>30 || setTemp<10) { setTemp=22.00; }

  // Inicializa sensores DS18B20
  digitalWrite(STATUS_LED_PIN,LOW);
  if (debug >1 ) { Serial.print(getTime()); Serial.println(F(": Initializing [DS18B20] sensors...")); }
  sensors.begin();
  
  // Localiza e mostra enderecos dos sensores
  numeroSensoresDS18B20 = sensors.getDeviceCount();
  if (debug > 1) { Serial.print(getTime()); Serial.print(F(": INFO - ")); Serial.print(numeroSensoresDS18B20, DEC); Serial.println(F(" sensor(s) found.")); }
  
  if ( numeroMaxSensoresDS18B20 < numeroSensoresDS18B20) {
    if (debug > 1) { Serial.print(getTime()); Serial.print(F(": WARNING - Maximum of ")); Serial.print(numeroMaxSensoresDS18B20); Serial.print(F(" sensors allowed.")); }
    numeroSensoresDS18B20 = numeroMaxSensoresDS18B20;
  }

  while (!sensors.getAddress(sensorsDS18B20[0], 0)) {
    if (debug > 0) { Serial.print(getTime()); Serial.println(F(": ERROR - No sensors found! Retry in 5s...")); }
    digitalWrite(STATUS_LED_PIN,HIGH);
    delay(5000);
    digitalWrite(STATUS_LED_PIN,LOW);
  }

    // Mostra o endereco do sensor encontrado no barramento
    if (debug > 1) { Serial.print(getTime()); Serial.print(F(": INFO - Sensor address(es): ")); }
    for (int i = 0; i < numeroSensoresDS18B20; i++) {
      sensors.getAddress(sensorsDS18B20[i], i);
      if (debug > 1) { mostra_endereco_sensor(sensorsDS18B20[i]); }
      if (i<(numeroSensoresDS18B20-1)) {
        if (debug > 1) { Serial.print(", "); }
      } else {
        Serial.println();
      }
    }

  // If both buttons are pressed during the initialization, it will start the callibration:
  //
  if (digitalRead(BUTTON_UP_PIN) & digitalRead(BUTTON_DOWN_PIN)) {
    calibrate();
  }

  Serial.println();
  if (debug > 1) {
    Serial.println(F("------------------------"));
    Serial.println(F("Initialization complete."));
    Serial.println(F("------------------------"));
    Serial.println();
  }
  digitalWrite(STATUS_LED_PIN,HIGH);
 
  (1, false); 

}

void mostra_endereco_sensor(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // Adiciona zeros se necessário
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

//String getTime()
//
unsigned long getTime()
{
  return millis();

//   unsigned long t = millis();
//
//  // Quando inicializa (millis ~= 0), ou quando o contador "dá a volta" (50 dias), vai buscar a hora certa:
//  if ( t <= last_millis ) {
//    //network_time = get_network_time();
//    last_millis = t;
//  } else {
//    last_millis = t;
//  }
//
//  unsigned long seg = ((network_time * 1000) + t) / 1000;
//
//  // Calcula os dias, horas, minutos e segundos com base nos millisegundos do uptime:
//  unsigned int dias = seg / 86400; unsigned long resto_dos_dias = seg % 86400;
//  byte horas = resto_dos_dias / 3600; unsigned long resto_das_horas = resto_dos_dias % 3600;
//  byte minutos = resto_das_horas / 60; unsigned long resto_dos_minutos = resto_das_horas % 60;
//  byte segundos = resto_dos_minutos;
//
//  //String time = String(dias) + ":" + String(horas) + ":" + String(minutos) + ":" + String(segundos);
//  String time = dias + ":";
//  time += horas + ":";
//  time += minutos + ":";
//  time += segundos;
//  return time;
  
}

void storeSetTemp (float value) {
  if (EEPROMaddress < EEPROMaddressMax) {
    EEPROMaddress += 1;
  } else {
    EEPROMaddress = 0;
  }
  byte temp = int(value);
  EEPROM.update(EEPROMaddress, temp);
  EEPROM.update(EEPROMaddress+1, 0);  
  if (debug > 1) {Serial.print(getTime()); Serial.print(F(": INFO - Storing setTemp value of ")); Serial.print(temp); Serial.print(F(" on EEPROM address #")); Serial.print(EEPROMaddress); Serial.println("."); } 
}

void checkButton () {

  if (digitalRead(BUTTON_UP_PIN) & !buttonUpStatus) {
    if (setTemp<30) {
      setTemp = setTemp+0.5;
      if (debug > 1) { Serial.println(" "); Serial.print(getTime()); Serial.print(F(": INFO - Set temperature to: ")); Serial.print(setTemp); Serial.print("ºC"); }
      storeSetTemp(setTemp);
      buttonUpStatus = true;
      checkTemp();
    }
  } else {
    if (!digitalRead(BUTTON_UP_PIN) & buttonUpStatus) {
      buttonUpStatus = false;
    }
  }
  
  if (digitalRead(BUTTON_DOWN_PIN) & setTemp>10 & !buttonDownStatus) {
    setTemp = setTemp-0.5;
    if (debug > 1) { Serial.println(" "); Serial.print(getTime()); Serial.print(F(": INFO - Set temperature to: ")); Serial.print(setTemp); Serial.print("ºC"); }
    storeSetTemp(setTemp);
    buttonDownStatus = true;
    checkTemp();
  } else {
    if (!digitalRead(BUTTON_DOWN_PIN) & buttonDownStatus) {
      buttonDownStatus = false;
    }
  }

}

void pause (unsigned long int tempo, boolean tipo, boolean button) {
  // Tempo: Número de segundos para a 
  // Tipo: TRUE-> com output "...", FALSE sem output
  // button: check for button press or not
  
      char rolex[4] = { '|', '/', '-', '\\' };
      unsigned long int t = tempo * 1000;
      unsigned long int t1 = millis();
      unsigned long int t2 = t1;
    
      if ( debug > 2 ) { if ( tipo ) { Serial.print(getTime()); Serial.print(F(": INFO - Pause for ")); Serial.print(tempo); Serial.print(F(" seconds... ")); } }
      
      while ( t2 - t1 < t )
      {
        for (int b=0; b<4; b++ ) 
        {
          
          digitalWrite(STATUS_LED_PIN,LOW);
          for (int j=0; j<25; j++)
          {
            if (button) { checkButton(); }
            delay(speedCursor);
          }
          
          digitalWrite(STATUS_LED_PIN,HIGH);
          for (int j=0; j<25; j++)
          {
            if (button) { checkButton(); }
            delay(speedCursor);
          }
          
          t2 = millis();
          if ( debug > 2 ) { if ( tipo ) { Serial.print(rolex[b]); Serial.write(8); } }
        }
      }
      if ( debug > 2 ) { Serial.println(" "); }
}

void lerDS18B20() {
// Estado do sensor DS18B20 (temperatura digital):
  sensors.requestTemperatures();

  for (int i=0; i<numeroSensoresDS18B20; i++) {
      DS18B20_temp[i] = sensors.getTempC(sensorsDS18B20[i]);

    if ( DS18B20_temp[i] == -127 ) {
      if ( debug > 0) { Serial.print(getTime()); Serial.print(F(": ERROR: Error reding sensor [DS18B20]: ")); mostra_endereco_sensor(sensorsDS18B20[i]); Serial.println(); }
    } else {
      // Mostra dados na consola
      //if ( debug > 1 ) { Serial.print(getTime()); Serial.print(F(": INFO: Temperatura (DS18B20): ")); Serial.print(DS18B20_temp[i]); Serial.print(F("ºC : ")); mostra_endereco_sensor(sensorsDS18B20[i]); Serial.println(); }

      // Converte um float em array de 5 caracteres e o nome da sonda (TEMP0 ou TEMP1)
      char DS18B20_ID_char[5];
      char DS18B20_temp_char[5];
      String a = "TEMP" + String(i+1);
      a.toCharArray(DS18B20_ID_char, 6);
      String(DS18B20_temp[i],2).toCharArray(DS18B20_temp_char, 6);

    }
  }
}

void calibrate() {

  if (debug > 1) { Serial.print(getTime()); Serial.println(F(": WARNING - Calibration process started!")); }
  lerDS18B20();
  float tempStart = DS18B20_temp[0];
  unsigned long t = millis();
  setHeater(true);
  if (debug > 1) { Serial.print(getTime()); Serial.println(F(": INFO - Heating for up to 5 min., or the temperature raises 0,2ºC... ")); }
  raiseTime = millis() - t;
  while (riseTemp < 0.2 && raiseTime < 300000) {
    lerDS18B20();
    riseTemp = DS18B20_temp[0]-tempStart;
    raiseTime = millis() - t;
    Serial.print(riseTemp); Serial.print("º  "); Serial.write(13); //Serial.write(8); Serial.write(8); Serial.write(8); Serial.write(8);
    delay(500);
  }
  if (raiseTime >= 300000) {
    if (debug > 0) { Serial.print(getTime()); Serial.println(F(": ERROR - Timout waiting for the temperature to raise 0,2ºC")); }
  } else {
    if (debug > 1) { Serial.print(getTime()); Serial.print(F(": INFO - raiseTime = ")); Serial.print(raiseTime/1000); Serial.println(F(" seconds")); }
  }

  lerDS18B20();
  tempStart = DS18B20_temp[0];
  t = millis();
  setHeater(false);
  if (debug > 1) { Serial.print(getTime()); Serial.println(F(": INFO - Cooling for up to 5 min., or the temperature falls 0,2ºC... ")); }
  fallTime = millis() - t;
  while (fallTemp < 0.2 && fallTime < 300000) {
    lerDS18B20();
    fallTemp = tempStart-DS18B20_temp[0];
    fallTime = millis() - t;
    Serial.print(fallTemp); Serial.print("º  "); Serial.write(13); //Serial.write(8); Serial.write(8); Serial.write(8); Serial.write(8);
    delay(500);
  }
  if (fallTime >= 300000) {
    if (debug > 0) { Serial.print(getTime()); Serial.println(F(": ERROR - Timout waiting for the temperature to fall 0,2ºC")); }
  } else {
    if (debug > 1) { Serial.print(getTime()); Serial.print(F(": INFO - fallTime = ")); Serial.print(fallTime/1000); Serial.println(F(" seconds")); }
  }

}


void setHeater (boolean state) {
  
  if (state) {
    if (heaterStatus) {
      heaterOnTime +=  millis() - heaterLastChange; heaterLastChange = millis(); 
    } else {
      if (debug > 1) { Serial.print(getTime()); Serial.println(": WARNING - Setting heater ON.."); }
      digitalWrite(RELE_PWR_PIN,HIGH);
      heaterStatus = true;
      heaterOffTime +=  millis() - heaterLastChange;
      heaterLastChange = millis(); 
    }
      
   } else {
     if (!heaterStatus) { 
      heaterOffTime +=  millis() - heaterLastChange; heaterLastChange = millis(); 
    } else {
       if (debug > 1) { Serial.print(getTime()); Serial.println(": WARNING - Setting heater OFF.."); }
       digitalWrite(RELE_PWR_PIN,LOW);
       heaterStatus = false;
       heaterOnTime +=  millis() - heaterLastChange;
       heaterLastChange = millis(); 
     }
   }

  heaterOnPercent = int (100.00 * heaterOnTime / millis());

}

void displayInfo () {

  if (debug > 1) { Serial.print(getTime()); Serial.print(": INFO - Current temperature: "); Serial.print(DS18B20_temp[0]); Serial.print("ºC (setTemp="); Serial.print(setTemp); Serial.println("ºC)");}
  
  if (!debug) {
    Serial.print(DS18B20_temp[0]); Serial.print(" ");
    Serial.print(setTemp); Serial.print(" ");
    if (heaterStatus) {
      Serial.print("20 ");
    } else {
      Serial.print("18 ");
    }
    Serial.print(heaterOnPercent); Serial.println("%");
  }
}

void checkTemp () {
  
  digitalWrite(STATUS_LED_PIN,LOW);
  lerDS18B20();                                                 // Lê a temperatura em todos as sondas

  if (DS18B20_temp[0] < (setTemp+fallTemp)) {                      // Compara a temperatura lida com o valor definido
    setHeater(true);                                            // e liga ou desliga consoante a necessidade
   } else {
    setHeater(false);
   }
  
  digitalWrite(STATUS_LED_PIN,HIGH);

}

void loop() {

  checkTemp();
  displayInfo();
  pause(10, true, true);

}

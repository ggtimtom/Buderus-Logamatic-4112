#include "esphome.h"

class UartReadLineSensor : public Component, public UARTDevice, public TextSensor {
 public:
  UartReadLineSensor(UARTComponent *parent) : UARTDevice(parent) {}

  
  Sensor *SolarKollektor_temperature_sensor = new Sensor();
  Sensor *Solarspeicher_temperature_sensor = new Sensor();
  Sensor *WarmwasserIst_temperature_sensor = new Sensor();
  Sensor *Aussentemperatur_temperature_sensor = new Sensor();
  Sensor *GedAussentemperatur_temperature_sensor = new Sensor();
  Sensor *AnlagenvorlaufSoll_temperature_sensor = new Sensor();
  Sensor *AnlagenvorlaufIst_temperature_sensor = new Sensor();
  
  void setup() override {
    // nothing to do here
  }

  int readline(int readch, char *buffer, int len)
  {
    static int pos = 0;
    int rpos;
    int endflag;

    if (1 == 1) {
      switch (readch) {

        case 0x82: // new line
          if (pos > 1 && buffer[pos-1]== 0xAF)
          {
            rpos = pos;
            pos = 0;  // Reset position index ready for next time
            return rpos;
          }
         case 0x02: // new line
          if (pos > 1 && buffer[pos-1]== 0xAF)
          {
            rpos = pos;
            pos = 0;  // Reset position index ready for next time
            return rpos;
          }
  
        default:
          if (pos < len-1) {
            buffer[pos] = readch;
            pos++;
            buffer[pos] = 0;
          }
          else {
            pos = 0;
          }
      }
    }
    // No end of line has been found, so return -1.
    return -1;
  }

  void loop() override {
    const int max_line_length = 26;
    static char buffer[max_line_length];
    static char buff0[10];
    static char buff1[10];
    static char buff2[10];
    static char buff3[10];
    static char buff4[10];
    static char buff5[10];
    static char buff6[10];
    static char buff7[10];
    static char buff8[10];
    static char buff9[10];
    static char buff10[10];
    static char buff[300];
    
    

    while (available()) {
      if(readline(read(), buffer, max_line_length) > 0) {
      //der fertige buffer
        if (1 == 1){
          sprintf(buff0, " %d ", (int)buffer[0]);
          sprintf(buff1, " %d ", (int)buffer[1]);
          sprintf(buff2, " %d ", (int)buffer[2]);
          sprintf(buff3, " %d ", (int)buffer[3]);
          sprintf(buff4, " %d ", (int)buffer[4]);
          sprintf(buff5, " %d ", (int)buffer[5]);
          sprintf(buff6, " %d ", (int)buffer[6]);
          sprintf(buff7, " %d ", (int)buffer[7]);
          sprintf(buff8, " %d ", (int)buffer[8]);
          sprintf(buff9, " %d ", (int)buffer[9]);
          sprintf(buff10, " %d ", (int)buffer[10]);
          sprintf(buff, "%s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s", buff0, buff1, buff2, buff3, buff4, buff5, buff6, buff7, buff8, buff9, buff10);
          
          //Warmwasser
          if(buffer[0] == 132 && buffer[1] == 0){
            WarmwasserIst_temperature_sensor->publish_state(buffer[5]);
          }
          if(buffer[0] == 132 && buffer[1] == 6){

          }
          
          //Kessel
          if(buffer[0] == 0x85 && buffer[1] ==0){
            AnlagenvorlaufSoll_temperature_sensor->publish_state(buffer[2]);
            AnlagenvorlaufIst_temperature_sensor->publish_state(buffer[3]);
          
          }
          
          //Solar
          if(buffer[0] == 0x9E && buffer[1] == 24){
            SolarKollektor_temperature_sensor->publish_state(buffer[7]);
          }
          if(buffer[0] == 0x9E && buffer[1] == 6){
            Solarspeicher_temperature_sensor->publish_state(buffer[2]);
          }
          
          //Konfiguration
          if(buffer[0] == 0x89 && buffer[1] == 0){
            Aussentemperatur_temperature_sensor->publish_state(buffer[2]);
            GedAussentemperatur_temperature_sensor->publish_state(buffer[3]);
          }
          
          publish_state(buff);
          if (buffer[0] == 0x88){
            
          }
        }
        

      }
    }
  }

};

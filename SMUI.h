
#ifndef SMUI_h
#define SMUI_h

#include "Arduino.h"
#include <stdio.h>
#include <string.h>
#include <PJsonstore.h>

String smui_logdata;
int smui_loglines=0;
unsigned char smui_max_logline=50;

pjsonstore configdef;
pjsonstore config;

class smui_logger{
  public: 
  
  logger(){}
  logger(unsigned char maxlines){
    smui_max_logline=maxlines;
  }
  
  void _logger(String s) {

   if (smui_loglines>smui_max_logline) {
      smui_logdata.remove(0, smui_logdata.indexOf("\n") + 1);
   }
   
   if (Serial) Serial.println(s);
   
   smui_logdata+=String(millis());
   smui_logdata+=": ";
   smui_logdata+=s;
   smui_logdata+="<br />\n";
   smui_loglines++;
  }

  void log(String c){
   _logger(c);
  }

  void log(char * c){
   _logger(String(c));
  }

  void log(char b){
   _logger(String(b));
  }

  void log(int b){
   _logger(String(b));
  }

  void log(unsigned int b){
   _logger(String(b));
  }

  void log(long b){
   _logger(String(b));
  }

  void log(unsigned long b){
   _logger(String(b));
  }
  
  void log(float b){
   _logger(String(b));
  }  

  void log(double b){
   _logger(String(b));
  }  
  
};

class smui{

    public: smui(){
    
    }
    
    begin(){
        
    }
    
    String _getespflashconf() {
    
      char tmp[200];
      String flashconf="";

      //kesobbi updatehez az infok
      uint32_t realSize = ESP.getFlashChipRealSize();
      uint32_t ideSize = ESP.getFlashChipSize();
      FlashMode_t ideMode = ESP.getFlashChipMode();

      sprintf(tmp, "Flash real id:   %08X<br />", ESP.getFlashChipId());
      flashconf += tmp;
      sprintf(tmp, "Flash real size: %u<br /><br />", realSize);
      flashconf += tmp;

      sprintf(tmp, "Flash ide size: %u<br />", ideSize);
      flashconf += tmp;
      sprintf(tmp, "Flash ide speed: %u<br />", ESP.getFlashChipSpeed());
      flashconf += tmp;
      sprintf(tmp, "Flash ide mode:  %s<br />", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
      flashconf += tmp;
      if (ideSize != realSize) {
        sprintf(tmp, "Chip conf wrong!<br />");
        flashconf += tmp;
      } else {
        sprintf(tmp, "Chip conf ok.<br />");
        flashconf += tmp;
      }
    
      return flashconf;
    }
}

#endif /* SMUI_h */

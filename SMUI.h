#ifndef SMUI_h
#define SMUI_h

#include "Arduino.h"
#include <stdio.h>
#include <string.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <PJsonstore.h>
#include "FS.h"

ESP8266WebServer smui_httpServer(80);
ESP8266HTTPUpdateServer smui_httpUpdater;

String smui_logdata;
int smui_loglines=0;
unsigned char smui_max_logline=50;

pjsonstore smui_config;

class smui_logger{
  public: 
  
  smui_logger(){}
  void setloglines(unsigned char maxlines){
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
  
  void log(const char * c){
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

smui_logger smui_log;

class smui{

    public: 
    smui(){
    
    }
    
    void begin(String &configdef){
        Serial.begin(115200);
        smui_log.log("Serial init.");
        
        if (!SPIFFS.begin()) {
    
            smui_log.log("Failed to mount file system. Fatal Error!");
            return;
        }
        
        loadConfiguration("/config.json",configdef,smui_config);
        
        ArduinoOTA.onStart([]() {
            smui_log.log("OTA onStart");
        });
        ArduinoOTA.onEnd([]() {
            smui_log.log("OTA onEnd");
        });
          
        ArduinoOTA.onError([](ota_error_t error) {
            smui_log.log("OTA ERROR: code:");
            smui_log.log(error);
            if (error == OTA_AUTH_ERROR) smui_log.log("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) smui_log.log("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) smui_log.log("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) smui_log.log("Receive Failed");
            else if (error == OTA_END_ERROR) smui_log.log("End Failed");
        });
        
        ArduinoOTA.begin();
        
        smui_httpUpdater.setup(&smui_httpServer);
        
        smui_httpServer.on("/", this->mainfunc);
        smui_httpServer.on("/ajax", this->mainfuncajax);
        smui_httpServer.on("/setup", this->setupfunc);
        smui_httpServer.on("/setup/", this->setupfunc);
        smui_httpServer.on("/setup/ajax", this->setupfuncajax);
       
        smui_httpServer.begin();
    }
    
    String _getespflashconf() {
    
      char tmp[200];
      String flashconf="";

      //kesobbi updatehez az infok
      uint32_t realSize = ESP.getFlashChipRealSize();
      uint32_t ideSize = ESP.getFlashChipSize();
      FlashMode_t ideMode = ESP.getFlashChipMode();

      sprintf(tmp, "Flash real id:   %08X\n", ESP.getFlashChipId());
      flashconf += tmp;
      sprintf(tmp, "Flash real size: %u\n", realSize);
      flashconf += tmp;

      sprintf(tmp, "Flash ide size: %u\n", ideSize);
      flashconf += tmp;
      sprintf(tmp, "Flash ide speed: %u\n", ESP.getFlashChipSpeed());
      flashconf += tmp;
      sprintf(tmp, "Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
      flashconf += tmp;
      if (ideSize != realSize) {
        sprintf(tmp, "Chip conf wrong!\n");
        flashconf += tmp;
      } else {
        sprintf(tmp, "Chip conf ok.\n");
        flashconf += tmp;
      }
    
      return flashconf;
    }
    
    String specch(String s) {
      String tmp;

      tmp = s;

      tmp.replace(">", "&gt;");
      tmp.replace("<", "&lt;");
      tmp.replace("\"", "&quot;");
      tmp.replace("&", "&amp;");

      return tmp;
    }
    
    void redirtopage(String s) {
      smui_httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      smui_httpServer.sendHeader("Pragma", "no-cache");
      smui_httpServer.sendHeader("Expires", "0");
      smui_httpServer.sendHeader("Location", s);

      smui_httpServer.send(302);
    }
    
    void loadConfiguration(const char *filename,String &configdefstr,pjsonstore &dest) {

      smui_log.log("loadConfiguration start...");
      smui_log.log(filename);
      
      pjsonstore configdef;

      if (configdef.from_json(64, configdefstr.c_str())) {
        smui_log.log("Config definition parsed!");
      } else {
        smui_log.log("Error parsing config definition!");
      }

      if (!SPIFFS.exists("exists")) {
        
        smui_log.log("Formatting filesystem...");
        SPIFFS.format();
        File f = SPIFFS.open("exists", "w");
        f.print("exists");
        f.close();

      }

      File file = SPIFFS.open(filename, "r");
      if (!file) {
        smui_log.log("Failed to open config file, using defaults.");

        if (configdef.get_size() > 0) {
          for (int i = 0; i < configdef.get_size(); i++) {
            dest[configdef.get_by_num(i).get_key()] = configdef.get_by_num(i)[0];
          }
         
          smui_log.log("Default config:");
          String st;
          dest.to_json(st);
          smui_log.log(st);

        }

      } else {
        
        String s = file.readString();
        if (dest.from_json(64, s.c_str())) {
          
          smui_log.log("Json config read successfully!");
          smui_log.log(s);
   
          if (configdef.get_size() > 0) {
            for (int i = 0; i < configdef.get_size(); i++) {

              if (dest.has_key(configdef.get_by_num(i).get_key())) {

                if (dest[configdef.get_by_num(i).get_key()].get_type()!=configdef.get_by_num(i)[0].get_type()) {
                  dest[configdef.get_by_num(i).get_key()] = configdef.get_by_num(i)[0];
                  
                }
              } else {
               
                dest[configdef.get_by_num(i).get_key()] = configdef.get_by_num(i)[0];
              
              }

            }
    
            smui_log.log("Config:");
            String st;
            dest.to_json(st);
            
            smui_log.log(st);

          }

        } else {
          
          smui_log.log("Error parsing json config, deleteing it...");
          file.close();
          SPIFFS.remove(filename);
          return;
        }
      }

      file.close();
  
    }
    
    void saveConfiguration(const char *filename, pjsonstore &source) {
      
      smui_log.log("Saving config:");  
      smui_log.log(filename);

      if (SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
      }

      File file = SPIFFS.open(filename, "w");
      if (!file) {
        smui_log.log("Failed to save config file.");
      }

      String s;
      source.to_json(s);
      
      smui_log.log("Writing config to file:");
    
      if (file.print(s)){ 
        smui_log.log("File print ok.");
      }else{
        smui_log.log("File print error.");  
      }
        
      file.close();

    }
    
    String softredir(String s) {

      String s1;
      s1 = "<script>window.location.href='" + s + "';</script>";
      return s1;

    }
    
    static String templ(unsigned char ajax, String sb) {

      smui_httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      smui_httpServer.sendHeader("Pragma", "no-cache");
      smui_httpServer.sendHeader("Expires", "0");

      String s = "";

      if (ajax == 1) {
        s += sb;
      } else {
        s += "<!DOCTYPE HTML>\r\n<html><head><script>";

        s += String("") + R"=(
          var r=1;
          function ajf() {
            var x=new XMLHttpRequest(); 
            x.timeout = 3000; 
            var u=window.location.href;
            u+=(u.substr(u.length-1)=="/"?"ajax":"/ajax");
            x.open('GET',u,true);
            x.onload =  function() {
              if (x.status===200) {
                document.getElementById('d').innerHTML=x.responseText; 
                if (r) setTimeout(ajf,1500);
              } else {
                if (confirm('Bad response, retry refresh?')) {} else { r=0; } 
              }
            }        
            x.ontimeout=function(e) {
                if (r) setTimeout(ajf,1500);
            }
            x.send(null);
          };

          function isi(e,u){
            var c = (e.which) ? e.which : e.keyCode
            if (c==8 || c==37) return;
            e.target.value=e.target.value.replace(/[^0-9]/g,"");
            if (e.target.value>u) e.target.value=u;
          }
          
          ajf();
          
            )=";

        s += "</script></head><body><div id=\"d\">\r\n";
        s += "</div><div id=\"s\">\r\n";
        s += sb;
        s += "</div></body></html>\n";
      }
      return s;
    }
    
    static void mainfunc() {
        String s = "main static content";
        smui_httpServer.send(200, "text/html", templ(0, s));
    }
    
    static void mainfuncajax() {
        String s = "main dynamic content";
        smui_httpServer.send(200, "text/html", templ(1, s));
    }
    
    static void setupfunc() {
        String s = "<a href=\"/\">main</a><br /><br /><a href=\"/update\">FW Update!</a><br /><a href=\"/setup?reset=1\">Reset!</a><br /><a href=\"/setup?factoryreset=1\">Factory Reset!</a>";
        smui_httpServer.send(200, "text/html", templ(0, s));
    }
    
    static void setupfuncajax() {
        String s = "setup dynamic content";
        smui_httpServer.send(200, "text/html", templ(1, s));
    }

    void loop(){
       ArduinoOTA.handle();
       smui_httpServer.handleClient();   
    }
    
};

#endif /* SMUI_h */

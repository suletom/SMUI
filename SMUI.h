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

/*https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html*/

#define SMUI_CONFIG_TOKEN 96

ESP8266WebServer smui_httpServer(80);
ESP8266HTTPUpdateServer smui_httpUpdater;

File fsUploadFile; 

String smui_logdata;
int smui_loglines=0;
unsigned char smui_max_logline=50;
unsigned char smui_restartflag=0;

pjsonstore smui_config;

class smui_logger{
  public: 
  
  smui_logger(){}
  
  void getlog(unsigned long from){
    
  }
  
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
   smui_logdata+="\n";
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
        
        initwifi();
        
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
        smui_httpServer.on("/config",HTTP_POST,this->conffunc);
       
        smui_httpServer.begin();
        
    }
    
        
    static String specch(String s) {
      String tmp;

      tmp = s;

      tmp.replace(">", "&gt;");
      tmp.replace("<", "&lt;");
      tmp.replace("\"", "&quot;");
      tmp.replace("&", "&amp;");

      return tmp;
    }
    
    static void redirtopage(String s) {
      smui_httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      smui_httpServer.sendHeader("Pragma", "no-cache");
      smui_httpServer.sendHeader("Expires", "0");
      smui_httpServer.sendHeader("Location", s);

      smui_httpServer.send(302);
    }
    
    static void loadConfiguration(const char *filename,String &configdefstr,pjsonstore &dest) {

      smui_log.log("loadConfiguration start...");
      smui_log.log(filename);
      
      pjsonstore configdef;

      if (configdef.from_json(SMUI_CONFIG_TOKEN, configdefstr.c_str())) {
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
        if (dest.from_json(SMUI_CONFIG_TOKEN, s.c_str())) {
          
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
    
    static void saveConfiguration(const char *filename, pjsonstore &source) {
      
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
    
    static String softredir(String s) {

      String s1;
      s1 = "<script>window.location.href='" + s + "';</script>";
      return s1;

    }
    
    
    static void refresh(int msec) { 

        String s="setTimeout(function(){ document.body.innerHTML =  \"reloading...\"; },1000); setTimeout(function(){ window.location.href=\"/\"; },"+String(msec)+");";
        smui_httpServer.send(200, "text/html", templ(3, s));

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

        if (ajax==0) {
            s += String("") + R"=(
              var r=1;
              function ajf() {
                var x=new XMLHttpRequest(); 
                x.timeout = 3000; 
                var u=window.location.href;
                u+=(u.substr(u.length-1)=="/"?"ajax":"/ajax");
                x.open("GET",u,true);
                x.onload =  function() {
                  if (x.status===200) {
                    let jd=null;
                    try {
                        jd=JSON.parse(x.responseText);
                    }catch(e){  }
                    if (jd!==null && jd.text!=undefined){
                        document.getElementById("d").innerHTML=jd.text;
                        document.getElementById("l").innerHTML=jd.log;
                    }else{
                        document.getElementById("d").innerHTML=x.responseText; 
                    }    
                    if (r) setTimeout(ajf,1500);
                  } else {
                    if (confirm("Bad response, retry refresh?")) {} else { r=0; } 
                  }
                }        
                x.ontimeout=function(e) {
                    if (r) setTimeout(ajf,1500);
                }
                x.send(null);
              };

              function postfile(u,fn,d){
                var xhr = new XMLHttpRequest();
                xhr.open("POST", u, true);
                var boundary = '---------------------------7da24f2e50046';
                xhr.setRequestHeader('Content-Type', 'multipart/form-data; boundary=" + boundary);
                var body = "" +     
                '--' + boundary + '\r\n' + 
                'Content-Disposition: form-data; name="file[]"; filename="' + fn + '"' + '\r\n' + 
                'Content-Type: text/plain' + '\r\n' + 
                '' + '\r\n' + 
                d + '\r\n' + 
                '--' + boundary + '--'; 
                xhr.onreadystatechange = function() {
                  if (xhr.readyState == XMLHttpRequest.DONE) {
                      alert(xhr.responseText);
                  }
                }
                xhr.send(body);
              }

              function isi(e,u){
                var c = (e.which) ? e.which : e.keyCode
                if (c==8 || c==37) return;
                e.target.value=e.target.value.replace(/[^0-9]/g,"");
                if (e.target.value>u) e.target.value=u;
              }

              function cf(){
                if (confirm("Sure?")){
                  return true;
                }
                return false;
              }
              
              ajf();
              
                )=";
        }else{
            s += sb;
        }

        s += "</script></head><body>";
        s += R"=(
        <ul id="m">
          <li><a href="/">Main dashboard</a></li>
        </ul>
        <div id="d">
        </div>
        <div id="s">)=";

        if (ajax<2){
            s += sb;
        }    

        s += "</div><div id=\"l\"></div></body></html>\n";
      }
      return s;
    }
  
   static void conffunc() {

        HTTPUpload& upload = smui_httpServer.upload();
        if(upload.status == UPLOAD_FILE_START){
          String filename = upload.filename;
          if(!filename.startsWith("/")) filename = "/"+filename;
          
          fsUploadFile = SPIFFS.open(filename, "w");
          filename = String();
        } else if(upload.status == UPLOAD_FILE_WRITE){
          if(fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);
        } else if(upload.status == UPLOAD_FILE_END){
          if(fsUploadFile) {
            fsUploadFile.close();
            
            smui_httpServer.send(200,"text/plain", "Upload OK");
          } else {
            smui_httpServer.send(500, "text/plain", "500: couldn't create file");
          }
        }
                
        return;

    }

    static void mainfunc() {
    
        int i=0;
          
        if (  smui_httpServer.hasArg("reset") ) {
            i = smui_httpServer.arg("reset").toInt();
            
            if (i==1) {
              smui_restartflag=1;
            }
            
            refresh(12000);
            return;
        }
        
        if (  smui_httpServer.hasArg("factoryreset") ) {
            i = smui_httpServer.arg("factoryreset").toInt();
            if (i==1){
              smui_config.unset();
              saveConfiguration("/config.json",smui_config);
              
              smui_log.log("Factory reset: Empty Config! Reboot....");
              smui_restartflag=1;

              refresh(12000);
              return;
            }
        }
    
        
        String s="<pre>";

        char tmp[200];
        //kesobbi updatehez az infok
        uint32_t realSize = ESP.getFlashChipRealSize();
        uint32_t ideSize = ESP.getFlashChipSize();
        FlashMode_t ideMode = ESP.getFlashChipMode();

        sprintf(tmp, "Flash real id:   %08X\n", ESP.getFlashChipId());
        s += tmp;
        sprintf(tmp, "Flash real size: %u\n", realSize);
        s += tmp;

        sprintf(tmp, "Flash ide size: %u\n", ideSize);
        s += tmp;
        sprintf(tmp, "Flash ide speed: %u\n", ESP.getFlashChipSpeed());
        s += tmp;
        sprintf(tmp, "Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
        s += tmp;
        
        
        s += "</pre>";

        s += R"=(   
        <form method="POST" action="/update" enctype="multipart/form-data">
         Firmware:<br>
         <input type="file" accept=".bin,.bin.gz" name="firmware" />
         <a href="javascript:void(0);" onclick="cf()?this.closest('form').submit():void(0);">Update Firmware</a>
        </form>
        <a href="/?backup=1";">Backup firmware!</a><br />
        <a href="javascript:void(0);" onclick="postfile('/config',document.getElementById('c').value);">Apply config!</a><br />
        
        <a href="/?reset=1" onclick="return cf();">Reset!</a><br />
        <a href="/?factoryreset=1" onclick="cf();">Factory Reset!</a>)=";

        s+="<textarea id=\"c\">";
        String ct;
        smui_config.to_json(ct);
        s+=ct;
        s+="</textarea>";
        s+="<pre>"+smui_logdata+"</pre>";

        smui_httpServer.send(200, "text/html", templ(0, s));
        
    }
    
    static void mainfuncajax() {
    
        String s = "main dynamic content";
        smui_httpServer.send(200, "text/html", templ(1, s));
    }
    
    void loop(){
       ArduinoOTA.handle();
       smui_httpServer.handleClient();   
       if (smui_restartflag==1){
            smui_restartflag=0;

            //ujra, mert nem zarja le a kapcsolatot elotte
            unsigned long t=millis();
 
            smui_log.log("Waiting to flush contents...");
            while(millis()-t<1000){
              smui_httpServer.handleClient();
            }
            smui_log.log("Reset...");
            ESP.restart();
            return;
       }
    }
    
    void initwifi(){

      smui_log.log("INIT Wifi: WIFI_AP_STA");
      WiFi.mode(WIFI_AP_STA);

      uint8_t mac[WL_MAC_ADDR_LENGTH];
      WiFi.softAPmacAddress(mac);
      String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                     String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
      macID.toUpperCase();
      String AP_NameString = String(smui_config["smui_ssid"].get_value().pj_string) + macID;

      char AP_NameChar[AP_NameString.length() + 1];
      memset(AP_NameChar, 0, AP_NameString.length() + 1);

      for (int i=0; i<AP_NameString.length(); i++)
        AP_NameChar[i] = AP_NameString.charAt(i);

      smui_log.log("Creating AP:");
      smui_log.log(AP_NameChar);
      smui_log.log(smui_config["smui_wifipw"].get_value().pj_string);
      
      WiFi.softAP(AP_NameChar,String(smui_config["smui_wifipw"].get_value().pj_string).c_str() );

      if (smui_config.has_key("smui_ap_to_connect")){
          if (String(smui_config["smui_ap_to_connect"].get_value().pj_string).length()>0){
              smui_log.log("Connecting to AP:");
              
              smui_log.log(smui_config["smui_ap_to_connect"].get_value().pj_string);
              WiFi.begin(smui_config["smui_ap_to_connect"].get_value().pj_string, smui_config["smui_ap_pw"].get_value().pj_string);
          }
      }
      
    }
    
};

#endif /* SMUI_h */

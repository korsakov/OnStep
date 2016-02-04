#if defined(__TM4C1294NCPDT__) || defined(__TM4C1294XNCZAD__) || defined(W5100_ON)

int  www_xmit_buffer_send_pos=0;
int  www_xmit_buffer_pos=0;
char www_xmit_buffer[1024] = "";

// provide the same functions as for serial

// Enter a MAC address and IP address for your controller below. MAC address will be on the sticker on the Tiva Connected Launchpad.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 55);
IPAddress myDns(192,168,1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


// Network server port for command channel
EthernetServer cmd_server(9999);
EthernetClient cmd_client;
// Network server port for web channel
EthernetServer web_server(80);
EthernetClient www_client;
boolean alreadyConnected = false; // whether or not the client was connected previously


void Ethernet_Init() {
  // initialize the ethernet device
#if defined(ETHERNET_USE_DHCP_ON)
  Ethernet.begin(mac);
#else
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
#endif

  cmd_server.begin();
  web_server.begin();
  www_client = web_server.available(); // initialise www_client
}

void Ethernet_send(const char data[]) {
  if (!cmd_client)
   return;
   
//  cmd_client.flush();
   
  cmd_client.print(data);
}

void Ethernet_print(const char data[]) {
  if (!cmd_client)
   return;
   
   cmd_server.print(data);
 }
 
 boolean Ethernet_transmit() {
   return false;
 }
 
 boolean Ethernet_available() {
   cmd_client = cmd_server.available();
   return cmd_client;
 }
 
 char Ethernet_read() {
   if (!cmd_client)
   return -1;
   
   return cmd_client.read();
 }

// -----------------------------------------------------------------------------------------
// web server

boolean currentLineIsBlank;
boolean responseStarted = false;
boolean clientNeedsToClose = false;
unsigned long responseFinish_ms;
unsigned long transactionStart_ms;
int html_page_step=0;

bool get_check=false;
bool get_name=false;
bool get_val=false;
int get_idx=0;
char get_names[11] = "";
char get_vals[11] = "";

// variables to support web-page request detection
const char index_page[] = "GET /index.htm"; bool index_page_found; byte index_page_count;
const char settings_page[] = "GET /settings.htm"; bool settings_page_found; byte settings_page_count;
const char control_page[] = "GET /control.htm"; bool control_page_found; byte control_page_count;
const char pec_page[] = "GET /pec.htm"; bool pec_page_found; byte pec_page_count;
const char config_page[] = "GET /config.htm"; bool config_page_found; byte config_page_count;
void reset_page_requests() {
  index_page_found=false; index_page_count=0;
  settings_page_found=false; settings_page_count=0;
  control_page_found=false; control_page_count=0;
  pec_page_found=false; pec_page_count=0;
  config_page_found=false; config_page_count=0;
  get_check=false; get_val=false; get_name=false;
}

void Ethernet_www() {
  // if a client doesn't already exist try to find a new one
  if (!www_client) { www_client = web_server.available(); currentLineIsBlank = true; responseStarted=false; clientNeedsToClose=false; reset_page_requests(); transactionStart_ms=millis(); }

  // active client?
  if (www_client) {
    if (www_client.connected()) {
      if (www_client.available()) {
        char c = www_client.read();

        //Serial_char(c);

        // record the get request name and value, then process the request
        if ((get_val) && ((c==' ') || (c=='&'))) { get_val=false; get_idx=0; Ethernet_get(); if (c=='&') { c='?'; get_check=true; } }
        if (get_val) { if (get_idx<10) { get_vals[get_idx]=c; get_vals[get_idx+1]=0; get_idx++; } }
        if ((get_name) && (c=='=')) { get_name=false; get_val=true; get_idx=0; }
        if (get_name) { if (get_idx<10) { get_names[get_idx]=c; get_names[get_idx+1]=0; get_idx++; } }
        if ((get_check) && (c=='?')) { get_name=true; get_idx=0; } get_check=false;
        
        // watch for page requests and flag them
        if (!index_page_found)    { if (c==index_page[index_page_count])       index_page_count++;    else index_page_count=0;    if (index_page_count==14)    { index_page_found=true; get_check=true; } }
        if (!settings_page_found) { if (c==settings_page[settings_page_count]) settings_page_count++; else settings_page_count=0; if (settings_page_count==17) { settings_page_found=true; get_check=true; } }
        if (!control_page_found)  { if (c==control_page[control_page_count])   control_page_count++;  else control_page_count=0;  if (control_page_count==16)  { control_page_found=true; get_check=true; } }
        if (!pec_page_found)      { if (c==pec_page[pec_page_count])           pec_page_count++;      else pec_page_count=0;      if (pec_page_count==12)      { pec_page_found=true; get_check=true; } }
        if (!config_page_found)   { if (c==config_page[config_page_count])     config_page_count++;   else config_page_count=0;   if (config_page_count==15)   { config_page_found=true; get_check=true; } }
        
        // if you've gotten to the end of the line (received a newline character) and the line is blank, the http request has ended, so you can send a reply
        if ((c == '\n') && currentLineIsBlank) { 
          responseStarted=true;
          html_page_step=0;
        } else {
          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          } else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }
      }

      // any data ready to send gets processed here
      if (responseStarted) {
        if (www_xmit_buffer_pos==0) {
          if (index_page_found) index_html_page(); else
          if (settings_page_found) settings_html_page(); else
          if (control_page_found) control_html_page(); else
          if (pec_page_found) pec_html_page(); else
          if (config_page_found) config_html_page(); else
          index_html_page();
        }
      }

      if (www_xmit_buffer_pos>0) {
        if (!www_send()) {
          clientNeedsToClose=true;
          responseFinish_ms=millis();
        } else {
          clientNeedsToClose=false;
        }
      }

    }
  
    // if data was sent give the web browser time to receive it then stop the client
    // if a transaction is taking more than five seconds, stop the client
    if ((clientNeedsToClose && (millis()-responseFinish_ms>100)) || (millis()-transactionStart_ms>5000)) {
      clientNeedsToClose=false;
      www_client.stop();
    }
    Ethernet.maintain();

  }
}

// quickly copies string to www server transmit buffer
// returns true if successful
// returns false if buffer is too full to accept the data (and copies no data into buffer)
boolean www_write(const char data[]) {
  int l=strlen(data);
  if (www_xmit_buffer_pos+l>1022) return false;
  strcpy((char *)&www_xmit_buffer[www_xmit_buffer_pos],data);
  www_xmit_buffer_pos+=l;
  return true;
}

// quickly writes up to 7 chars at a time from buffer to ethernet adapter
// returns true if data is still waiting for transmit
// returns false if data buffer is empty
boolean www_send() {
  char buf[11] = "";
  char c;
  
  // copy some data
  boolean buffer_empty=false;
  int count=0;
  for (int l=0; l<9; l++) {
    c=www_xmit_buffer[www_xmit_buffer_send_pos];
    buf[l+1]=0;
    buf[l]=c; 
    if ((c==0) || (www_xmit_buffer_send_pos>1022)) { buffer_empty=true; break; }
    www_xmit_buffer_send_pos++; count++;
  }

  www_client.write(buf,count);
  // send network data
//  www_client.write(buf,);
 // www.client.

  // hit end of www_xmit_buffer? reset and start over
  if (buffer_empty) {
    www_xmit_buffer_pos=0;
    www_xmit_buffer_send_pos=0;
    www_xmit_buffer[0]=0;
    return false;
  }

  return true;
}

// The index.htm page --------------------------------------------------------------------------------------
// process GET requests 
// currently only the name and value are handled, the page sending the name isn't considered
// this lowers the processing overhead and is sufficient for our purposes here
double get_temp_float;
int get_temp_day,get_temp_month,get_temp_year;
int get_temp_hour,get_temp_minute,get_temp_second;
void Ethernet_get() {
  if (get_names[2]!=0) return; // only two char names for now

  // from the Settings.htm page -------------------------------------------------------------------
  // Slew Speed
  if ((get_names[0]=='s') && (get_names[1]=='s')) {
    if ((get_vals[0]=='v') && (get_vals[1]=='s') && (get_vals[2]==0)) maxRate=MaxRate*32L;
    if ((get_vals[0]=='s') && (get_vals[1]==0)) maxRate=MaxRate*24L;
    if ((get_vals[0]=='n') && (get_vals[1]==0)) maxRate=MaxRate*16L;
    if ((get_vals[0]=='f') && (get_vals[1]==0)) maxRate=MaxRate*11L;
    if ((get_vals[0]=='v') && (get_vals[1]=='f') && (get_vals[2]==0)) maxRate=MaxRate*8L;
    
    if (maxRate<(MaxRate/2L)*16L) maxRate=(MaxRate/2L)*16L;
    if (maxRate>(MaxRate*2L)*16L) maxRate=(MaxRate*2L)*16L;
    EEPROM_writeInt(EE_maxRate,(int)(maxRate/16L));
  }
  // Overhead and Horizon Limits
  if ((get_names[0]=='o') && (get_names[1]=='l')) {
    if ( (atoi2(get_vals,&i)) && ((i>=60) && (i<=90))) { 
      maxAlt=i; 
#ifdef MOUNT_TYPE_ALTAZM
      if (maxAlt>87) maxAlt=87;
#endif
      EEPROM.update(EE_maxAlt,maxAlt);
    }
  }
  if ((get_names[0]=='h') && (get_names[1]=='l')) {
    if ( (atoi2(get_vals,&i)) && ((i>=-30) && (i<=30))) { minAlt=i; EEPROM.update(EE_minAlt,minAlt+128); }
  }
  // Backlash Limits
  if (get_names[0]=='b') {
    if ( (atoi2(get_vals,&i)) && ((i>=0) && (i<=999))) { 
      if (get_names[1]=='2') {
        backlashAxis2=(int)round(((double)i*(double)StepsPerDegreeAxis2)/3600.0);
        EEPROM_writeInt(EE_backlashAxis2,backlashAxis2);
      } else
      if (get_names[1]=='1') {
        backlashAxis1 =(int)round(((double)i*(double)StepsPerDegreeAxis1)/3600.0);
        EEPROM_writeInt(EE_backlashAxis1,backlashAxis1);
      }
    }
  }
  // Location
  if ((get_names[0]=='g') && (get_names[1]=='1')) {
    if ( (atoi2(get_vals,&i)) && ((i>=-180) && (i<=180))) { get_temp_float=i; }
  }
  if ((get_names[0]=='g') && (get_names[1]=='2')) {
    if ( (atoi2(get_vals,&i)) && ((i>=0) && (i<=60))) { if (get_temp_float<0.0) i=-i; longitude=get_temp_float+((double)i)/60.0; EEPROM_writeFloat(EE_sites+(currentSite)*25+4,longitude); update_lst(jd2last(JD,UT1)); }
  }
  if ((get_names[0]=='t') && (get_names[1]=='1')) {
    if ( (atoi2(get_vals,&i)) && ((i>=-180) && (i<=180))) { get_temp_float=i; }
  }
  if ((get_names[0]=='t') && (get_names[1]=='2')) {
    if ( (atoi2(get_vals,&i)) && ((i>=0) && (i<=60))) {
      if (get_temp_float<0.0) i=-i;
      latitude=get_temp_float+((double)i)/60.0;
      EEPROM_writeFloat(100+(currentSite)*25+0,latitude);
#ifdef MOUNT_TYPE_ALTAZM
      celestialPoleDec=fabs(latitude);
      if (latitude<0) celestialPoleHA=180L; else celestialPoleHA=0L;
#else
      if (latitude<0) celestialPoleDec=-90.0; else celestialPoleDec=90.0;
#endif
      cosLat=cos(latitude/Rad);
      sinLat=sin(latitude/Rad);
      if (latitude>0.0) HADir = HADirNCPInit; else HADir = HADirSCPInit;
    }
  }
  if ((get_names[0]=='u') && (get_names[1]=='1')) {
    if ( (atoi2(get_vals,&i)) && ((i>=-13) && (i<=13))) { get_temp_float=i; }
  }
  if ((get_names[0]=='u') && (get_names[1]=='2')) {
    if ( (atoi2(get_vals,&i)) && ((i==00) || (i==30) || (i==45))) {
      if (get_temp_float<0.0) i=-i;
      timeZone=get_temp_float+((double)i)/60.0;
      b=encodeTimeZone(timeZone)+128;
      EEPROM.update(EE_sites+(currentSite)*25+8,b);
      UT1=LMT+timeZone;
      UT1_start  =UT1;
      UT1mS_start=millis();
      update_lst(jd2last(JD,UT1));
    }
  }
  // from the Control.htm page -------------------------------------------------------------------
  // Date/Time
  if ((get_names[0]=='d') && (get_names[1]=='m')) {
    if ( (atoi2(get_vals,&i)) && ((i>=0) && (i<=11))) { get_temp_month=i+1; }
  }
  if ((get_names[0]=='d') && (get_names[1]=='d')) {
    if ( (atoi2(get_vals,&i)) && ((i>=1) && (i<=31))) { get_temp_day=i; }
  }
  if ((get_names[0]=='d') && (get_names[1]=='y')) {
    if ( (atoi2(get_vals,&i)) && ((i>=2016) && (i<=9999))) {
      get_temp_year=i-2000;
      char temp[10];
      sprintf(temp,"%02d/%02d/%02d",get_temp_month,get_temp_day,get_temp_year);
      if (dateToDouble(&JD,temp)) {
        EEPROM_writeFloat(EE_JD,JD); 
        update_lst(jd2last(JD,UT1));
      }
    }
  }
  if ((get_names[0]=='t') && (get_names[1]=='h')) {
    if ( (atoi2(get_vals,&i)) && ((i>=0) && (i<=23))) { get_temp_hour=i; }
  }
  if ((get_names[0]=='t') && (get_names[1]=='m')) {
    if ( (atoi2(get_vals,&i)) && ((i>=0) && (i<=59))) { get_temp_minute=i; }
  }
  if ((get_names[0]=='t') && (get_names[1]=='s')) {
    if ( (atoi2(get_vals,&i)) && ((i>=0) && (i<=59))) {
      get_temp_second=i;
      char temp[10];
      sprintf(temp,"%02d:%02d:%02d",get_temp_hour,get_temp_minute,get_temp_second);
      i=highPrecision; highPrecision=true; 
      if (hmsToDouble(&LMT,temp)) {
        EEPROM_writeFloat(EE_LMT,LMT); 
        UT1=LMT+timeZone; 
        UT1_start  =UT1;
        UT1mS_start=millis(); 
        update_lst(jd2last(JD,UT1)); 
      }
      highPrecision=i;
    }
  }
  // Align
  if ((get_names[0]=='a') && (get_names[1]=='l')) {
    if ((get_vals[0]=='1') && (get_vals[1]==0)) startAlign(get_vals[0]);
    if ((get_vals[0]=='2') && (get_vals[1]==0)) startAlign(get_vals[0]);
    if ((get_vals[0]=='3') && (get_vals[1]==0)) startAlign(get_vals[0]);
    if ((get_vals[0]=='n') && (get_vals[1]==0)) nextAlign();
  }
  // Home/Park
  if ((get_names[0]=='h') && (get_names[1]=='m')) {
    if ((get_vals[0]=='r') && (get_vals[1]==0)) setHome();
    if ((get_vals[0]=='f') && (get_vals[1]==0)) goHome();
  }
  if ((get_names[0]=='p') && (get_names[1]=='k')) {
    if ((get_vals[0]=='p') && (get_vals[1]==0)) park();
    if ((get_vals[0]=='u') && (get_vals[1]==0)) unpark();
    if ((get_vals[0]=='s') && (get_vals[1]==0)) setPark();
  }
  // Tracking control
  if ((get_names[0]=='t') && (get_names[1]=='k')) {
    if ((get_vals[0]=='o') && (get_vals[1]=='n') && (get_vals[2]==0)) trackingState=TrackingSidereal;
    if ((get_vals[0]=='o') && (get_vals[1]=='f') && (get_vals[2]=='f') && (get_vals[3]==0)) trackingState=TrackingNone;
    if (((get_vals[0]=='f') || (get_vals[0]=='-') || (get_vals[0]=='r')) && (get_vals[1]==0)) {
       if (get_vals[0]=='f') siderealInterval+=HzCf*(0.1);
       if (get_vals[0]=='-') siderealInterval-=HzCf*(0.1);
       if (get_vals[0]=='r') siderealInterval=15956313L;
       EEPROM_writeLong(EE_siderealInterval,siderealInterval);
       SetSiderealClockRate(siderealInterval);
       cli(); SiderealRate=siderealInterval/StepsPerSecondAxis1; sei();
    }
    if ((get_vals[0]=='s') && (get_vals[1]==0)) SetTrackingRate(default_tracking_rate); // sidereal
    if ((get_vals[0]=='l') && (get_vals[1]==0)) SetTrackingRate(0.96236513150); // lunar
    if ((get_vals[0]=='h') && (get_vals[1]==0)) SetTrackingRate(0.99726956632); // solar
  }
  // PEC control
  if ((get_names[0]=='p') && (get_names[1]=='e')) {
    if ((get_vals[0]=='p') && (get_vals[1]=='l') && (get_vals[2]==0)) { if (pecRecorded) pecStatus=ReadyPlayPEC; } // play
    if ((get_vals[0]=='s') && (get_vals[1]=='t') && (get_vals[2]==0)) { pecStatus=IgnorePEC; }; // stop
    if ((get_vals[0]=='r') && (get_vals[1]=='e') && (get_vals[2]==0)) { pecStatus=ReadyRecordPEC; }; // record
    if ((get_vals[0]=='c') && (get_vals[1]=='l') && (get_vals[2]==0)) { // clear
      for (i=0; i<PECBufferSize; i++) pecBuffer[i]=128;
      pecFirstRecord = true;
      pecStatus      = IgnorePEC;
      pecRecorded    = false;
      EEPROM.update(EE_pecStatus,pecStatus);
      EEPROM.update(EE_pecRecorded,pecRecorded);
      wormSensePos=0;
      EEPROM_writeLong(EE_wormSensePos,wormSensePos);
    };
    if ((get_vals[0]=='w') && (get_vals[1]=='r') && (get_vals[2]==0)) { // write to eeprom
      pecRecorded=true;
      pecStatus=IgnorePEC;
      EEPROM.update(EE_pecRecorded,pecRecorded);
      EEPROM.update(EE_pecStatus,pecStatus);
      EEPROM_writeLong(EE_wormSensePos,wormSensePos);
      // trigger recording of PEC buffer
      pecAutoRecord=PECBufferSize;
    }
  }
}

// --------------------------------------------------------------------------------------------------------------
// Web-site pages, the design of each xxxxx_html_page() function is now capable of sending a web-page of any size
// since it stops and waits should the local outgoing buffer become full
const char html_header1[] PROGMEM = "HTTP/1.1 200 OK\r\n";
const char html_header2[] PROGMEM = "Content-Type: text/html\r\n";
const char html_header3[] PROGMEM = "Connection: close\r\n";
const char html_header4[] PROGMEM = "Refresh: 5\r\n\r\n";
const char html_header4a[] PROGMEM = "\r\n";
const char html_header5[] PROGMEM = "<!DOCTYPE HTML>\r\n<html>\r\n";
const char html_header6[] PROGMEM = "<head>\r\n";
const char html_header7[] PROGMEM = "</head>\r\n";
const char html_header8[] PROGMEM = "<body bgcolor=\"#26262A\">\r\n";

const char html_main_css1[] PROGMEM = "<STYLE>\r\n";
const char html_main_css2[] PROGMEM = ".a { background-color: #111111; }\r\n .t { padding: 15px; border: 15px solid #551111;\r\n";
const char html_main_css3[] PROGMEM = " margin: 25px; color: #999999; background-color: #111111; }\r\n input { width:4em; font-weight: bold; background-color: #A01010; padding: 2px 2px; }\r\n";
const char html_main_css4[] PROGMEM = ".b { padding: 30px; border: 2px solid #551111; margin: 30px; color: #999999; background-color: #111111; }\r\n";
const char html_main_css5[] PROGMEM = "select { width:4em; font-weight: bold; background-color: #A01010; padding: 2px 2px; }\r\n .c { color: #A01010; font-weight: bold; }\r\n";
const char html_main_css6[] PROGMEM = "h1 { text-align: right; }\r\n a:hover, a:active { background-color: red; }\r\n .g { color: #10A010; font-weight: bold; }";
const char html_main_css7[] PROGMEM = "a:link, a:visited { background-color: #332222; color: #a07070; border:1px solid red; padding: 5px 10px;";
const char html_main_css8[] PROGMEM = " margin: none; text-align: center; text-decoration: none; display: inline-block; }\r\n";
const char html_main_css9[] PROGMEM = "button { background-color: #A01010; font-weight: bold; border-radius: 5px; font-size: 12px; margin: 2px; padding: 4px 8px; }\r\n</STYLE>\r\n";

const char html_links1[] PROGMEM = "<a href=\"/index.htm\">Status</a><a href=\"/settings.htm\">";
const char html_links2[] PROGMEM = "Settings</a><a href=\"/control.htm\">Control</a>";
const char html_links3[] PROGMEM = "<a href=\"/pec.htm\">PEC</a><a href=\"/config.htm\">Configuration</a><br />";

// The index.htm page --------------------------------------------------------------------------------------
#ifdef MOUNT_TYPE_ALTAZM
  #define Axis1 "Az"
  #define Axis1a "Az"
  #define Axis2 "Alt"
#else
  #define Axis1 "HA"
  #define Axis1a "RA"
  #define Axis2 "Dec"
#endif
const char html_index1[] PROGMEM = "<div class=\"t\"><table width=\"100%\"><tr><td><b>" FirmwareName " " FirmwareNumber;
const char html_index2[] PROGMEM = "</b></td><td align=\"right\"><b><font size=\"5\">";
const char html_index2a[] PROGMEM = "STATUS</font></b></td></tr></table><br />";
const char html_index2b[] PROGMEM = "</div><div class=\"b\">Longitude = <font class=\"c\">%s</font>, Latitude = <font class=\"c\">%s</font><br />";
const char html_index3[] PROGMEM = "<font class=\"c\">%02d/%02d/%02d</font>";
const char html_index4[] PROGMEM = "&nbsp;<font class=\"c\">%s</font>&nbsp;UT";
const char html_index4a[] PROGMEM = "&nbsp;(<font class=\"c\">%s</font>&nbsp; Local Apparent Sidereal Time)<br /><br />";
const char html_indexFault[] PROGMEM =  "Stepper driver " Axis1 " axis %s, " Axis2 " axis %s<br /><br />";
const char html_indexTrue[] PROGMEM = "Stepper Position: " Axis1 "=<font class=\"c\">%ld</font>, " Axis2 "=<font class=\"c\">%ld</font><br />";
const char html_indexIndex[] PROGMEM = "IHS=<font class=\"c\">%ld</font>, IDS=<font class=\"c\">%ld</font><br />";
const char html_indexPosition[] PROGMEM = "Current Position: " Axis1 "=<font class=\"c\">%s</font>, " Axis2 "=<font class=\"c\">%s</font><br />";
const char html_indexTarget[] PROGMEM = "Target Position: " Axis1 "=<font class=\"c\">%s</font>, " Axis2 "=<font class=\"c\">%s</font><br />";
const char html_indexPier[] PROGMEM = "Pier Side=<font class=\"c\">%s</font> (meridian flips <font class=\"c\">%s</font>)<br /><br />";
const char html_index7[] PROGMEM = "Current MaxRate: <font class=\"c\">%ld</font> (Default MaxRate: <font class=\"c\">%ld</font>)<br /><br />";
const char html_index8[] PROGMEM = "Tracking: <font class=\"c\">%s %s</font><br />";
const char html_index9[] PROGMEM = "Parking: <font class=\"c\">%s</font><br />";
const char html_index10[] PROGMEM = "Last Error: <font class=\"c\">%s</font><br /><br />";
const char html_index11[] PROGMEM = "Workload: <font class=\"c\">%ld%%</font><br />";

void index_html_page() {
  char temp[320] = "";
  char temp1[320] = "";
  char temp2[80] = "";
  char temp3[80] = "";
  bool r=true;
  int stp=0;
  html_page_step++;
    
  // send a standard http response header
  if (html_page_step==++stp) strcpy_P(temp, html_header1); 
  if (html_page_step==++stp) strcpy_P(temp, html_header2);
  if (html_page_step==++stp) strcpy_P(temp, html_header3);
  if (html_page_step==++stp) strcpy_P(temp, html_header4);
  if (html_page_step==++stp) strcpy_P(temp, html_header5);
  if (html_page_step==++stp) strcpy_P(temp, html_header6);

  if (html_page_step==++stp) strcpy_P(temp, html_main_css1);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css2);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css3);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css4);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css5);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css6);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css7);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css8);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css9);
  
  if (html_page_step==++stp) strcpy_P(temp, html_header7);
  if (html_page_step==++stp) strcpy_P(temp, html_header8);

  // and the remainder of the page
  if (html_page_step==++stp) strcpy_P(temp, html_index1);
  if (html_page_step==++stp) strcpy_P(temp, html_index2);
  if (html_page_step==++stp) strcpy_P(temp, html_index2a);
  if (html_page_step==++stp) strcpy_P(temp, html_links1);
  if (html_page_step==++stp) strcpy_P(temp, html_links2);
  if (html_page_step==++stp) strcpy_P(temp, html_links3);
  
if (html_page_step==++stp) {
    i=highPrecision; 
    highPrecision=false; 
    doubleToDms(temp2,&longitude,true,true);
    doubleToDms(temp3,&latitude,false,true);
    highPrecision=i;
    strcpy_P(temp1, html_index2b); sprintf(temp,temp1,temp2,temp3);
  }
  if (html_page_step==++stp) { 
    f1=JD;
    f=UT1; while (f>24.0) { f-=24.0; f1+=1; }
    
    greg(f1,&i2,&i,&i1); 
    i2=(i2/99.99999-floor(i2/99.99999))*100;

    strcpy_P(temp1, html_index3); sprintf(temp,temp1,i,i1,i2);
  }
  if (html_page_step==++stp) { 
    i=highPrecision; highPrecision=true; 
    f=UT1; while (f>24.0) f-=24.0;
    doubleToHms(temp2,&f);
    highPrecision=i;

    strcpy_P(temp1, html_index4); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) { 
    i=highPrecision; highPrecision=true;
    f=LST();
    doubleToHms(temp2,&f);
    highPrecision=i;
    
    strcpy_P(temp1, html_index4a); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) {
    strcpy_P(temp1, html_indexFault);
    sprintf(temp, temp1, faultAxis1 ? "<font class=\"c\">FAULT</font>" : "<font class=\"g\">Ok</font>", faultAxis2 ? "<font class=\"c\">FAULT</font>" : "<font class=\"g\">Ok</font>");
  }
  if (html_page_step==++stp) {
    cli();
    long a1=(posAxis1+blAxis1)-trueAxis1;
    long a2=(posAxis2+blAxis2)-trueAxis2;
    sei();
    strcpy_P(temp1, html_indexTrue); sprintf(temp,temp1,a1,a2); 
  }
  if (html_page_step==++stp) {
    strcpy_P(temp1, html_indexIndex); sprintf(temp,temp1,IHS,IDS); 
  }
  if (html_page_step==++stp) {
    i=highPrecision; highPrecision=true; 
    cli();
    long h=posAxis1+IHS;
    long d=posAxis2+IDS;
    sei();
#ifdef MOUNT_TYPE_ALTAZM
    double ha=(double)h/(double)StepsPerDegreeAxis1; 
    doubleToDms(temp2,&ha,true,true);
#else
    double ha=(double)h/(double)(StepsPerDegreeAxis1*15L); 
    doubleToHms(temp2,&ha);
#endif
    double dec=(double)d/(double)StepsPerDegreeAxis2; 
    doubleToDms(temp3,&dec,false,true);
    highPrecision=i;
    
    strcpy_P(temp1, html_indexPosition); sprintf(temp,temp1,temp2,temp3); 
  }
  if (html_page_step==++stp) {
    i=highPrecision; highPrecision=true;
    cli();
    long h=(long)targetAxis1.part.m+IHS;
    long d=(long)targetAxis2.part.m+IDS;
    sei();
#ifdef MOUNT_TYPE_ALTAZM
    double ha=(double)h/(double)(StepsPerDegreeAxis1);
    doubleToDms(temp2,&ha,true,true);
#else
    double ha=(double)h/(double)(StepsPerDegreeAxis1*15L);
    doubleToHms(temp2,&ha);
#endif
    double dec=(double)d/(double)StepsPerDegreeAxis2; 
    doubleToDms(temp3,&dec,false,true);
    highPrecision=i;
    
    strcpy_P(temp1, html_indexTarget); sprintf(temp,temp1,temp2,temp3); 
  }
  if (html_page_step==++stp) {
    if ((pierSide==PierSideFlipWE1) ||
        (pierSide==PierSideFlipWE2) ||
        (pierSide==PierSideFlipWE3)) strcpy(temp2,"Meridian Flip, West to East"); else
    if ((pierSide==PierSideFlipEW1) ||
        (pierSide==PierSideFlipEW2) ||
        (pierSide==PierSideFlipEW3)) strcpy(temp2,"Meridian Flip, East to West"); else
    if (pierSide==PierSideWest) strcpy(temp2,"West"); else
    if (pierSide==PierSideEast) strcpy(temp2,"East"); else
    if (pierSide==PierSideNone) strcpy(temp2,"None"); else strcpy(temp2,"Unknown");
    if (meridianFlip==MeridianFlipNever) strcpy(temp3,"Disabled"); else strcpy(temp3,"Enabled");
    strcpy_P(temp1, html_indexPier); sprintf(temp,temp1,temp2,temp3); 
  }
  if (html_page_step==++stp) {
    strcpy_P(temp1, html_index7); sprintf(temp,temp1,maxRate/16L,(long)MaxRate);
  }
  if (html_page_step==++stp) {
    if (trackingState==TrackingNone)     strcpy(temp2,"Off");
    if (trackingState==TrackingSidereal) strcpy(temp2,"On");
    if (trackingState==TrackingMoveTo)   strcpy(temp2,"Slewing");

    if ((PPSsynced) && (atHome)) strcpy(temp3,"(at home, PPS sync)");
    if ((PPSsynced) && (!atHome)) strcpy(temp3,"(PPS sync)");
    if ((!PPSsynced) && (atHome)) strcpy(temp3,"(at home)");
    if ((!PPSsynced) && (!atHome)) strcpy(temp3,"");
    
    strcpy_P(temp1, html_index8); sprintf(temp,temp1,temp2,temp3);
  }
  if (html_page_step==++stp) {
    if (parkStatus==NotParked)  strcpy(temp2,"Not Parked");
    if (parkStatus==Parked)     strcpy(temp2,"Parked");
    if (parkStatus==Parking)    strcpy(temp2,"Parking");
    if (parkStatus==ParkFailed) strcpy(temp2,"Park Failed");

    strcpy_P(temp1, html_index9); sprintf(temp,temp1,temp2,(worst_loop_time*100L)/9970L);
    worst_loop_time=0;
  }
  if (html_page_step==++stp) {
    if (lastError==ERR_NONE) strcpy(temp2,"None"); else
    if (lastError==ERR_MOTOR_FAULT) strcpy(temp2,"Motor Fault"); else
    if (lastError==ERR_ALT) strcpy(temp2,"Altitude Min/Max"); else
    if (lastError==ERR_LIMIT_SENSE) strcpy(temp2,"Limit Sense"); else
    if (lastError==ERR_DEC) strcpy(temp2,"Dec Limit Exceeded"); else
    if (lastError==ERR_AZM) strcpy(temp2,"Azm Limit Exceeded"); else
    if (lastError==ERR_UNDER_POLE) strcpy(temp2,"Under Pole Limit Exceeded"); else
    if (lastError==ERR_MERIDIAN) strcpy(temp2,"Meridian Limit (W) Exceeded");
    strcpy_P(temp1, html_index10); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) {
    strcpy_P(temp1, html_index11); sprintf(temp,temp1,(worst_loop_time*100L)/9970L);
    worst_loop_time=0;
  }
  if (html_page_step==++stp) strcpy(temp,"</div></body></html>");


  // stop sending this page
  if (html_page_step==++stp) { html_page_step=0; responseStarted=false; return; }

  // send the data
  r=www_write(temp);
  if (!r) html_page_step--; // repeat this step if www_write failed
}

// The settings.htm page ----------------------------------------------------------------------------------
const char html_settings1[] PROGMEM = "<div class=\"t\"><table width=\"100%\"><tr><td><b>" FirmwareName " " FirmwareNumber ;
const char html_settings2[] PROGMEM = "</b></td><td align=\"right\"><b><font size=\"5\">SETTINGS</font></b></td></tr></table><br />";
const char html_settings3[] PROGMEM = "</div><div class=\"b\">\r\n";
const char html_settings4[] PROGMEM = 
"Maximum Slew Speed: "
"<form method=\"get\" action=\"/settings.htm\">"
"<button name=\"ss\" value=\"vs\" type=\"submit\">Very Slow (0.5x)</button>"
"<button name=\"ss\" value=\"s\" type=\"submit\">Slow (0.75x)</button>";
const char html_settings5[] PROGMEM = 
"<button name=\"ss\" value=\"n\" type=\"submit\">Normal (1x) </button>"
"<button name=\"ss\" value=\"f\" type=\"submit\">Fast (1.5x) </button>"
"<button name=\"ss\" value=\"vf\" type=\"submit\">Very Fast (2x) </button>"
"</form><br />\r\n";
const char html_settings7[] PROGMEM = 
"Backlash: <br />"
"<form method=\"get\" action=\"/settings.htm\">"
" <input value=\"%d\" type=\"number\" name=\"b1\" min=\"0\" max=\"999\">"
"<button type=\"submit\">Upload</button>"
" (" Axis1a ", in arc-seconds 0 to 999)"
"</form>"
"\r\n";
const char html_settings8[] PROGMEM = 
"<form method=\"get\" action=\"/settings.htm\">"
" <input value=\"%d\" type=\"number\" name=\"b2\" min=\"0\" max=\"999\">"
"<button type=\"submit\">Upload</button>"
" (" Axis2 ", in arc-seconds 0 to 999)"
"</form>"
"<br />\r\n";
const char html_settings9[] PROGMEM = 
"Limits: <br />"
"<form method=\"get\" action=\"/settings.htm\">"
" <input value=\"%d\" type=\"number\" name=\"hl\" min=\"-30\" max=\"30\">"
"<button type=\"submit\">Upload</button>"
" (Horizon, in degrees +/- 30)"
"</form>"
"\r\n";
const char html_settings10[] PROGMEM = 
"<form method=\"get\" action=\"/settings.htm\">"
" <input value=\"%d\" type=\"number\" name=\"ol\" min=\"60\" max=\"90\">"
"<button type=\"submit\">Upload</button>"
" (Overhead, in degrees 60 to 90)"
"</form>"
"<br />\r\n";
const char html_settings11[] PROGMEM = 
"Location: <br />"
"<form method=\"get\" action=\"/settings.htm\">"
" <input value=\"%s\" type=\"number\" name=\"g1\" min=\"-180\" max=\"180\">&nbsp;&deg;&nbsp;";
const char html_settings12[] PROGMEM = 
" <input value=\"%s\" type=\"number\" name=\"g2\" min=\"0\" max=\"60\">&nbsp;'&nbsp;&nbsp;"
"<button type=\"submit\">Upload</button>"
" (Longitude, in deg. and min. +/- 180)"
"</form>"
"\r\n";
const char html_settings13[] PROGMEM = 
"<form method=\"get\" action=\"/settings.htm\">"
" <input value=\"%s\" type=\"number\" name=\"t1\" min=\"-90\" max=\"90\">&nbsp;&deg;&nbsp;";
const char html_settings14[] PROGMEM = 
" <input value=\"%s\" type=\"number\" name=\"t2\" min=\"0\" max=\"60\">&nbsp;'&nbsp;&nbsp;"
"<button type=\"submit\">Upload</button>"
" (Latitude, in deg. and min. +/- 90)"
"</form>"
"\r\n";
const char html_settings15[] PROGMEM = 
"<form method=\"get\" action=\"/settings.htm\">"
" <input value=\"%s\" type=\"number\" name=\"u1\" min=\"-12\" max=\"14\">&nbsp;hrs&nbsp;";
const char html_settings16[] PROGMEM = 
" <select name=\"u2\"><option value=\"0\" %s>00</option><option value=\"30\" %s>30</option><option value=\"45\" %s>45</option></select>&nbsp;min.&nbsp;"
"<button type=\"submit\">Upload</button>"
" (UTC Offset, in hours -12 to +14)"
"</form>"
"\r\n";

void settings_html_page() {
  char temp[320] = "";
  char temp1[320] = "";
  char temp2[80] = "";
  int stp=0;
  html_page_step++;

  // send a standard http response header
  if (html_page_step==++stp) strcpy_P(temp, html_header1);
  if (html_page_step==++stp) strcpy_P(temp, html_header2);
  if (html_page_step==++stp) strcpy_P(temp, html_header3);
  if (html_page_step==++stp) strcpy_P(temp, html_header4a);
  if (html_page_step==++stp) strcpy_P(temp, html_header5);
  if (html_page_step==++stp) strcpy_P(temp, html_header6);

  if (html_page_step==++stp) strcpy_P(temp, html_main_css1);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css2);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css3);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css4);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css5);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css6);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css7);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css8);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css9);
  
  if (html_page_step==++stp) strcpy_P(temp, html_header7);
  if (html_page_step==++stp) strcpy_P(temp, html_header8);

  // and the remainder of the page
  if (html_page_step==++stp) strcpy_P(temp, html_settings1);
  if (html_page_step==++stp) strcpy_P(temp, html_settings2);
  if (html_page_step==++stp) strcpy_P(temp, html_links1);
  if (html_page_step==++stp) strcpy_P(temp, html_links2);
  if (html_page_step==++stp) strcpy_P(temp, html_links3);
  if (html_page_step==++stp) strcpy_P(temp, html_settings3);
  if (html_page_step==++stp) strcpy_P(temp, html_settings4);
  if (html_page_step==++stp) strcpy_P(temp, html_settings5);
  if (html_page_step==++stp) { strcpy_P(temp1, html_settings7); i=(int)round(((double)backlashAxis1*3600.0)/(double)StepsPerDegreeAxis1); if (i<0) i=0; if (i>999) i=999; sprintf(temp,temp1,i); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_settings8); i=(int)round(((double)backlashAxis2*3600.0)/(double)StepsPerDegreeAxis2); if (i<0) i=0; if (i>999) i=999; sprintf(temp,temp1,i); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_settings9); sprintf(temp,temp1,minAlt); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_settings10); sprintf(temp,temp1,maxAlt); }
  if (html_page_step==++stp) {
    i=highPrecision; 
    highPrecision=false; 
    doubleToDms(temp2,&longitude,true,true);
    temp2[4]=0; // deg. part only
    if (temp2[0]=='+') temp2[0]='0'; // remove +
    highPrecision=i;
    strcpy_P(temp1, html_settings11); sprintf(temp,temp1,temp2);
    }
  if (html_page_step==++stp) { 
    i=highPrecision; 
    highPrecision=false; 
    doubleToDms(temp2,&longitude,true,true);
    highPrecision=i;
    strcpy_P(temp1, html_settings12); sprintf(temp,temp1,(char*)&temp2[5]);
  }
  if (html_page_step==++stp) { 
    i=highPrecision; 
    highPrecision=false; 
    doubleToDms(temp2,&latitude,false,true);
    temp2[3]=0; // deg. part only
    if (temp2[0]=='+') temp2[0]='0'; // remove +
    highPrecision=i;
    strcpy_P(temp1, html_settings13); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) { 
    i=highPrecision; 
    highPrecision=false; 
    doubleToDms(temp2,&latitude,false,true);
    highPrecision=i;
    strcpy_P(temp1, html_settings14); sprintf(temp,temp1,(char*)&temp2[4]);
  }
  if (html_page_step==++stp) {
    timeZoneToHM(temp2,timeZone);
    temp2[3]=0; // deg. part only
    if (temp2[0]=='+') temp2[0]='0'; // remove +
    strcpy_P(temp1, html_settings15); sprintf(temp,temp1,temp2);
  }
  if (html_page_step==++stp) {
    timeZoneToHM(temp2,timeZone);
    strcpy_P(temp1, html_settings16);
    if (temp2[3]==0) sprintf(temp,temp1,"selected","",""); else
    if (temp2[4]=='3') sprintf(temp,temp1,"","selected",""); else
    if (temp2[4]=='4') sprintf(temp,temp1,"","","selected");
  }
  if (html_page_step==++stp) strcpy(temp,"</div></body></html>");

  // stop sending this page
  if (html_page_step==++stp) { html_page_step=0; responseStarted=false; return; }

  // send the data
  if (!www_write(temp)) html_page_step--; // repeat this step if www_write failed
}

// The control.htm page ----------------------------------------------------------------------------------
const char html_control1[] PROGMEM = "<div class=\"t\"><table width=\"100%\"><tr><td><b>" FirmwareName " " FirmwareNumber ;
const char html_control2[] PROGMEM = "</b></td><td align=\"right\"><b><font size=\"5\">CONTROL</font></b></td></tr></table><br />";
const char html_control3[] PROGMEM = "</div><div class=\"b\">\r\n";
const char html_control4a[] PROGMEM = 
"Date/Time: "
"<form method=\"get\" action=\"/control.htm\">"
"<button type=\"submit\" onclick=\"SetDateTime();\">Set</button>"
"<input id=\"dm\" type=\"hidden\" name=\"dm\">"
"<input id=\"dd\" type=\"hidden\" name=\"dd\">";
const char html_control4b[] PROGMEM = 
"<input id=\"dy\" type=\"hidden\" name=\"dy\">"
"<input id=\"th\" type=\"hidden\" name=\"th\">"
"<input id=\"tm\" type=\"hidden\" name=\"tm\">"
"<input id=\"ts\" type=\"hidden\" name=\"ts\">"
"</form><br />\r\n";
const char html_control4c[] PROGMEM =
"<script>\r\n"
"function SetDateTime() {"
"var d = new Date();"
"document.getElementById(\"dd\").value = d.getDate();"
"document.getElementById(\"dm\").value = d.getMonth();"
"document.getElementById(\"dy\").value = d.getFullYear();";
const char html_control4d[] PROGMEM =
"document.getElementById(\"th\").value = d.getHours();"
"document.getElementById(\"tm\").value = d.getMinutes();"
"document.getElementById(\"ts\").value = d.getSeconds();"
"}\r\n"
"</script>\r\n";
const char html_controlAlign1[] PROGMEM =
"Align: "
"<form method=\"get\" action=\"/control.htm\">"
"<button name=\"al\" value=\"1\" type=\"submit\">1 Star</button>";
const char html_controlAlign23[] PROGMEM = 
"<button name=\"al\" value=\"2\" type=\"submit\">2 Star</button>"
"<button name=\"al\" value=\"3\" type=\"submit\">3 Star</button>";
const char html_controlAlign4[] PROGMEM = 
"<br /><button name=\"al\" value=\"n\" type=\"submit\">Accept</button>"
"</form><br />\r\n";
const char html_control6[] PROGMEM = 
"Home/Park: "
"<form method=\"get\" action=\"/control.htm\">"
"<button name=\"hm\" value=\"r\" type=\"submit\">Home (Reset)</button>&nbsp;&nbsp;&nbsp;"
"<button name=\"hm\" value=\"f\" type=\"submit\">Find Home</button><br />";
const char html_control7[] PROGMEM = 
"<button name=\"pk\" value=\"p\" type=\"submit\">Park</button>&nbsp;&nbsp;&nbsp;"
"<button name=\"pk\" value=\"u\" type=\"submit\">Un-Park</button><br />"
"<button name=\"pk\" value=\"s\" type=\"submit\">Set-Park</button>"
"</form><br />\r\n";
const char html_control8[] PROGMEM = 
"Tracking: "
"<form method=\"get\" action=\"/control.htm\">"
"<button name=\"tk\" value=\"on\" type=\"submit\">On</button>"
"<button name=\"tk\" value=\"off\" type=\"submit\">Off</button>&nbsp;&nbsp;&nbsp;";
const char html_control9[] PROGMEM = 
"<button name=\"tk\" value=\"f\" type=\"submit\">+ (0.1Hz faster)</button>"
"<button name=\"tk\" value=\"-\" type=\"submit\">- (0.1Hz slower)</button>"
"<button name=\"tk\" value=\"r\" type=\"submit\">Reset (default)</button><br />";
const char html_control10[] PROGMEM = 
"<button name=\"tk\" value=\"s\" type=\"submit\">Sidereal</button>"
"<button name=\"tk\" value=\"l\" type=\"submit\">Lunar</button>"
"<button name=\"tk\" value=\"h\" type=\"submit\">Solar</button>"
"</form>\r\n";

void control_html_page() {
  char temp[320] = "";
  int stp=0;
  html_page_step++;

  // send a standard http response header
  if (html_page_step==++stp) strcpy_P(temp, html_header1);
  if (html_page_step==++stp) strcpy_P(temp, html_header2);
  if (html_page_step==++stp) strcpy_P(temp, html_header3);
  if (html_page_step==++stp) strcpy_P(temp, html_header4a);
  if (html_page_step==++stp) strcpy_P(temp, html_header5);
  if (html_page_step==++stp) strcpy_P(temp, html_header6);

  if (html_page_step==++stp) strcpy_P(temp, html_main_css1);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css2);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css3);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css4);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css5);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css6);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css7);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css8);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css9);
  
  if (html_page_step==++stp) strcpy_P(temp, html_header7);
  if (html_page_step==++stp) strcpy_P(temp, html_header8);

  // and the remainder of the page
  if (html_page_step==++stp) strcpy_P(temp, html_control1);
  if (html_page_step==++stp) strcpy_P(temp, html_control2);
  if (html_page_step==++stp) strcpy_P(temp, html_links1);
  if (html_page_step==++stp) strcpy_P(temp, html_links2);
  if (html_page_step==++stp) strcpy_P(temp, html_links3);
  if (html_page_step==++stp) strcpy_P(temp, html_control3);
  if (html_page_step==++stp) strcpy_P(temp, html_control4a);
  if (html_page_step==++stp) strcpy_P(temp, html_control4b);
  if (html_page_step==++stp) strcpy_P(temp, html_control4c);
  if (html_page_step==++stp) strcpy_P(temp, html_control4d);
  if (html_page_step==++stp) strcpy_P(temp, html_controlAlign1);
#if defined(MOUNT_TYPE_GEM) || defined(MOUNT_TYPE_FORK_ALT)
  if (html_page_step==++stp) strcpy_P(temp, html_controlAlign23);
#endif
  if (html_page_step==++stp) strcpy_P(temp, html_controlAlign4);
  if (html_page_step==++stp) strcpy_P(temp, html_control6);
  if (html_page_step==++stp) strcpy_P(temp, html_control7);
  if (html_page_step==++stp) strcpy_P(temp, html_control8);
  if (html_page_step==++stp) strcpy_P(temp, html_control9);
  if (html_page_step==++stp) strcpy_P(temp, html_control10);
  if (html_page_step==++stp) strcpy(temp,"</div></body></html>");

  // stop sending this page
  if (html_page_step==++stp) { html_page_step=0; responseStarted=false; return; }

  // send the data
  if (!www_write(temp)) html_page_step--; // repeat this step if www_write failed
}

// The pec.htm page ----------------------------------------------------------------------------------
const char html_pec1[] PROGMEM = "<div class=\"t\"><table width=\"100%\"><tr><td><b>" FirmwareName " " FirmwareNumber ;
const char html_pec2[] PROGMEM = "</b></td><td align=\"right\"><b><font size=\"5\">PEC</font></b></td></tr></table><br />";
const char html_pec3[] PROGMEM = "</div><div class=\"b\">\r\n";
const char html_pec4[] PROGMEM = "PEC: <br /><br />";
const char html_pec5[] PROGMEM = "STATUS = <font class=\"c\">%s</font><br /><br />";
const char html_pecControls1[] PROGMEM =
"<form method=\"get\" action=\"/pec.htm\">"
"<button name=\"pe\" value=\"pl\" type=\"submit\">Play</button>";
const char html_pecControls2[] PROGMEM = 
"<button name=\"pe\" value=\"st\" type=\"submit\">Stop</button><br /><br />"
"<button name=\"pe\" value=\"cl\" type=\"submit\">Clear</button><button name=\"pe\" value=\"re\" type=\"submit\">";
const char html_pecControls3[] PROGMEM = 
"Record</button><br />Clear erases the memory buffer not EEPROM.  During recording corrections are averaged 3:1 favoring the buffer unless the buffer was cleared in which case the full correction is used.<br /><br />"
"<button name=\"pe\" value=\"wr\" type=\"submit\">Write to EEPROM</button><br />Writing PEC data to EEPROM pauses everything in OnStep for about 2 to 4 seconds.</form>"
"</form><br />\r\n";

void pec_html_page() {
  char temp[320] = "";
  char temp1[320] = "";
  char temp2[80] = "";
  int stp=0;
  html_page_step++;

  // send a standard http response header
  if (html_page_step==++stp) strcpy_P(temp, html_header1);
  if (html_page_step==++stp) strcpy_P(temp, html_header2);
  if (html_page_step==++stp) strcpy_P(temp, html_header3);
  if (html_page_step==++stp) strcpy_P(temp, html_header4);
  if (html_page_step==++stp) strcpy_P(temp, html_header5);
  if (html_page_step==++stp) strcpy_P(temp, html_header6);

  if (html_page_step==++stp) strcpy_P(temp, html_main_css1);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css2);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css3);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css4);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css5);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css6);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css7);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css8);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css9);
  
  if (html_page_step==++stp) strcpy_P(temp, html_header7);
  if (html_page_step==++stp) strcpy_P(temp, html_header8);

  // and the remainder of the page
  if (html_page_step==++stp) strcpy_P(temp, html_pec1);
  if (html_page_step==++stp) strcpy_P(temp, html_pec2);
  if (html_page_step==++stp) strcpy_P(temp, html_links1);
  if (html_page_step==++stp) strcpy_P(temp, html_links2);
  if (html_page_step==++stp) strcpy_P(temp, html_links3);
  if (html_page_step==++stp) strcpy_P(temp, html_pec3);
  if (html_page_step==++stp) strcpy_P(temp, html_pec4);
  if (html_page_step==++stp) {
#ifndef MOUNT_TYPE_ALTAZM
    const char *pecStatusCh = PECStatusString;
    if (pecStatusCh[pecStatus]=='I') { strcpy(temp2,"Idle"); } else
    if (pecStatusCh[pecStatus]=='p') { strcpy(temp2,"Play waiting to start"); } else
    if (pecStatusCh[pecStatus]=='P') { strcpy(temp2,"Playing"); } else
    if (pecStatusCh[pecStatus]=='r') { strcpy(temp2,"Record waiting for the index to arrive"); } else
    if (pecStatusCh[pecStatus]=='R') { strcpy(temp2,"Recording"); } else
#endif
      strcpy(temp2,"Unknown");
    strcpy_P(temp1, html_pec5); sprintf(temp,temp1,temp2);
  }

#ifdef MOUNT_TYPE_ALTAZM
  if (html_page_step==++stp) strcpy(temp, "PEC CONTROLS DISABLED");
#else
  if (html_page_step==++stp) strcpy_P(temp, html_pecControls1);
  if (html_page_step==++stp) strcpy_P(temp, html_pecControls2);
  if (html_page_step==++stp) strcpy_P(temp, html_pecControls3);
#endif
  if (html_page_step==++stp) strcpy(temp,"</div></body></html>");

  // stop sending this page
  if (html_page_step==++stp) { html_page_step=0; responseStarted=false; return; }

  // send the data
  if (!www_write(temp)) html_page_step--; // repeat this step if www_write failed
}

// The config.htm page --------------------------------------------------------------------------------------
const char html_config1[] PROGMEM = "<div class=\"t\"><table width=\"100%\"><tr><td><b>" FirmwareName " " FirmwareNumber;
const char html_config2[] PROGMEM = "</b></td><td align=\"right\"><b><font size=\"5\">";
const char html_config3[] PROGMEM = "CONFIGURATION</font></b></td></tr></table><br />";
const char html_config4[] PROGMEM = "</div><div class=\"b\">";
#ifdef DEBUG_ON
const char html_debug[] PROGMEM = "Debug Mode: <font class=\"c\">ON</font><br /><br />";
#else
const char html_debug[] PROGMEM = "Debug Mode: <font class=\"c\">OFF</font><br /><br />";
#endif
#ifdef SYNC_ANYWHERE_ON
const char html_sync[] PROGMEM = "Sync Anywhere: <font class=\"c\">ON</font><br /><br />";
#else
const char html_sync[] PROGMEM = "Sync Anywhere: <font class=\"c\">OFF</font><br /><br />";
#endif
#if defined(MOUNT_TYPE_GEM)
const char html_configMount[] PROGMEM = "Mount type: <font class=\"c\">GEM</font><br /><br />";
#elif defined(MOUNT_TYPE_FORK)
const char html_configMount[] PROGMEM = "Mount type: <font class=\"c\">Fork or similar</font><br /><br />";
#elif defined(MOUNT_TYPE_FORKALT)
const char html_configMount[] PROGMEM = "Mount type: <font class=\"c\">Fork or similar (Alternate)</font><br /><br />";
#elif defined(MOUNT_TYPE_ALTAZM)
const char html_configMount[] PROGMEM = "Mount type: <font class=\"c\">AltAz</font><br /><br />";
#else
const char html_configMount[] PROGMEM = "Mount type: <font class=\"c\">Unknown</font><br /><br />";
#endif
#if defined(ST4_OFF)
const char html_configST4[] PROGMEM = "ST4 interface <font class=\"c\">OFF</font> (";
#elif defined(ST4_ON)
const char html_configST4[] PROGMEM = "ST4 interface <font class=\"c\">ON</font> (<font class=\"c\">no internal pullup resistors</font>, ";
#elif defined(ST4_PULLUP)
const char html_configST4[] PROGMEM = "ST4 interface <font class=\"c\">ON</font> (<font class=\"c\">with internal pullup resistors</font>, ";
#else
const char html_configST4[] PROGMEM = "ST4 interface <font class=\"c\">OFF</font> (";
#endif
#ifdef ST4_ALTERNATE_PINS_ON
const char html_pinsST4[] PROGMEM = "at <font class=\"c\">Alternate Pins</font>)<br /><br />";
#else
const char html_pinsST4[] PROGMEM = "at <font class=\"c\">Default Pins</font>)<br /><br />";
#endif
#ifdef PPS_SENSE_ON
const char html_configPPS[] PROGMEM = "PPS sense is <font class=\"c\">ON</font>, ";
#else
const char html_configPPS[] PROGMEM = "PPS sense is <font class=\"c\">OFF</font>, ";
#endif
#ifdef PEC_SENSE_ON
const char html_configPEC[] PROGMEM = "PEC sense is <font class=\"c\">ON</font>, ";
#else
const char html_configPEC[] PROGMEM = "PEC sense is <font class=\"c\">OFF</font>, ";
#endif
#ifdef LIMIT_SENSE_ON
const char html_configLimit[] PROGMEM = "LIMIT sense is <font class=\"c\">ON</font><br /><br />";
#else
const char html_configLimit[] PROGMEM = "LIMIT sense is <font class=\"c\">OFF</font><br /><br />";
#endif
#ifdef STATUS_LED_PINS_ON
const char html_configLed1[] PROGMEM = "Status LED 1 <font class=\"c\">ON</font>, ";
#else
const char html_configLed1[] PROGMEM = "Status LED 1 <font class=\"c\">OFF</font>, ";
#endif
#if defined(STATUS_LED2_PINS_ON)
const char html_configLed2[] PROGMEM = "Status LED 2 <font class=\"c\">ON</font>, ";
#elif defined(STATUS_LED2_PINS)
const char html_configLed2[] PROGMEM = "Status LED 2 <font class=\"c\">Reticule Mode, ";
#else
const char html_configLed2[] PROGMEM = "Status LED 2 <font class=\"c\">OFF</font>, ";
#endif
#ifdef RETICULE_LED_PINS
const char html_configLed3[] PROGMEM = "Reticule LED <font class=\"c\">ON</font><br /><br />";
#else
const char html_configLed3[] PROGMEM = "Reticule LED <font class=\"c\">OFF</font><br /><br />";
#endif
#ifdef POWER_SUPPLY_PINS_ON
const char html_configPwr[] PROGMEM = "Optional Logic Level Power to Stepper drivers <font class=\"c\">ON</font><br />";
#else
const char html_configPwr[] PROGMEM = "Optional Logic Level Power to Stepper drivers <font class=\"c\">OFF</font><br />";
#endif
#if defined(AXIS1_DISABLED_HIGH)
const char html_configA1En[] PROGMEM = "Stepper driver Enable line support Axis1 (RA/Az) disabled <font class=\"c\">HIGH";
#elif defined(AXIS1_DISABLED_LOW)
const char html_configA1En[] PROGMEM = "Stepper driver Enable line support Axis1 (RA/Az) disabled <font class=\"c\">LOW";
#else
const char html_configA1En[] PROGMEM = "Stepper driver Enable line support Axis1 (RA/Az) disabled <font class=\"c\">Unknown";
#endif
#if defined(AXIS2_DISABLED_HIGH)
const char html_configA2En[] PROGMEM = "</font>, Axis2 (Dec/Alt) disabled <font class=\"c\">HIGH</font><br />";
#elif defined(AXIS2_DISABLED_LOW)
const char html_configA2En[] PROGMEM = "</font>, Axis2 (Dec/Alt) disabled <font class=\"c\">LOW</font><br />";
#else
const char html_configA2En[] PROGMEM = "</font>, Axis2 (Dec/Alt) disabled <font class=\"c\">Unknown</font><br />";
#endif
#if defined(AXIS1_FAULT_LOW)
const char html_configA1Flt[] PROGMEM = "Stepper driver Fault on Axis1 (RA/Az) when <font class=\"c\">LOW</font>, ";
#elif defined(AXIS1_FAULT_HIGH)
const char html_configA1Flt[] PROGMEM = "Stepper driver Fault on Axis1 (RA/Az) when <font class=\"c\">HIGH</font>, ";
#elif defined(AXIS1_FAULT_OFF)
const char html_configA1Flt[] PROGMEM = "Stepper driver Fault on Axis1 (RA/Az) <font class=\"c\">OFF</font>, ";
#endif
#if defined(AXIS2_FAULT_LOW)
const char html_configA2Flt[] PROGMEM = "Axis2 (Dec/Alt) when <font class=\"c\">LOW</font><br /><br />";
#elif defined(AXIS2_FAULT_HIGH)
const char html_configA2Flt[] PROGMEM = "Axis2 (Dec/Alt) when <font class=\"c\">HIGH</font><br /><br />";
#elif defined(AXIS2_FAULT_OFF)
const char html_configA2Flt[] PROGMEM = "Axis2 (Dec/Alt) <font class=\"c\">OFF</font><br /><br />";
#endif
#ifdef TRACK_REFRACTION_RATE_DEFAULT_ON
const char html_configTrack[] PROGMEM = "Tracking compensation for refraction effect, defaults to <font class=\"c\">ON</font><br /><br />";
#else
const char html_configTrack[] PROGMEM = "Tracking compensation for refraction effect, defaults to <font class=\"c\">OFF</font><br /><br />";
#endif
#ifdef SEPERATE_PULSE_GUIDE_RATE_ON
const char html_configGuide[] PROGMEM = "Separate pulse guide rate <font class=\"c\">ON</font><br /><br />";
#else
const char html_configGuide[] PROGMEM = "Separate pulse guide rate <font class=\"c\">OFF</font><br /><br />";
#endif
#ifdef RememberMaxRate_ON
const char html_configRemMax[] PROGMEM = "Remember MaxRate setting changes <font class=\"c\">ON</font><br /><br />";
#else
const char html_configRemMax[] PROGMEM = "Remember MaxRate setting changes <font class=\"c\">OFF</font><br /><br />";
#endif
const char html_configMaxRate[] PROGMEM = "MaxRate <font class=\"c\">%d</font>uS<br /><br />";
const char html_configDegAcc[] PROGMEM = "Degrees for acceleration/deceleration <font class=\"c\">%s</font><br />";
const char html_configBacklash[] PROGMEM = "Backlash takeup rate <font class=\"c\">%d</font> x Sidereal<br /><br />";
const char html_configStepsDegHA[] PROGMEM = "Steps per degree for Axis1 (RA/Az) <font class=\"c\">%d</font><br />";
const char html_configStepsDegDec[] PROGMEM = "Steps per degree for Axis2 (Dec/Alt) <font class=\"c\">%d</font><br />";
const char html_configStepsPerSec[] PROGMEM = "Steps per second for tracking <font class=\"c\">%s</font><br /><br />";
const char html_configStepsPerWR[] PROGMEM = "Steps per worm rotation for PEC <font class=\"c\">%d</font>, ";
const char html_configPecBufSize[] PROGMEM = "PEC buffer size <font class=\"c\">%d</font> seconds<br /><br />";
const char html_configMPME[] PROGMEM = "Minutes past meridian East <font class=\"c\">%d</font> minutes<br />";
const char html_configMPMW[] PROGMEM = "Minutes past meridian West <font class=\"c\">%d</font> minutes<br />";
const char html_configUPL[] PROGMEM = "Under pole limit <font class=\"c\">%d</font> hours<br />";
const char html_configMinDec[] PROGMEM = "Minimum Declination <font class=\"c\">%d</font> degrees<br />";
const char html_configMaxDec[] PROGMEM = "Maximum Declination <font class=\"c\">%d</font> degrees<br /><br />";

void config_html_page() {
  char temp[320] = "";
  char temp1[320] = "";
  char temp2[80] = "";
  char temp3[80] = "";
  int stp=0;
  html_page_step++;
    
  // send a standard http response header
  if (html_page_step==++stp) strcpy_P(temp, html_header1); 
  if (html_page_step==++stp) strcpy_P(temp, html_header2);
  if (html_page_step==++stp) strcpy_P(temp, html_header3);
  if (html_page_step==++stp) strcpy_P(temp, html_header4a);
  if (html_page_step==++stp) strcpy_P(temp, html_header5);
  if (html_page_step==++stp) strcpy_P(temp, html_header6);

  if (html_page_step==++stp) strcpy_P(temp, html_main_css1);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css2);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css3);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css4);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css5);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css6);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css7);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css8);
  if (html_page_step==++stp) strcpy_P(temp, html_main_css9);
  
  if (html_page_step==++stp) strcpy_P(temp, html_header7);
  if (html_page_step==++stp) strcpy_P(temp, html_header8);

  // and the remainder of the page
  if (html_page_step==++stp) strcpy_P(temp, html_config1);
  if (html_page_step==++stp) strcpy_P(temp, html_config2);
  if (html_page_step==++stp) strcpy_P(temp, html_config3);
  if (html_page_step==++stp) strcpy_P(temp, html_links1);
  if (html_page_step==++stp) strcpy_P(temp, html_links2);
  if (html_page_step==++stp) strcpy_P(temp, html_links3);
  if (html_page_step==++stp) strcpy_P(temp, html_config4);

  if (html_page_step==++stp) strcpy_P(temp, html_debug);
  if (html_page_step==++stp) strcpy_P(temp, html_sync);
  if (html_page_step==++stp) strcpy_P(temp, html_configMount);
  if (html_page_step==++stp) strcpy_P(temp, html_configST4);
  if (html_page_step==++stp) strcpy_P(temp, html_pinsST4);
  if (html_page_step==++stp) strcpy_P(temp, html_configPPS);
  if (html_page_step==++stp) strcpy_P(temp, html_configPEC);
  if (html_page_step==++stp) strcpy_P(temp, html_configLimit);
  if (html_page_step==++stp) strcpy_P(temp, html_configLed1);
  if (html_page_step==++stp) strcpy_P(temp, html_configLed2);
  if (html_page_step==++stp) strcpy_P(temp, html_configLed3);
  if (html_page_step==++stp) strcpy_P(temp, html_configPwr);
  if (html_page_step==++stp) strcpy_P(temp, html_configA1En);
  if (html_page_step==++stp) strcpy_P(temp, html_configA2En);
  if (html_page_step==++stp) strcpy_P(temp, html_configA1Flt);
  if (html_page_step==++stp) strcpy_P(temp, html_configA2Flt);
  if (html_page_step==++stp) strcpy_P(temp, html_configTrack);
  if (html_page_step==++stp) strcpy_P(temp, html_configGuide);
  if (html_page_step==++stp) strcpy_P(temp, html_configRemMax);
  if (html_page_step==++stp) { strcpy_P(temp1, html_configMaxRate); sprintf(temp,temp1,MaxRate); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configDegAcc); dtostrf(DegreesForAcceleration,2,1,temp2); sprintf(temp,temp1,temp2); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configBacklash); sprintf(temp,temp1,(long)round(BacklashTakeupRate)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configStepsDegHA); sprintf(temp,temp1,(long)round(StepsPerDegreeAxis1)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configStepsDegDec); sprintf(temp,temp1,(long)round(StepsPerDegreeAxis2)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configStepsPerSec); dtostrf(StepsPerSecondAxis1,3,6,temp2); sprintf(temp,temp1,temp2); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configStepsPerWR); sprintf(temp,temp1,(long)round(StepsPerWormRotationAxis1)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configPecBufSize); sprintf(temp,temp1,(long)round(PECBufferSize)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configMPME); sprintf(temp,temp1,(long)round(MinutesPastMeridianE)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configMPMW); sprintf(temp,temp1,(long)round(MinutesPastMeridianW)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configUPL); sprintf(temp,temp1,(long)round(UnderPoleLimit)); }
 
  if (html_page_step==++stp) { strcpy_P(temp1, html_configMinDec); sprintf(temp,temp1,(long)round(MinDec)); }
  if (html_page_step==++stp) { strcpy_P(temp1, html_configMaxDec); sprintf(temp,temp1,(long)round(MaxDec)); }


  if (html_page_step==++stp) strcpy(temp,"</div></body></html>");

  // stop sending this page
  if (html_page_step==++stp) { html_page_step=0; responseStarted=false; return; }

  // send the data
  if (!www_write(temp)) html_page_step--; // repeat this step if www_write failed
}

#endif

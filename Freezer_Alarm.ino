
//This code written by ChatGPT
// It didnt compile, and had quite a few issues.

#include <ESP8266WiFi.h>
#include <WiFiClientSecureAxTLS.h>
//#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Replace these with your own Wi-Fi credentials
const char* ssid = "A_Virtual_Information_Ext";
const char* password = "BananaRock";
boolean connectWifi();
boolean wifiConnected = false;
boolean OverTemp = false;

// Set up the temperature sensor
// Data wire is plugged into digital pin IO0
#define ONE_WIRE_BUS 0
const int LedPin = 2;  // GPIO2 pin. // On board LED on some ESP01's

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);


//float temperatureThreshold = -126.0; // Set the temperature threshold for test/debug
//float temperatureThreshold = 23.0; // Set the temperature threshold for room temp test/debug
float temperatureThreshold = -9.0; // Set the temperature threshold for in service use
float CurrentTemperature = 0.0;
float MaxTemperature = -30; // freezer will likely never get this cold, and will be overwritten in first pass with sensible value
float MinTemperature = 0.0;// freezer will likely never get this warm, and will be overwritten in first pass with sensible value

// Set up the web server
//ESP8266WebServer server(80);

const char* WatchDogHost = "192.168.1.60"; // ip address of the watchdog esp8266
long WatchDogCounterLoopThreshold = 60;// 60 is about 60 secs
long WatchDogLoopCounter = 0;

int   AlarmNumber = 35 ;// defines which Virtual Button number is passed to the watchdog proxy and then triggered in Virtual Button land 

String NormalColor = "ForestGreen";
String FaultColor = "FireBrick";


const long timeoutTime = 2000; // Define web page timeout time in milliseconds (example: 2000ms = 2s)
String NormalFaultState = "Normal";


String EventLogArray[64] ; // 20 events each containing Date [0], Time[1], Proxt [2]. last 3 entries (60,61,62) are headers
//String LastRebootDate ;
//String LastRebootTime ;
//float DayCounter = 0;
long LoopCounter =0; // number of 1 second loops since reboot
long DayCounter = 0;
long HourCounter = 0;
long MinuteCounter = 0;
long SecondCounter = 0;



byte EventLogArrayIndex = 0;
//byte WDResponcesArrayIndex = 0;

WiFiServer server(80); // internally facing web page is http.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
WiFiClientSecure client;
#pragma GCC diagnostic pop

String header; // Variable to store the HTTP request for the local webserver

unsigned long currentTime = millis();

unsigned long previousTime = 0;
unsigned long previousMillis = 0;

bool TempAlarmMute = true; // start unmuted
bool DoorAlarmMute = true; // start unmuted

bool DoorSwitchState = false; // Current State of the Door
bool PrevDoorSwitchState;
const int DoorSwitchPin = 2; // same pin as used for led o/p during void setup...3 is the RX pin

void setup() {
  //delay(5000); // let the serial port initistalsation crap complete
  pinMode(LedPin, OUTPUT); // to start with, it might change later...
  
  // Set up serial communication for debugging
  Serial.begin(115200);
  delay(2000); // let the serial port initistalsation crap complete
    Serial.println("Booting - Freezer Alarm...");
  delay(1000);
  Serial.println("flashing LED on GPIO2 (if supported)...");
  //flash GPIO2 (if supported) fast a few times to indicate CPU is booting
  digitalWrite(LedPin, LOW);
  delay(100);
  digitalWrite(LedPin, HIGH);
  delay(100);
  digitalWrite(LedPin, LOW);
  delay(100);
  digitalWrite(LedPin, HIGH);
  delay(100);
  digitalWrite(LedPin, LOW);
  delay(100);
  digitalWrite(LedPin, HIGH);

  Serial.println("Delaying a bit...");
  delay(2000);

// Initialise wifi connection
  wifiConnected = connectWifi();

  if (wifiConnected) {
    Serial.println("flashing slow to indicate wifi connected...");
    //flash slow a few times to indicate wifi connected OK
    digitalWrite(LedPin, LOW);
    delay(1000);
    digitalWrite(LedPin, HIGH);
    delay(1000);
    digitalWrite(LedPin, LOW);
    delay(1000);
    digitalWrite(LedPin, HIGH);
    delay(1000);
    digitalWrite(LedPin, LOW);
    delay(1000);
    digitalWrite(LedPin, HIGH);

  }
  
  
    
  // Initialize the temperature sensor
  sensors.begin(); // start the library
  // Set up the web server
  //server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started");

   // Set up the digital input pin to detect the freezer door
 
  pinMode(DoorSwitchPin, FUNCTION_3);
  pinMode(DoorSwitchPin, INPUT);
 



// populate event log headers
      EventLogArray[60] = "Date";
      EventLogArray[61] = "Time";
      EventLogArray[62] = "Event Info";
      
      EventLogArray[57] = "1st Entry date";
      EventLogArray[58] = "1st Entry time";
      EventLogArray[59] = "1st Entry Event";

      EventLogArray[54] = "2 Entry date";
      EventLogArray[55] = "2 Entry time";
      EventLogArray[56] = "2 Entry Event";

      EventLogArray[51] = "3 Entry date";
      EventLogArray[52] = "3 Entry time";
      EventLogArray[53] = "3 Entry Event";
      
      EventLogArray[30] = "1/2way date";
      EventLogArray[31] = " 1/2way Time";
      EventLogArray[32] = "1/2way Event Info";



      EventLogArray[3] = "2nd2Last Entry date";
      EventLogArray[4] = "2nd2Last Entry time";
      EventLogArray[5] = "2nd2Last Entry Event";

      EventLogArray[0] = "oldest Entry date";
      EventLogArray[1] = "oldest Entry time";
      EventLogArray[2] = "oldest Entry Event";
    
    
     
     // Populate " - " in all Event log slots

     for (EventLogArrayIndex=0; EventLogArrayIndex<57; EventLogArrayIndex = EventLogArrayIndex+1){
    
     EventLogArray[EventLogArrayIndex] = " - ";
     }



  //TempAlarmMute = false; // start muted
  //DoorAlarmMute = false; // start muted
  
  Serial.println("VOID setup completed");
}


void loop() {
  //Serial.print("void loop starting");
  // Door Switch monitoring 
  DoorSwitchState = digitalRead(DoorSwitchPin); 
  delay(10);  
  if (DoorSwitchState == LOW) {
      if (PrevDoorSwitchState == HIGH){ 
        Serial.println("Freezer Door Just opened"); 
            if (DoorAlarmMute == 1) {// ie not muted             
                
                AlarmNumber = 27; // Door open alarm code
                EventPost(); 
              
                Serial.println("Door open call");                         
           }
        }
      }
        
      
  if (DoorSwitchState == HIGH) {
       if (PrevDoorSwitchState == LOW){ 
        Serial.println("Freezer Door Just closed");        
      }
  }      

   PrevDoorSwitchState = DoorSwitchState;   // remember prev state for next pass

  

  // Read the temperature from the sensor
  sensors.requestTemperatures();
CurrentTemperature = (sensors.getTempCByIndex(0));

  //print the CurrentTemperature in Celsius to the serial port
  Serial.print("CurrentTemperature: ");
  Serial.print(sensors.getTempCByIndex(0));
  
  Serial.print((char)176);//shows degrees character
  Serial.print("C  |  ");
  
 
  //updateTemperatureReadings(temperature);
  // Check if the CurrentTemperature is above or below the threshold
  OverTemp = false;
  if (CurrentTemperature > temperatureThreshold) {
    // Send a message to the watchdog proxy, indicate on web page ect
    Serial.println("setting OverTemp to true");
    OverTemp = true;
  } 
  if (CurrentTemperature > MaxTemperature) {
    
    MaxTemperature = CurrentTemperature;
  } 
    if (CurrentTemperature < MinTemperature) {
    
    MinTemperature = CurrentTemperature;
  } 
  
  
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client)
  { // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            Serial.println(header);
            //delay (5); // so you can interpet header contents

// Check for button presses ect from client

            if (header.indexOf("GET /TempMuteAlarm") >= 0) {
              TempAlarmMute = 1;
            }
            if (header.indexOf("GET /TempUnMuteAlarm") >= 0) {
              TempAlarmMute = 0;
            }

            if (header.indexOf("GET /DoorMuteAlarm") >= 0) {
              DoorAlarmMute = 1;
            }
            if (header.indexOf("GET /DoorUnMuteAlarm") >= 0) {
              DoorAlarmMute = 0;
            }

            if (header.indexOf("GET /IncTempThreshold") >= 0) {
              temperatureThreshold = temperatureThreshold + 1;
            }
            if (header.indexOf("GET /DecTempThreshold") >= 0) {
              temperatureThreshold = temperatureThreshold - 1;
            }

            if (header.indexOf("GET /ResetMaxMin") >= 0) {
              MaxTemperature = 0;
              MinTemperature = -30;
            }

        //Display the web page

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            //client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("<style>html { font-family: Helvetica; display: inline; margin: 0; margin-top:0px; padding: 0; text-align: center;}");

            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");

            //client.println(".redrectangle { background-color: red; border: none; color: black; padding: 40px 10px;}");
            //client.println(".purplerectangle { background-color: purple; border: none; color: black; padding: 100px 10px;}");
            //client.println(".genericrectangle { background-color: brown; border: none; color: white; padding: " + String(RectHeight) + "px " + String(RectWidth) + "px;}");
            //client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");


            //client.println(".button2 {background-color: #77878A;}");
            client.println(".buttonFault {background-color:" + String(FaultColor) + ";}");
            client.println(".buttonNormal {background-color:" + String(NormalColor) + ";}</style></head>");

            //client.println(".buttonsmall { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            //client.println("text-decoration: none; font-size: 10px; margin: 2px; cursor: pointer;}");

            // Web Page Heading
            client.println("<body><h1>Freezer Status</h1>");

            // Display current watchdog state
            

            client.println ("<h1>Current Freezer Temperature: " + String(CurrentTemperature) + "°C</h1>\n");
            client.println ("");                       
            client.println ("<h1>Past Maximum Temperature: " + String(MaxTemperature) + "°C</h1>\n");
            client.println ("<h1>Past Minimum Temperature: " + String(MinTemperature) + "°C</h1>\n");
            client.println (""); 
            client.println ("<h1>Alarm Temperature Threshold: " + String(temperatureThreshold) + "°C</h1>\n");

            if (OverTemp == true) {
              client.println("<p><a href=\"/2/on\"><button class=\"button buttonFault\">Temperature Fault</button></a> </p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button buttonNormal\">Temperature Normal</button></a></p>");
            }

            if (TempAlarmMute == 0) {
              client.println("<p><a href=\"/2/on\"><button class=\"button buttonFault\">Alexa Temp Alarm Muted</button></a> </p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button buttonNormal\">Temp Alarm Enabled</button></a></p>");
            }
            if (DoorSwitchState == 0) {
              client.println("<p><a href=\"/2/on\"><button class=\"button buttonFault\">Freezer Door Open</button></a> </p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button buttonNormal\">Freezer Door Closed</button></a></p>");
            }

            if (DoorAlarmMute == 0) {
              client.println("<p><a href=\"/2/on\"><button class=\"button buttonFault\">Alexa Door Alarm Muted</button></a> </p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button buttonNormal\">Door Alarm Enabled</button></a></p>");
            }         
            // Display date
            client.println("<p>Uptime " + String(DayCounter) + " Days " + String(HourCounter) + " Hours " + String(MinuteCounter) + " Minutes " + String(LoopCounter) + " Seconds" + "</p>");
                   
                        
/*
            //client.println ("<h2>2 hourly Temperature readings :</h2>\n");
                  // display Event log
                  // put extended logging stuff here     
                   // Display log 60 - 30
                   /*
                    // display Eventlog2
                   for (EventLogArrayIndex=63; EventLogArrayIndex>0; EventLogArrayIndex = EventLogArrayIndex-3){
    
                   client.println("<p>" + (EventLogArray[EventLogArrayIndex]) + "       " + (EventLogArray[(EventLogArrayIndex+1)]) + " " + (EventLogArray[(EventLogArrayIndex+2)])+  "</p>");
                   }
*/
                   client.print("<p><a href=\"/IncTempThreshold\"><button class=\"buttonsmall\">Inc Alarm Temp Threshold</button></a> <a href=\"/DecTempThreshold\"><button class=\"buttonsmall\">Dec Alarm Temp Threshold</button></a></p>");
                   //client.print("<p><a href=\"/IncStandThreshold\"><button class=\"buttonsmall\">Inc Standing Time</button></a> <a href=\"/DecStandThreshold\"><button class=\"buttonsmall\">Dec Standing Time</button></a></p>");
                    client.println();
                    

                    // Display Mute Buttons
                    if (TempAlarmMute == 1) {
                      client.println("<p><a href=\"/TempUnMuteAlarm\"><button class=\"buttonsmall\">Mute Temp Alarm</button></a> </p>");
                    } else {
                      client.println("<p><a href=\"/TempMuteAlarm\"><button class=\"buttonsmall\">Unmute Temp Alarm</button></a> </p>");
                    }

                    if (DoorAlarmMute == 1) {
                      client.println("<p><a href=\"/DoorUnMuteAlarm\"><button class=\"buttonsmall\">Mute Door Alarm</button></a></p>");
                    } else {
                      client.println("<p><a href=\"/DoorMuteAlarm\"><button class=\"buttonsmall\">Unmute Door Alarm</button></p>");
                    }

                    //Display Max/Min reset Button
                    client.println("<p><a href=\"/ResetMaxMin\"> <button class=\"buttonsmall\">Reset Max Min Temps</button></a></p>");
                   
                    
                    //Display EventLog Button
                    //client.println("<p><a href=\"/EventLog2\"> <button class=\"buttonsmall\">EventLog</button></a></p>");
                    //client.println("<p><a href=\"/WatchDog\"><button class=\"buttonsmall\">WatchDog</button></a> <a href=\"/WDFailLog\"><button class=\"buttonsmall\">WDFailLog</button></a> <a href=\"/EventLog2\"> <button class=\"buttonsmall\">EventLog</button></a></p>");




  // Update the web page with the current temperature and the graph of the last week's readings
  //updateWebPage(temperature); // dont know why this call fails, code copied inline to void loop for now...
//Update the web page with the current temperature and the graph of the last week's readings

  String html = "<html>\n";
  html += "<head>\n";
  html += "<title>Freezer Temperature Monitor</title>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<h1>Freezer Temperature: " + String(CurrentTemperature) + "°C</h1>\n";
  html += "<h1>Alarm Temperature Threshold: " + String(temperatureThreshold) + "°C</h1>\n";
  //html += "<h2>Temperature readings for the last week:</h2>\n";
  //html += "<div id=\"chart\"></div>\n";
  //html += "</body>\n";
  //html += "</html>\n";

                 client.println("</body></html>");
                  // The HTTP response ends with another blank line
                  client.println();
                  // Break out of the while loop
                  break;
                } else { // if you got a newline, then clear currentLine
                  currentLine = "";
                }
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
              currentLine += c;      // add it to the end of the currentLine
            }
          }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
      }
  
  //server.send(200, "text/html", html);
//Serial.print("HTML to display temp complete");
  
  // Handle incoming web server requests
  //server.handleClient();
  //Serial.print("Handle client call complete");



WatchDogLoopCounter = WatchDogLoopCounter +1;
   Serial.println(WatchDogLoopCounter);
   if (WatchDogLoopCounter > WatchDogCounterLoopThreshold) {
        WatchDogLoopCounter = 0;
        AlarmNumber = 35; // Freezer alarm watchdog code
        WatchDogPost();
        Serial.println("Watchdog checkin call");
        if (TempAlarmMute == 1) {// ie not muted
            //Serial.println("checking for alarms to be sent");
            //Serial.println (String(CurrentTemperature) + "°C");
            //Serial.println (String(temperatureThreshold) + "°C");
  
            if (OverTemp == true) {  // ie, the freezer is over threshold triggered
              
              AlarmNumber = 26; // freezer overtemp alarm code
              EventPost(); 
            
              Serial.println("Freezer overtemp call");
            }
             if (CurrentTemperature < -100) {  // the temp sensor default reading is -127 degrees, this is displayed when the sensor is broken!
              AlarmNumber = 26; // freezer overtemp alarm code
              EventPost(); 
              Serial.println("Temp sensor missing alarm call");
            } 
              
        }

            if (DoorAlarmMute == 1) {// ie not muted
              if (DoorSwitchState == 0) {  // ie, the freezer door is open
                
                AlarmNumber = 27; // Door open alarm code
                EventPost(); 
              
                Serial.println("Door open call");
              }            
           }
   }
  
  // Add a delay between readings
    delay(1000); // roughly a second

  LoopCounter = LoopCounter +2;

// Maintain RTC 

       
        if (LoopCounter == 60)
        {
           // Things to do every second here
          LoopCounter = 0;
          MinuteCounter = MinuteCounter + 1;
          //Serial.println("another minute has passed");

        }
        if (MinuteCounter == 60)
        {
          

          // Things to do every hour here
         MinuteCounter = 0;
         HourCounter = HourCounter + 1;
         DayCounter = DayCounter + 0.0417 ; // (1/24)
         //Serial.println("DayCounter =  " + String(DayCounter) );

        }
        if (HourCounter == 24)
        {
          // Things to do every day here
          LoopCounter = 0;
          MinuteCounter = 0;
          HourCounter = 0;
          
  
        }

  

    
    
  //Serial.println("Void LOOP complete");
} // end of void loop

void EventPost() {
TwitchLED();
// assumes AlarmNumber set to desired Proxy call to be made

// 26 is freezer overtemp alarm, 27 is door open too long

  Serial.print("Requesting POST to WatchDog ");
  Serial.println(AlarmNumber);

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(WatchDogHost, httpPort)) {
    Serial.println("connection failed");
    return;
  }

String data = "";

   
   
   // Send request to the server:
   client.println("POST / HTTP/1.1");
   client.println("Host: ProxyRequest" +(String(AlarmNumber))); // this endpoint value gets to the server and is used to transfer the identity of the calling slave
   client.println("Accept: */*"); // this gets to the server!
   client.println("Content-Type: application/x-www-form-urlencoded");
   client.print("Content-Length: ");
   client.println(data.length());
   client.println();
   client.print(data);
  
   delay(500); // Can be changed
  if (client.connected()) { 
    client.stop();  // DISCONNECT FROM THE SERVER
  }
  Serial.println();
  Serial.println("closing connection");
  delay(1000);
}// end EventPost

void WatchDogPost() {

TwitchLED();

// assumes AlarmNumber set to desired VB call to be made

// 35 is freezer watchdog

  Serial.print("Requesting POST to WatchDog ");
  Serial.println(AlarmNumber);

  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(WatchDogHost, httpPort)) {
    Serial.println("connection failed");
    return;
  }

String data = "";

   
   
   // Send request to the server:
   client.println("POST / HTTP/1.1");
   client.println("Host: WatchDog Endpoint" +(String(AlarmNumber))); // this endpoint value gets to the server and is used to transfer the identity of the calling slave
   client.println("Accept: */*"); // this gets to the server!
   client.println("Content-Type: application/x-www-form-urlencoded");
   client.print("Content-Length: ");
   client.println(data.length());
   client.println();
   client.print(data);
  
   delay(500); // Can be changed
  if (client.connected()) { 
    client.stop();  // DISCONNECT FROM THE SERVER
  }
  Serial.println();
  Serial.println("closing connection");
  delay(1000);
}// end WatchDogPost

void TwitchLED () {

  
  digitalWrite(LedPin, LOW);
  delay(10);
  digitalWrite(LedPin, HIGH);
}



// connect to wifi – returns true if successful or false if not
boolean connectWifi() {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi Network");

  // Wait for connection
  Serial.print("Connecting ...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    Serial.print(".");
    if (i > 10) {
      state = false;
      break;
    }
    i++;
  }

  if (state) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Connection failed. Bugger");
  }

  return state;
}


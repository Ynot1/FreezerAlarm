
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
boolean WeeklyGraph = 0;
boolean DailyGraph = 0;
boolean StatusPage = 1;

int DSTOffset = 1;

unsigned long currentTime = millis();

unsigned long previousTime = 0;
unsigned long previousMillis = 0;
byte currentseconds = 0 ;
byte currentminutes = 55;
byte currenthours = 12 ;
long currentday = 0 ;
long currentmonth = 0 ;
byte Minute15Counter =14;

boolean SetTimeWasSuccesfull = 0;

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
long WatchDogSecondCounter = 0;

int   AlarmNumber = 35 ;// defines which Virtual Button number is passed to the watchdog proxy and then triggered in Virtual Button land 

String NormalColor = "ForestGreen";
String FaultColor = "FireBrick";


const long timeoutTime = 2000; // Define web page timeout time in milliseconds (example: 2000ms = 2s)
String NormalFaultState = "Normal";


float DailyTempArray[110] ; // 96 * 15min entries . array set at 100 cos round numbers
// index 0 is the reading just taken, 1 is 30 min ago ect 

//float DayCounter = 0;
long SecondCounter =0; // number of 1 second loops since reboot
long DayCounter = 0;
long HourCounter = 0;
long MinuteCounter = 0;
//long SecondCounter = 0;



byte DailyTempArrayIndex = 0;
//byte WDResponcesArrayIndex = 0;

WiFiServer server(80); // internally facing web page is http.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
WiFiClientSecure client;
#pragma GCC diagnostic pop

String header; // Variable to store the HTTP request for the local webserver


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
  Serial.println("Booting - Freezer Alarm...");
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
 



// populate daily temp array with dummy values
      DailyTempArray[0] = -11.11;
      DailyTempArray[1] = -22.22;
      DailyTempArray[2] = -30.33;

      DailyTempArray[12] = -12.00;
DailyTempArray[19] = -19.00;

            DailyTempArray[24] = -24;

            DailyTempArray[50] = -50;
      
      DailyTempArray[94] = -100.1;
      DailyTempArray[95] = -100.2;
      DailyTempArray[96] = -96.3;
       DailyTempArray[100] = -100.3;

  SetTime(); // sysnc the clock..
     // Serial.println(" Delaying 5 sec before trying clock sync again...");
   //   delay (5000);
 // SetTime(); // sysnc the clock.. 
     //  Serial.println(" comleted 2nd clock sync ..");
       /*
  // Load root certificate in DER format into WiFiClientSecure object
  bool res = client.setCACert_P(caCert, caCertLen);
  if (!res) {
    Serial.println("Failed to load root CA certificate!");
    while (true) {
      yield();
    }
    Serial.println("root CA certificate loaded");
  }
*/
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

            if (header.indexOf("GET /WeeklyGraph") >= 0) {
              WeeklyGraph = 1;
              DailyGraph = 0;
              StatusPage = 0;
            }

            if (header.indexOf("GET /DailyGraph") >= 0) {
              WeeklyGraph = 0;
              DailyGraph = 1;
              StatusPage = 0;
            }

           if (header.indexOf("GET /StatusPage") >= 0) {
              WeeklyGraph = 0;
              DailyGraph = 0;
              StatusPage = 1;
            }

             if (header.indexOf("GET /DummyEntry") >= 0) {

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

              client.print("<p><a href=\"/StatusPage\"><button class=\"buttonsmall\">Status Page</button></a> <a href=\"/DailyGraph\"><button class=\"buttonsmall\">DailyGraph</button></a></p>");
                   client.println();

            // Web Page Heading
            client.println("<body><h1>Freezer Status</h1>");

             // Display date
                client.println("<p>Current Date is " + String(currentday) + " / " + String(currentmonth)  + "</p>");
              // Display current time of day
                client.println("<p>Current Time is " + String(currenthours) + ":" + String(currentminutes) + ":" + String(currentseconds) + ":" + "</p>");
              

if (StatusPage == 1) {;

            // Display current freezer state
            

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
            client.println("<p>Uptime " + String(DayCounter) + " Days " + String(HourCounter) + " Hours " + String(MinuteCounter) + " Minutes " + String(SecondCounter) + " Seconds" + "</p>");
                   
                        
/*
            //client.println ("<h2>2 hourly Temperature readings :</h2>\n");
                  // display Event log
                  // put extended logging stuff here     
                   // Display log 60 - 30
                   /*
                    // display DailyTemp2
                   for (DailyTempArrayIndex=63; DailyTempArrayIndex>0; DailyTempArrayIndex = DailyTempArrayIndex-3){
    
                   client.println("<p>" + (DailyTempArray[DailyTempArrayIndex]) + "       " + (DailyTempArray[(DailyTempArrayIndex+1)]) + " " + (DailyTempArray[(DailyTempArrayIndex+2)])+  "</p>");
                   }
*/
                   client.print("<p><a href=\"/IncTempThreshold\"><button class=\"buttonsmall\">Inc Alarm Temp Threshold</button></a> <a href=\"/DecTempThreshold\"><button class=\"buttonsmall\">Dec Alarm Temp Threshold</button></a></p>");
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
                   
                    
                    //Display DailyTemp Button
                    //client.println("<p><a href=\"/DailyTemp2\"> <button class=\"buttonsmall\">DailyTemp</button></a></p>");
                    //client.println("<p><a href=\"/WatchDog\"><button class=\"buttonsmall\">WatchDog</button></a> <a href=\"/WDFailLog\"><button class=\"buttonsmall\">WDFailLog</button></a> <a href=\"/DailyTemp2\"> <button class=\"buttonsmall\">DailyTemp</button></a></p>");


}

if (DailyGraph == 1) {;

            // Display last 24 hrs temps


            client.println ("<p> Current Temp" + String(CurrentTemperature) + "</p>");
            client.println (""); 

                    client.println ("<p> 15min counter =" + String(Minute15Counter) + "</p>");

            client.println ("<p> 15 Min readings for last 24 hrs </p>");


// disply array contents 
     // for (LineIndex =0; LineIndex <10; LineIndex = LineIndex +1) {
      
                    client.print("<p>");
                    
                   for (DailyTempArrayIndex=0; DailyTempArrayIndex<=100; DailyTempArrayIndex = DailyTempArrayIndex+1){
    
                   client.print( String(DailyTempArray[DailyTempArrayIndex]) + " , " );
                   }
                    client.println("</p>");

                    

      
            
/*
 * 
 * 
                     client.print("<p>");
                    // display array contents 0 - 10
                   for (DailyTempArrayIndex=0; DailyTempArrayIndex<=9; DailyTempArrayIndex = DailyTempArrayIndex+1){
    
                   client.print( String(DailyTempArray[DailyTempArrayIndex]) + " , " );
                   }
                    client.println("</p>");

                    client.print("<p>");
                    // display array contents 10 - 19
                   for (DailyTempArrayIndex=10; DailyTempArrayIndex<=19; DailyTempArrayIndex = DailyTempArrayIndex+1){
    
                   client.print( String(DailyTempArray[DailyTempArrayIndex]) + " , " );
                   }
                    client.println("</p>");
 * 
 * client.print("<p>" + String(DailyTempArray[DailyTempArrayIndex]) + " , " +  "</p>");
                   }
client.println (" String(DailyTempArray[0])");
            client.println ("");

client.println ("String(CurrentTemperature)");
            client.println (""); 
  */          
}            

  // Update the web page with the current temperature 
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



WatchDogSecondCounter = WatchDogSecondCounter +1;
   Serial.println(WatchDogSecondCounter);
   if (WatchDogSecondCounter > WatchDogCounterLoopThreshold) {
        WatchDogSecondCounter = 0;
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
  
  // Add a delay between program cycles
    delay(1000); // roughly a second

  //SecondCounter = SecondCounter +2;

// Maintain RTC 

if (millis() >= (previousMillis)) {
        //Serial.print(" millis = ");
        //Serial.print(String (millis())); 
        previousMillis = previousMillis + 1000;
        //Serial.print(" prevmillis = ");
        //Serial.print(String (previousMillis));
        // should be here every second....



        currentseconds = currentseconds + 1;
        //SecondCounter = SecondCounter +1;
        if (currentseconds == 60)
        {
           // Things to do every minute here
          currentseconds = 0;
          currentminutes = currentminutes + 1;
          //MinuteCounter = MinuteCounter +1;
          Minute15Counter = Minute15Counter +1;
          Serial.println("another minute has passed");

        }
        if (currentminutes == 60)
        {
          

          // Things to do every hour here
         currentminutes = 0;
         currenthours = currenthours + 1;
         //HourCounter = HourCounter +1;
         DayCounter = DayCounter + 0.0417 ; // (1/24)
         //Serial.println("UpTimeDays =  " + String(UpTimeDays) );

        }
        if (currenthours == 24)
        {
          // Things to do every day here
          currentseconds = 0;
          currentminutes = 0;
          currenthours = 0;
         // DayCounter = DayCounter +1;
          
          SetTime (); // resync the clock
          //UpTimeDays = UpTimeDays + 0.5; IDK why but it sometimes counts 2X at this point
             
        }



      } // end 1 second

/*
// this doesnt fucking work, dont know why....
      if (Minute15Counter == 15) {
        Minute15Counter = 0;

        // shuffle Temp Array 1 place to the right

            
    // Rotates DailyTempArray[100] one slot to the right (toward index 100)


    
    Serial.println("moving daily temp array");
     for (DailyTempArrayIndex=100; DailyTempArrayIndex<=1; DailyTempArrayIndex = DailyTempArrayIndex-1){

     Serial.print ("DailyTempArrayIndex = " + String(DailyTempArrayIndex));
     DailyTempArray[DailyTempArrayIndex] = DailyTempArray[(DailyTempArrayIndex-1)];

   
    } 

        // Insert current temp into index 100
        Serial.println("inserting new temp into index 0");
           DailyTempArray[0] = CurrentTemperature ;
        
      }
*/

  
 // This works, but is backwards, ie new values are insrted at the right hand end 
            if (Minute15Counter == 15) {
        Minute15Counter = 0;

        // shuffle Temp Array 1 place to the left

            
    // Rotates DailyTempArray[100] one slot to the left (toward index 0)

    // for (DailyTempArrayIndex=0; DailyTempArrayIndex<=100; DailyTempArrayIndex = DailyTempArrayIndex+1){
    
    Serial.println("moving daily temp array");
     for (DailyTempArrayIndex=0; DailyTempArrayIndex<=99; DailyTempArrayIndex = DailyTempArrayIndex+1){
    
     DailyTempArray[DailyTempArrayIndex] = DailyTempArray[(DailyTempArrayIndex+1)];

    Serial.print ("DailyTempArrayIndex = " + String(DailyTempArrayIndex));
    } 

        // Insert current temp into index 100
        Serial.println("inserting new temp into index 0");
           DailyTempArray[100] = CurrentTemperature ;
        
      }

/*
//old loopcountig       
        if (SecondCounter == 60)
        {
           // Things to do every second here
          SecondCounter = 0;
          MinuteCounter = MinuteCounter + 1;
          //Serial.println("another minute has passed");

        }
        if (MinuteCounter == 60)
        {
          

          // Things to do every hour here
         MinuteCounter = 0;
         HourCounter = HourCounter + 1;
         //DayCounter = DayCounter + 0.0417 ; // (1/24)
         //Serial.println("DayCounter =  " + String(DayCounter) );

        }
        if (HourCounter == 24)
        {
          // Things to do every day here
          SecondCounter = 0;
          MinuteCounter = 0;
          HourCounter = 0;
          DayCounter = DayCounter +1;
  
        }
*/
  

    
    
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


   void SetTime () {
      SetTimeWasSuccesfull = 0;
      
      Serial.println(" trying to synch clock...");
      // Synchronize time using SNTP. This is necessary to verify that
      // the TLS certificates offered by the server are currently valid. Also used by the graph plotting routines
      Serial.println("Setting time using SNTP");
      configTime(-13 * 3600, DSTOffset, "pool.ntp.org", "time.nist.gov"); // set localtimezone, DST Offset, timeservers, timeservers...


      time_t now = time(nullptr);
      //while (now < 8 * 3600 * 2) { // would take 8 hrs to fall thru. Thats a long time....
      while (now < 100) {
        delay(200);
        Serial.print(".");
        Serial.print(String (now));
        now = time(nullptr);
      }
      Serial.println("");


      struct tm * timeinfo; //http://www.cplusplus.com/reference/ctime/tm/
      time(&now);
      timeinfo = localtime(&now);
      Serial.println(timeinfo->tm_mon);
      Serial.println(timeinfo->tm_mday);
      Serial.println(timeinfo->tm_hour);
      Serial.println(timeinfo->tm_min);
      Serial.println(timeinfo->tm_sec);
      currentseconds = timeinfo->tm_sec ;
      currentminutes = timeinfo->tm_min;
      currenthours = timeinfo->tm_hour ;
      currentday = timeinfo->tm_mday;
      currentmonth = timeinfo->tm_mon;
      currentmonth = currentmonth + 1; // month counted from 0
      SetTimeWasSuccesfull = 1;
      
 }


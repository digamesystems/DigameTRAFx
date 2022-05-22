/* BlueTrail -- A BLE interface to the TRAFx trail counter 
* 
* This version uses an ESP32-DEVKIT-C-V4 
* 
* debugUART - Console for Debugging
* trafxUART - TRAFx Trail Counter Module
* bleUART   - Bluetooth Classic Interface
* 
* OVERVIEW: 
* 
* [Phone] <--BLUETOOTH-> [ESP32] <--UART--> [App]
*                                          ^
*                                          |<--UART--> [TRAFx]
*                                          |<--UART--> [Console]
*                                          |<--SPI---> [RTC]
* 
* Copyright 2022, Digame Systems. All rights reserved. 
* 
*/

#include <digameTime.h>
#include <digameNetwork_v2.h>
#include "digameVersion.h"

#include "BluetoothSerial.h" // Part of the ESP32 board package. 
                             // By Evandro Copercini - 2018

// Aliases for easier reading
#define debugUART Serial
#define trafxUART Serial1

BluetoothSerial bleUART; // Create a BlueTooth Serial Port Object

// TRAFx IR trail counter pins
const int TRAFX_RST = 4; // Reset line to the Trafx Module
const int TRAFX_ENB = 2; // Enable TRAFx UART reset transistor switch

// Serial Ports
const int COM0 = 0;
const int COM1 = 1; 
const int COM2 = 2; 

bool passthrough_active = false;

//****************************************************************************************
void configureBluetooth(){
//****************************************************************************************
  debugUART.println(" Bluetooth...");
  bleUART.begin("BlueTrail_" + getShortMACAddress()); // My Bluetooth device name 
                                                      //  TODO: Provide opportunity to change names. 
  delay(1000);                                        // Give port time to initalize
    
}


/*
 * RTC /TIME FUNCTIONS
 */

// Returns the date time of the top of the next hour from now. 
String get_start_date_time(){
  String message = "";
  byte next_hour = 0;
  byte current_hour = 0;

  /*
   * This needs to be extended to handle year rollover, etc... 
   */
  current_hour = getRTCHour(); //Clock.getHour(h12,PM); //Assume we're on military time
  
  if (current_hour == 23){
      next_hour = 0;
  } else {
      next_hour = current_hour + 1;
  }
  
  // Start with the year
  message += twoDigits(getRTCYear());
  message += "-";
  message += twoDigits(getRTCMonth()); 
  message += "-";
  message += twoDigits(getRTCDay());
  message += " ";
  
  // Finally the hour and minute
  message += twoDigits(next_hour);
  message += ":";
  message += twoDigits(0);
  
  return message;  
  
}


// The user wants to set the RTC using one of the available serial interfaces.
void set_rtc(int port){
  // Set the external RTC
  DS3231 clock;
  String message = "";
  String data = "";
  
  message += "\n---------------------------------------\n";
  message += "REAL TIME CLOCK:\n";
  message += "  Current Date / Time: " + getRTCTime() + "\n";
  message += "  Change RTC? [y]/n >";
  
  ports_print(message);
  data = port_read(port);

  data.trim();
  
  ports_print(data);

  if ((data == "y")||(data =="")){
      
    ports_print(data + '\n');
  
    message = "  Year:  [" + String(getRTCYear()) + "] >" ;
    ports_print(message);
    data = port_read(port);
    if (data ==""){
      data = getRTCYear();
    }
    ports_print(data + '\n');
    clock.setYear(data.toInt()-1900-100); // TODO: Figure out a fix for this in c.a. 80 years...
    
    message = "  Month: [" + twoDigits(getRTCMonth()) + "] >" ;
    ports_print(message);
    data = port_read(port);
    if (data ==""){
      data = twoDigits(getRTCMonth()); 
    }
    ports_print(data + '\n');
    clock.setMonth(data.toInt());
  
    message = "  Day:   [" + twoDigits(getRTCDay()) + "] >" ;
    ports_print(message);
    data = port_read(port);
    if (data ==""){
      data = twoDigits(getRTCDay()); 
    }
    ports_print(data + '\n');
    clock.setDate(data.toInt());
  
    message = "  Hour:  [" + twoDigits(getRTCHour()) + "] >" ;
    ports_print(message);
    data = port_read(port);
    if (data ==""){
      data = twoDigits(getRTCHour());
    }
    ports_print(data + '\n');
    clock.setHour(data.toInt());
  
    message = "  Min:   [" + twoDigits(getRTCMinute()) + "] >" ;
    ports_print(message);
    data = port_read(port);
    if (data ==""){
      data = twoDigits(getRTCMinute());
    }
    ports_print(data + '\n');
    clock.setMinute(data.toInt());
    clock.setSecond(0);

  }
  
}

/*
 * MENU FUNCTIONS
 */
 
void show_prompt(){
  String message = "";
  message += getRTCTime() + ">";
  //message += ">";
  
  ports_print(message);   
}

String help(){
  String message = "";
  message += "\n---------------------------------------\n";
  message += "HELP:\n You are on your own. Good luck.\n";
  return message;
}

void show_help(){
  String message = "";
  message = help();
  ports_print(message);  
}

String about(){
  String message = "";
  message += "\n---------------------------------------\n";
  message += "ABOUT:\n";
  message += " BlueTrail Version 0.9 -- Part of  \n"; 
  message += " the ParkData(tm) Network.       \n\n";
  message += " A 'touchless' interface to the    \n";
  message += " TRAFx IR trail counter.         \n\n";
  message += " Version: ";
  message += SW_VERSION;
  message += "\n\n";
  message += " Powered by Digame Systems\n";
  message += " www.digamesystems.com\n";
  message += " Copyright 2020, Digame Systems.\n";
  message += " All rights reserved.\n";
  return message;
}

void show_about(){
  String message = "";
  message = about();
  ports_print(message);  
}

String main_menu(){
  String message = "";
  message +="\n---------------------------------------\n";
  message +="MAIN MENU:\n";
  message +=" [A]bout            [R]TC \n";
  message +=" [C]onfigure Sensor [S]tart Logging \n";
  message +=" [D]ownload Data    [H]elp\n";
  message +=" re[B]oot counter\n";
  return message;
}

void show_main_menu(){
  String message = "";
  message = main_menu();
  ports_print(message);
}


/*
 * SERIAL UTILITY FUNCTIONS
 */
void ports_print(String s){
    debugUART.print(s);
    bleUART.print(s);
}
 
void port_print(String s, int port){
  if (port == COM0){
    debugUART.print(s);
  }
  if (port == COM2){
    bleUART.print(s);
  }  
}

String port_read(int port){
  String data = "";
  
  if (port == COM0){
    while (debugUART.available() == 0) {}  
    data = debugUART.readStringUntil('\n');
  }  

  if (port == COM2){
    while (bleUART.available() == 0) {}  
    data = bleUART.readStringUntil('\n');
  }  
  data.trim();
  return data;
}

/* 
 * Blocking read of TRAFX replies for a period of time. 
 * We us this approach since the TRAFx doesn't seem to have a consistent 
 * message terminator... 
 * :/
 */ 
String read_trafx(unsigned long timeout){
  unsigned long t1, t2;
  char c1; 

  String reply = "";

  t1 = millis();
  t2 = t1;

  while ((t2-t1)<timeout){
    if ( trafxUART.available() ) {
      c1 = trafxUART.read();
      reply += c1;
    }
    t2 = millis();
  }

  return reply;
}



/*
 * TRAFX FUNCTIONS
 */

void reboot_counter(){
 reset_trafx();
}


// Wake up the TRAFx board and start pass-through communication.
void reset_trafx(){
  String message ="";

  trafxUART.end(); // Be sure the TX/RX lines are low.
  
 // ports_print("\n---------------------------------------\n");     
  //ports_print("Connecting to Sensor... \n");
  //ports_print("Enter 3 'z's to return to MAIN MENU.\n");
  
  //passthrough_active = true;
  
  // Pull the RESET line on the TRAFx module low for one second.
  digitalWrite(TRAFX_RST, HIGH); //Activates a transistor switch to turn off power to the TrafX module
  delay(1000);                    
  digitalWrite(TRAFX_RST, LOW); //Turn off the switch. (No longer grounded.) Power to the TrafX restored.

  trafxUART.begin(9600, SERIAL_8N1, 25, 33); //Now we can turn on the UART
  
  while (!Serial1) {   
    delay(10); // wait for serial port to connect. Needed for Native USB only
  }

  trafxUART.println("?"); //Invalid command to get its attention
  

}


// Set the time on the TRAFX counter board to match the RTC and set the start time to the top of the 
// next hour.
void set_times(){
  String message = "";

  reset_trafx();// a low battery warning when USB is connected. 
  // Deal with it.
  trafxUART.print('y');
  delay(10);
  message = read_trafx(1000);
  //ports_print(message);
 
  trafxUART.print('c'); // Configure date/times
  delay(10);
  message = read_trafx(1000);
  //ports_print(message);
 
  String dt_string = getRTCTime();  
  
  for (int i=0; i<dt_string.length(); i++){
    trafxUART.print(dt_string[i]);
    trafxUART.flush();
    delay(10);
  }
  trafxUART.print("\r\n");
  
  message = read_trafx(1000);
  //ports_print(message);
  
  String dt_string2 = get_start_date_time();  
  
  for (int i=0; i<dt_string2.length(); i++){
    trafxUART.print(dt_string2[i]);
    trafxUART.flush();
    delay(10);
  }

  trafxUART.print("\r\n");
  message = read_trafx(1000);
  //ports_print(message);
  
  trafxUART.print('y'); // Confirm things look OK.
  message = read_trafx(3000);
  //ports_print(message);
 
  return;
}


// Set the time and start time and launch logging.
// TODO: Make more intelligent to handle errors like low battery, etc...

void start_trafx_logging(){

  set_times(); // Set the time from the RTC and set start time to the top of the next hour
  
  bleUART.print("Activating Logging...\n");
  trafxUART.print('l'); // Issue start logging command
  debugUART.print(read_trafx(2000));

  trafxUART.print('y'); // Confirm settings look ok
  debugUART.print(read_trafx(1000));

  trafxUART.print('y'); // Yes, erase the old data.
  debugUART.print(read_trafx(10000));

  trafxUART.print('y'); // Continue in spite of low battery warning.
  debugUART.print(read_trafx(3000));
  
  ports_print("Logging Active.\n You may remove magnetic switch key.\n");
  
}


void download_trafx_data(){
  String message;
  reset_trafx();
  delay(2000);

  ports_print("DOWNLOAD DATA:\n");
  
  // Sometimes we get a low battery warning when USB is connected. 
  // Deal with it.
  trafxUART.print('y');
  delay(10);
  message = read_trafx(2000);
  //debugUART.print(message);
  
  trafxUART.print('d');
  delay(10);
  message = read_trafx(2000);
  //debugUART.print(message);
  
  trafxUART.print('y');
  delay(10);

  bool downloading = true;

  message = "";
  String foo;
  while (downloading){
  
   foo = trafxUART.readStringUntil('\r');
   ports_print(foo);
   //message +=foo;
   int marker = foo.indexOf("END OF DATA");
   if (marker>-1){
     //ports_print(String(marker));
     //ports_print("Hello! End of data found!");
     //ports_print(message.substring(1,25));
     downloading = false;
   }

  }

  //ports_print(message);
  return;  
}


/*
 * INITIALIZE THE SYSTEM
 */
void setup()
{
  initRTC();
  //Wire.begin();
  
  // Open serial communications and wait for port to open:
  debugUART.begin(9600);
  while (!Serial) {
    delay(10); // wait for serial port to connect. Needed for Native USB only
  }
  debugUART.println("Hello.");

  // Set up the TRAFX communication and reset the module.
  trafxUART.begin(9600, SERIAL_8N1, 25, 33);
  
  while (!Serial1) {   
    delay(10);; // wait for serial port to connect. Needed for Native USB only
  }
  debugUART.println("Hello, Trafx.");

  
  // Set up the BLE communication and reset the module.
  configureBluetooth();
  

  debugUART.println("Hello, BLE.");

  // Initialize the Reset line to TRAFX module
  pinMode(TRAFX_RST, OUTPUT);
  reset_trafx();
  
  debugUART.setTimeout(20000); // Set the Serial receive time out to 20 sec. 
  bleUART.setTimeout(20000);
  
  show_about();
  show_main_menu();
  show_prompt();
  
}


/*
 * COMMAND PROCESSING / STATE MACHINE CONTROL
 */
void process_command(String command, int port){

  ports_print(command);
  
  switch (command[0]) {
    case 'a': // About
      show_about();
      break;
    case 'b': 
      reboot_counter();
      break;
    case 'c':
      passthrough_active = true;
      reset_trafx();
      break;
    case 'd':
      passthrough_active=true;
      download_trafx_data();
      passthrough_active=false;
      break;
    case 'h': // Help
      show_help();
      break;
    case 'r': // Set the RTC
      set_rtc(port);
      break;
    case 's': // Open loop command sequence to start the counter logging data.
      start_trafx_logging();
      break;
    case 't': // Sets counter times to RTC value.
      passthrough_active = true;
      set_times();
      break;
    default:
      break;    
  } 

  if (!passthrough_active){
    show_main_menu(); 
    show_prompt();       
  }

}

// To exit passthrough mode, enter three ':'s. 
void z_check(char c1){
  static int zcounter = 0;
  if (c1 == ':'){
    zcounter ++;
    if (zcounter > 2){
      zcounter = 0;
      ports_print("\n-----------------------------------\n");
      ports_print("Disconnecting from Sensor...\n");
      //digitalWrite(TRAFX_RST, LOW); //Turn off the switch. (No longer grounded.)
      passthrough_active = false;
    }
  }  
}


/*
 * MAIN LOOP
 */
void loop() 
{
  String c0 ="";
  char c1;
  String c2 ="";
 
  if (passthrough_active){ // We are talking direcly to the TRAFx counter.
    // When data comes in from the TRAFx module, route it to 
    // the console / BLE module.  
    if ( trafxUART.available() ) {
      c1 = trafxUART.read();
      debugUART.print(c1);
      bleUART.print(c1);
    }

    //When data comes in over the console, process it.
    if (debugUART.available()){
      c1 = debugUART.read();
      bleUART.print(c1);   // Copy to BLE
      if (c1!='z') trafxUART.print(c1); // Route to TRAFx
      delay(10);
      z_check(c1);
    }
  
    //When data comes in over the BLE link, process it. 
    if (bleUART.available()){
      c1 = bleUART.read();
      debugUART.print(c1); // Copy to Console
      if (c1!='z') trafxUART.print(c1); // Route to TRAFx
      delay(10);
      z_check(c1);
    }
   
      
  } else { //Passthrough mode isn't active. -- Talking to the Arduino.
    
    //When data comes in over the console, process it.
    if (debugUART.available()){
      c0 = debugUART.readStringUntil('\n');
      bleUART.print(c0);
      process_command(c0, COM0);
    }
  
    //When data comes in over the BLE link, process it. 
    
    if (bleUART.available()){
      c2 = bleUART.readStringUntil('\n');
      debugUART.print(c2);
      process_command(c2, COM2);
    }
    
  }
   
}

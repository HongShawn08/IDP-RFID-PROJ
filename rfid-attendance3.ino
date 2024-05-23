
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>

#include<Wire.h>
#include<LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
//---------------------------------------------------------------------------------------------------------
//Acts as a value that remains constant to be applied whenever called for
// Enter Google Script Deployment ID:
const char *GScriptId = "ENTER_DEPLOYMENT_ID";
//---------------------------------------------------------------------------------------------------------
// Enter network credentials:
const char* ssid     = "ENTER_WIFI_SSID";
const char* password = "ENTER_WIFI_PASS";
//---------------------------------------------------------------------------------------------------------
// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";
//---------------------------------------------------------------------------------------------------------
// Google Sheets setup (do not edit)
const char* host        = "script.google.com";
const int   httpsPort   = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;
//------------------------------------------------------------
// Declare variables that will be published to Google Sheets
String student_id;
//------------------------------------------------------------
//Similar to nodes, has value and a pointer. Similar to array in python 
int blocks[] = {4,5,6,8,9};
#define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))
//------------------------------------------------------------
//RST_PIN refers to reset pin for Node MCU, SS_PIN is the unique code in the rfid card, BUZZER is the sound component 
#define RST_PIN  0  //D3
#define SS_PIN   2  //D4
#define BUZZER   4  //D2
//------------------------------------------------------------
//Reciever checks if the unique identifier is valid and also wipes old unique identifier when a new pin is scanned. 
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;
//------------------------------------------------------------
/* Be aware of Sector Trailer Blocks */
int blockNum = 2;  
/* Create another array to read data from Block */
/* Legthn of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];
//------------------------------------------------------------

/****************************************************************************************************
 * setup Function
****************************************************************************************************/
void setup() {
  //----------------------------------------------------------
  //Void is similar to "null," cancels out any output, this function connects to rfid reader, intializes LCD, connects to wifi
  //Sets the data rate in bits per second (baud) for serial data transmission.
  Serial.begin(9600);        
  delay(10);
  Serial.println('\n');
  //----------------------------------------------------------
  //Connects to/initializes connection. Similar to turning on bluetooth
  SPI.begin();
  //----------------------------------------------------------
  //initialize lcd screen
  lcd.init();
  // turn on the backlight
  lcd.backlight();
  lcd.clear();
  //start writign text from (0,0) which is top left of LCD screen.
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("WiFi...");
  //----------------------------------------------------------
  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");
//every 1 second wifi is connected, print period to signal that wifi is working
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  //----------------------------------------------------------
  // Use HTTPSRedirect class to create a new secure connection
  //creates a new secure connection everytime you run function "setup"
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  //----------------------------------------------------------
  //connecting to google to send data to spreadsheet
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Connecting to");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("Google ");
  delay(5000);
  //----------------------------------------------------------
  Serial.print("Connecting to ");
  Serial.println(host);
  //----------------------------------------------------------
  // Try to connect for a maximum of 5 times
  bool flag = false;
  for(int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    //*************************************************
    //Prints on LCD if TLS was connected
    if (retval == 1){
      flag = true;
      String msg = "Connected. OK";
      Serial.println(msg);
      lcd.clear();
      lcd.setCursor(0,0); //col=0 row=0
      lcd.print(msg);
      delay(2000);
      break;
    }
    //*************************************************
    else
      Serial.println("Connection failed. Retrying...");
    //*************************************************
  }
  //----------------------------------------------------------
  if (!flag){
    //____________________________________________
    lcd.clear();
    lcd.setCursor(0,0); //col=0 row=0
    lcd.print("Connection fail");
    //____________________________________________
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    delay(5000);
    return;
    //____________________________________________
  }
  //----------------------------------------------------------
  //deletes the HTTPSRedirect to save storage
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
  //----------------------------------------------------------
}

/****************************************************************************************************
 * loop Function
****************************************************************************************************/
void loop() {
  //----------------------------------------------------------------
  //First section of loop creates invisible folder to store any new RFID data coming in
  static bool flag = false;
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
//If the client does not equal null (has data stored), connect to the secure TLS connection
  if (client != nullptr){
    if (!client->connected())
      {client->connect(host, httpsPort);}
  }
  //if flag never equals true (meaning the folder was never made when it was supposed to), call error
  else{Serial.println("Error creating client object!");}
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Scan your Tag");
  
  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if ( ! mfrc522.PICC_IsNewCardPresent()) {return;}
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) {return;}
  /* Read data from the same block */
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));  
  //----------------------------------------------------------------
  String values = "", data;
 //-----------------------------------------------------------------
  
  //creating payload - method 2 - More efficient
  //Purpose of this part of code is to convert the blocks data into the actual student ID which will eventually be stored in the payload.
  
//------------------------------------------------------------------------------------------------------------------------------------------
  
  //byte is a data type that denotes size in memory
  //This loop takes data stored within the block one by one in order of the block, and puts it into the payload.
  //ex: first time: i=0. Second time: i=1. Third time: i=2
  //Because C++ is 0-indexed, this for loop will include all data in the blocks even if the loop runs to LESS THAN the total amount of blocks
  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData); 
    if(i == 0){
      //forcing block data into string, data is the result of that
      data = String((char*)readBlockData);
      data.trim();
      student_id = data;
      values = "\"" + data + ",";
    }
    else if(i == total_blocks-1){
      data = String((char*)readBlockData);
      data.trim();
      values += data + "\"}";
    }
    else{
      data = String((char*)readBlockData);
      data.trim();
      values += data + ",";
    }
  }
  //----------------------------------------------------------------
  // Create json object string to send to Google Sheets (bascially makes the payload into a json file)
  // values = "\"" + value0 + "," + value1 + "," + value2 + "\"}"
  payload = payload_base + values;
  //----------------------------------------------------------------
  lcd.clear();
  lcd.setCursor(0,0); //col=0 row=0
  lcd.print("Publishing Data");
  lcd.setCursor(0,1); //col=0 row=0
  lcd.print("Please Wait...");
  //----------------------------------------------------------------
  // Publish data to Google Sheets (the data is all from the if statement from earlier)
  Serial.println("Publishing data...");
  Serial.println(payload);
  if(client->POST(url, host, payload)){ 
    // do stuff here if publish was successful
    lcd.clear();
    lcd.setCursor(0,0); //col=0 row=0
    lcd.print("Student ID: "+student_id);
    lcd.setCursor(0,1); //col=0 row=0
    lcd.print("Present");
  }
  //----------------------------------------------------------------
  else{
    // do stuff here if publish was not successful
    Serial.println("Error while connecting");
    lcd.clear();
    lcd.setCursor(0,0); //col=0 row=0
    lcd.print("Failed.");
    lcd.setCursor(0,1); //col=0 row=0
    lcd.print("Try Again");
  }
  //----------------------------------------------------------------
  // a delay of several seconds is required before publishing again    
  delay(5000);
}

/****************************************************************************************************
 * 
****************************************************************************************************/
/****************************************************************************************************
 * ReadDataFromBlock() function
 ****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  //----------------------------------------------------------------------------
  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //----------------------------------------------------------------------------
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  //----------------------------------------------------------------------------s
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  //----------------------------------------------------------------------------
  else {
    Serial.println("Authentication success");
  }
  //----------------------------------------------------------------------------
  /* Reading data from the Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  //----------------------------------------------------------------------------
  else {
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");  
  }
  //----------------------------------------------------------------------------
}

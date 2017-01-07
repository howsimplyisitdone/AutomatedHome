#include <SoftwareSerial.h>
 
#define DEBUG true
 
SoftwareSerial esp8266(4,5); // make RX Arduino line is pin 2, make TX Arduino line is pin 3.
                             // This means that you need to connect the TX line from the esp to the Arduino's pin 2
                             // and the RX line from the esp to the Arduino's pin 3
void setup()
{
  Serial.begin(115200);
  esp8266.begin(115200); // your esp's baud rate might be different
  Serial.println("Connection @ 115200");
  
  pinMode(11,OUTPUT);
  digitalWrite(11,LOW);
  Serial.println("Pin 11 set to LOW");
  
  pinMode(12,OUTPUT);
  digitalWrite(12,LOW);
  Serial.println("Pin 12 set to LOW");
  
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  Serial.println("Pin 13 set to LOW");
  
  pinMode(10,OUTPUT);
  digitalWrite(10,LOW);
  Serial.println("Pin 10 set to LOW");
   
  sendCommand("AT+RST\r\n",3000,DEBUG); // reset module
  Serial.print(esp8266.read());
  sendCommand("AT+CWMODE=1\r\n",3000,DEBUG); // configure as access point
  Serial.print(esp8266.read());
  sendCommand("AT+CWQAP\r\n",3000,DEBUG);
  Serial.print(esp8266.read());
  sendCommand("AT+CWJAP=\"<add AP here>\",\"<add AP password here>\"\r\n",8000,DEBUG);
  Serial.print(esp8266.read());
  delay(10000);
  sendCommand("AT+CIFSR\r\n",3000,DEBUG); // get ip address
  Serial.print(esp8266.read());
  sendCommand("AT+CIPMUX=1\r\n",3000,DEBUG); // configure for multiple connections
  Serial.print(esp8266.read());
  sendCommand("AT+CIPSERVER=1,80\r\n",3000,DEBUG); // turn on server on port 80
  Serial.print(esp8266.read());
  Serial.println("Server Ready");
}
 
void loop()
{
  if(esp8266.available()) // check if the esp is sending a message 
  {
 
    Serial.println(esp8266.readStringUntil('\n'));
    
    if(esp8266.find("+IPD,"))
    {
     delay(1000); // wait for the serial buffer to fill up (read all the serial data)
     // get the connection id so that we can then disconnect
     int connectionId = esp8266.read()-48; // subtract 48 because the read() function returns 
                                           // the ASCII decimal value and 0 (the first decimal number) starts at 48
     //Serial.println(esp8266.readStringUntil('\n'));
          
     esp8266.find("pin"); // advance cursor to "pin"
     int inData = esp8266.parseInt();
     Serial.println(esp8266.readStringUntil('\n'));
     //Serial.println("Got reponse from ESP8266: " + inData);
     //String pincode = inData;
     int pinNumber = 0;
     
     pinNumber = inData + 10;
     
     //int pinNumber = (esp8266.read()-48); // get first number i.e. if the pin 13 then the 1st number is 1
     //int secondNumber = (esp8266.read()-48);
     //if(secondNumber>=0 && secondNumber<=9)
     //{
     // pinNumber*=10;
     // pinNumber +=secondNumber; // get second number, i.e. if the pin number is 13 then the 2nd number is 3, then add to the first number
     //}

     if (pinNumber > 10)
     {
       digitalWrite(pinNumber, !digitalRead(pinNumber)); // toggle pin    
       
       // build string that is send back to device that is requesting pin toggle
       String content;
       content = "Pin ";
       content += pinNumber;
       content += " is ";
  
       if(digitalRead(pinNumber))
       {
         content += "ON";
       }
       else
       {
         content += "OFF";
       }
       sendHTTPResponse(connectionId,content);
     }
     else
     {
     // build string that is send back to device that is requesting pin toggle
       String content;
       content = "Pin ";
       content += pinNumber;
       content += " is undefined";
       sendHTTPResponse(connectionId,content);
     }
     
     // make close command
     String closeCommand = "AT+CIPCLOSE="; 
     closeCommand+=connectionId; // append connection id
     closeCommand+="\r\n";
         
     sendCommand(closeCommand,1000,DEBUG); // close connection
    }
  }
}
 
/*
* Name: sendData
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    
    int dataSize = command.length();
    char data[dataSize];
    command.toCharArray(data,dataSize);
           
    esp8266.write(data,dataSize); // send the read character to the esp8266
    if(debug)
    {
      Serial.println("\r\n====== HTTP Response From Arduino ======");
      Serial.write(data,dataSize);
      Serial.println("\r\n========================================");
    }
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(esp8266.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = esp8266.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(debug)
        {
      Serial.print(response);
    }
    
    return response;
}
 
/*
* Name: sendHTTPResponse
* Description: Function that sends HTTP 200, HTML UTF-8 response
*/
void sendHTTPResponse(int connectionId, String content)
{
     
     // build HTTP response
     String httpResponse;
     String httpHeader;
     // HTTP Header
     httpHeader = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n"; 
     httpHeader += "Content-Length: ";
     httpHeader += content.length();
     httpHeader += "\r\n";
     httpHeader +="Connection: close\r\n\r\n";
     httpResponse = httpHeader + content + " "; // There is a bug in this code: the last character of "content" is not sent, I cheated by adding this extra space
     sendCIPData(connectionId,httpResponse);
}
 
/*
* Name: sendCIPDATA
* Description: sends a CIPSEND=<connectionId>,<data> command
*
*/
void sendCIPData(int connectionId, String data)
{
   String cipSend = "AT+CIPSEND=";
   cipSend += connectionId;
   cipSend += ",";
   cipSend +=data.length();
   cipSend +="\r\n";
   sendCommand(cipSend,1000,DEBUG);
   sendData(data,1000,DEBUG);
}
/*
* Name: sendCommand
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendCommand(String command, const int timeout, boolean debug)
{
    String response = "";
           
    esp8266.print(command); // send the read character to the esp8266
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(esp8266.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = esp8266.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(debug)
    {
      Serial.print(response);
    }
    
    return response;
}

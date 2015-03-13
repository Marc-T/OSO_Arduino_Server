#include <SPI.h>
#include <Ethernet.h>

//Inputs
byte INmsSalon = 2;
byte INmsGarage = 3;
byte INswArm = 5;
byte INswDisarm = 6;

//Outputs
byte OUTAlarm = 9;

//Ethernet Vars
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xD3, 0xF1 };
IPAddress ip( 192,168,100,253 );
IPAddress gateway( 192,168,100,1 );
IPAddress subnet( 255,255,255,0 );
EthernetServer server(80);

//Vars
boolean Armed = false; // Etat de la protection : 0 Désactivée / 1 Activée;
boolean Silenced = false;
boolean Fired = false;
byte RelayArmState = 0;
byte RelayDisarmState = 0;
unsigned long Fired_at = 0;
unsigned long Action_at = 0;

//Qry 
String Qry;
String Action;
byte MaxCar = 25;

//Beep
byte BeepDuration = 750;
unsigned int AlarmDuration = 900000;

//MotionSensor
unsigned int CalibDuration = 10000;


void setup() {
  
  Serial.begin(9600);
  
  pinMode(INmsSalon, INPUT_PULLUP);
  pinMode(INmsGarage, INPUT_PULLUP);
  pinMode(INswArm, INPUT_PULLUP);
  pinMode(INswDisarm, INPUT_PULLUP);
  pinMode(OUTAlarm, OUTPUT);

  Serial.println();
  Serial.print(F("Etalonnage du detecteur de mouvement ..."));
  delay(CalibDuration);
  Serial.println(F("ok"));
  
  Serial.print(F("Lecture etat des relais ..."));  
  RelayArmState = digitalRead(INswArm);
  RelayDisarmState = digitalRead(INswDisarm);
  Serial.println(F("ok"));
  
  Serial.print(F("Demarrage module ethernet ..."));
  Ethernet.begin(mac, ip, gateway, gateway, subnet);

  Serial.println(Ethernet.localIP());
          
  Serial.println(F("Pret"));
  
  Qry.reserve(MaxCar);
  Action.reserve(MaxCar);
  
  Action_at = millis();
}


void loop() {
  if (digitalRead(INswArm) != RelayArmState) // Arme le système
  {
    RelayArmState = !RelayArmState; 
    Arm(false); 
  }
  if (digitalRead(INswDisarm) != RelayDisarmState) // Désarme le système
  {
    RelayDisarmState = !RelayDisarmState; 
    Disarm();
  }
  
  if (Armed) // Si la protection est activée
  { 
    if(Fired) // L'alarme a déjà été déclenchée
    {
      if ((millis() - Fired_at) > AlarmDuration) // Coupe l'alarme apres 15 minutes
      {
        Serial.println(F("Timeout alarme stoppee!"));
        Alarm(LOW);
        Fired = false;
      }
    }
    if(digitalRead(INmsSalon) || digitalRead(INmsGarage)) // Intrusion détéctée
    {
      if(!Fired) // Nouvelle Alarme
      {
        Serial.println(F("Intrusion detectee, declenchement alarme !"));
        Alarm(HIGH);
        Fired_at = millis();
        Fired = true;
        // Envois alerte serveur web
     }
    }
  }  
  
  EthernetClient client = server.available();
  if (client) {
    Serial.println(F("Client connecte"));

    boolean currentLineIsBlank = true;
    Qry = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (Qry.length() < MaxCar)
        {
          Qry += c;          
        }

        if (c == '\n' && currentLineIsBlank) {
          Action = Qry.substring(Qry.indexOf("GET")+5, Qry.indexOf(" HTTP"));
          Serial.println(Action);
          
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/plain"));
          client.println(F("Connection: close"));
          client.println();
          
          if(Action == "GetState")
          {       
            if (Armed)
            {
                if(Silenced)
                {
                  if(Fired)
                  {
                    client.println(F("4"));
                  }
                  else
                  {
                    client.println(F("3"));
                  }
                }
                else
                {
                  if(Fired)
                  {
                    client.println(F("2"));
                  }
                  else
                  {
                    client.println(F("1"));
                  }
                }
            }
            else
            {
               client.println(F("0"));
            }
          }
          else if(Action == "Arm")
          {
            Arm(false);
          }
          else if(Action == "Disarm")
          {
            Disarm();
          }
          else if(Action == "SilencedArm")
          {
            Arm(true);
          }
          break;
        }
        
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }

    // close the connection:
    client.stop();
    Serial.println(F("client deco"));
  }
}

void beep()
{
  Serial.println(F("beep")); 
  delay(BeepDuration);
  Alarm(HIGH);
  delay(BeepDuration);
  Alarm(LOW);
}

void Arm(boolean Sil)
{
  Silenced = Sil;
  beep();
  Armed = true;
  Serial.println(F("Systeme arme"));
}

void Disarm()
{
  beep();
  beep();  
  Armed = false;
  Fired = false;
  Silenced = false;
  Serial.println(F("Systeme desarme"));
}

void Alarm(boolean State)
{
  if(Silenced)
    return;
 
  digitalWrite(OUTAlarm, State); 
}

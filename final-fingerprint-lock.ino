
#include <Adafruit_Fingerprint.h>


#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)

SoftwareSerial mySerial(2, 3);

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);


// defining pins
byte enroll_fng_btn = 8;
byte delete_fng_btn = 7;
byte red_led = 9;
byte green_led = 10;
byte relay = 5;
byte buzzer = 13;
byte id = 0;
//defining button state
byte enrl_btnState = 0;
byte del_btnState = 0;
byte lock_count=0;

void setup()
{
  // setting up pins
  pinMode(enroll_fng_btn, INPUT);
  pinMode(delete_fng_btn, INPUT);
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(buzzer, OUTPUT);

  welcometone();
  // fingerprint-sensor configuration
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();
  id = finger.templateCount;
  if (id == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(id); Serial.println(" templates");
  }
}

void loop()                     // run over and over again
{
  getFingerprintID();
  new_enroll();
  del_user();
  delay(50);            //don't ned to run this at full speed.
  if(lock_count==3)
  {
    dangertone();
    lock_count=0;
  }
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
    errortone();
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
    errortone();
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      errortone();
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      errortone();
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      errortone();
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      errortone();
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      errortone();
      return p;
    default:
      Serial.println("Unknown error");
      errortone();
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    lock_count=0;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    errortone();
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    lock_count++;
    errortone();
    return p;
  } else {
    Serial.println("Unknown error");
    errortone();
    return p;
  }

  sucesstone();
  // found a match!
  unlock();
  sucesstone();
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

void unlock()
{
  digitalWrite(green_led, HIGH);
  digitalWrite(red_led,LOW);
  digitalWrite(relay, LOW);
  delay(3000);
  digitalWrite(relay, HIGH);
  digitalWrite(green_led, LOW);
  digitalWrite(red_led,HIGH);
}

void dangertone()
{
  byte i = 0;
  do{
      tone(buzzer, 500);
      delay(1500);
      noTone(buzzer);
      delay(500);
      i++;
    }while(i<5);
}

void sucesstone()
{
      tone(buzzer, 500);
      delay(600);
      noTone(buzzer);
      delay(100);
      tone(buzzer, 400);
      delay(300);
      noTone(buzzer);
}

void errortone()
{
   tone(buzzer, 500);
   delay(2000);
   noTone(buzzer);
}

void welcometone()
{
  tone(buzzer, 400);
  delay(300);
  tone(buzzer, 300);
  delay(400);
  tone(buzzer, 500);
  delay(1000);
  noTone(buzzer);
}

void new_enroll()
{
  enrl_btnState = digitalRead(enroll_fng_btn);
  if(enrl_btnState == HIGH)
  {
    Serial.println("Enrolling New Finger");
    id++;
    byte i = 0, j = 0;
    do{
      Serial.println("Verifying Admin...");
      delay(3000);
      if(getfingerid()==1)
      {
        break;
      }
      errortone();
      j++;
    }while(j<3);
    if(j<3)
    {
      Serial.println("Admin Verified");
       sucesstone();
      do{
        Serial.println('Try enrolling...');
        delay(3000);
          if(getFingerprintEnroll())
          {
            break;  
          }
          errortone();
          i++;
        }while(i<3);
      if(i == 3)
      {
        Serial.println("Can not enroll now, Try again later");
        dangertone();
        id--;  
      }
    }
    else
    {
      id--;
      Serial.println("Can not Verify Admin Try Again Later...");
      dangertone();
    }
  }
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  Serial.println("Remove finger");
  sucesstone();
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  sucesstone();
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    errortone();
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    errortone();
    return p;
  } else {
    Serial.println("Unknown error");
    errortone();
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    errortone();
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    errortone();
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    errortone();
    Serial.println("Error writing to flash");
    return p;
  } else {
    errortone();
    Serial.println("Unknown error");
    return p;
  }

  sucesstone();
  return true;
}

void del_user()
{
  del_btnState = digitalRead(delete_fng_btn);
  if(del_btnState == HIGH)
  {
    byte i=0, j=0;
    Serial.println("Verifying admin for deleting user");
    do{
      delay(3000);
        if(getfingerid()==1)
        {
          break;  
        }
        errortone();
        i++;
      }while(i<3);
     if(i<3)
     {
       Serial.println("Admin verified");
      sucesstone();
       do{
         delay(2000);
         Serial.println("Try deleting");
        uint8_t uid = 0;
        uid=getfingerid();
        if(uid==1)
        {
          return ;
        }
        else{
          uint8_t f = -1;
          Serial.println("finger found with id="+uid);
          f = finger.deleteModel(uid);
        
          if (f == FINGERPRINT_OK && uid!=-1) {
            sucesstone();
            Serial.println("Deleted!");
            break;
          } else if (f == FINGERPRINT_PACKETRECIEVEERR) {
            Serial.println("Communication error");
          } else if (f == FINGERPRINT_BADLOCATION) {
            Serial.println("Could not delete in that location");
          } else if (f == FINGERPRINT_FLASHERR) {
            Serial.println("Error writing to flash");
          } else {
            Serial.print("Unknown error: 0x"); Serial.println(f, HEX);
          }
        }
        errortone();
        j++;
       }while(j<3);
       if(j==3)
       {
         dangertone();
       }
     }
     else
     {
      dangertone();
     }
  }
}
uint8_t getfingerid() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
    errortone();
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
    errortone();
      Serial.println("Imaging error");
      return p;
    default:
    errortone();
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
    errortone();
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
    errortone();
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
    errortone();
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
    errortone();
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    errortone();
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    errortone();
    return p;
  } else {
    Serial.println("Unknown error");
    errortone();
    return p;
  }
  // found a match!
  Serial.println("finger id: "+finger.fingerID);
  return finger.fingerID;
}

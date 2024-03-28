#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OpenDrop.h>
#include <OpenDropAudio.h>
#include "hardware_def.h"

// OpenDrop setup
OpenDrop OpenDropDevice = OpenDrop(); 
Drop *myDrop1 = OpenDropDevice.getDrop(); // DROP1
Drop *myDrop2 = OpenDropDevice.getDrop(); // DROP2

// Fluxel and control arrays
bool FluxCom[20][8];
bool FluxBack[20][8];
int ControlBytesIn[30];
int ControlBytesOut[24];
int readbyte;
int writebyte;

// Joystick and counter variables
int JOY_value;
int joy_x, joy_y;
int x, y;
int del_counter = 0;
int del_counter2 = 0;

// Switch states and idle flag
bool SWITCH_state = true;
bool SWITCH_state2 = true;
bool idle = true;

// Magnet states
bool Magnet1_state = false;
bool Magnet2_state = false;

void setup() {
  Serial.begin(115200);

  // OpenDrop initialization
  char myString[] = "c10";
  OpenDropDevice.begin(myString);
  ControlBytesOut[23] = OpenDropDevice.get_ID();
  Serial.println(OpenDropDevice.get_ID());
  OpenDropDevice.set_Fluxels(FluxCom);

  // Joystick and audio setup
  pinMode(JOY_pin, INPUT);
  OpenDropAudio.begin(16000);
  OpenDropAudio.playMe(2);
  delay(2000);

  // Drive fluxels and update display
  OpenDropDevice.drive_Fluxels();
  OpenDropDevice.update_Display();
  Serial.println("Welcome to OpenDrop");

  // Initialize droplets at specific positions
  myDrop1->begin(0, 2); // initialize DROP1 at position (0,2)
  myDrop2->begin(0, 5); // initialize DROP2 at position (0,5)

  OpenDropDevice.update();
  del_counter = millis();
}

void loop() {
  // Communication with the app
  if (Serial.available() > 0) {
    readbyte = Serial.read();
    if (x < FluxlPad_width)
      for (y = 0; y < 8; y++)
        FluxCom[x][y] = (((readbyte) >> (y)) & 0x01);
    else
      ControlBytesIn[x - FluxlPad_width] = readbyte;

    x++;
    digitalWrite(LED_Rx_pin, HIGH);
    if (x == (FluxlPad_width + 16)) {
      // Update fluxels and display based on received data
      OpenDropDevice.set_Fluxels(FluxCom);
      OpenDropDevice.drive_Fluxels();
      OpenDropDevice.update_Display();

      // Control magnets based on input
      if ((ControlBytesIn[0] & 0x2) && (Magnet1_state == false)) {
        Magnet1_state = true;
        OpenDropDevice.set_Magnet(0, HIGH);
      };
      if (!(ControlBytesIn[0] & 0x2) && (Magnet1_state == true)) {
        Magnet1_state = false;
        OpenDropDevice.set_Magnet(0, LOW);
      };

      if ((ControlBytesIn[0] & 0x1) && (Magnet2_state == false)) {
        Magnet2_state = true;
        OpenDropDevice.set_Magnet(1, HIGH);
      };
      if (!(ControlBytesIn[0] & 0x1) && (Magnet2_state == true)) {
        Magnet2_state = false;
        OpenDropDevice.set_Magnet(1, LOW);
      };

      // Prepare data for transmission
      for (int x = 0; x < FluxlPad_width; x++) {
        writebyte = 0;
        for (int y = 0; y < FluxlPad_heigth; y++)
          writebyte = (writebyte << 1) + (int)OpenDropDevice.get_Fluxel(x, y);
        ControlBytesOut[x] = writebyte;
      }

      // Set temperature values
      OpenDropDevice.set_Temp_1(ControlBytesIn[10]);
      OpenDropDevice.set_Temp_2(ControlBytesIn[11]);
      OpenDropDevice.set_Temp_3(ControlBytesIn[12]);

      // Display feedback
      OpenDropDevice.show_feedback(ControlBytesIn[8]);

      // Get temperature values
      ControlBytesOut[17] = OpenDropDevice.get_Temp_L_1();
      ControlBytesOut[18] = OpenDropDevice.get_Temp_H_1();
      ControlBytesOut[19] = OpenDropDevice.get_Temp_L_2();
      ControlBytesOut[20] = OpenDropDevice.get_Temp_H_2();
      ControlBytesOut[21] = OpenDropDevice.get_Temp_L_3();
      ControlBytesOut[22] = OpenDropDevice.get_Temp_H_3();

      // Transmit data back to the app
      for (x = 0; x < 24; x++)
        Serial.write(ControlBytesOut[x]);
      x = 0;
    };
  } else
    digitalWrite(LED_Rx_pin, LOW);
  del_counter--;

  // Update display periodically
  if (millis() - del_counter > 2000) {
    OpenDropDevice.update_Display();
    del_counter = millis();
  }

  // Check switch states
  SWITCH_state = digitalRead(SW1_pin);
  SWITCH_state2 = digitalRead(SW2_pin);

  // Activate Menu if SWITCH_state is false
  if (!SWITCH_state) {
    OpenDropAudio.playMe(1);
    Menu(OpenDropDevice);
    OpenDropDevice.update_Display();
    del_counter2 = 200;
  }

  // Activate Reservoirs if SWITCH_state2 is false
  if (!SWITCH_state2) {
    if ((myDrop1->position_x() == 15) && (myDrop1->position_y() == 3)) {
      myDrop1->begin(14, 1);
      OpenDropDevice.dispense(1, 1200);
    }
    if ((myDrop1->position_x() == 15) && (myDrop1->position_y() == 4)) {
      myDrop1->begin(14, 6);
      OpenDropDevice.dispense(2, 1200);
    }

    if ((myDrop2->position_x() == 0) && (myDrop2->position_y() == 3)) {
      myDrop2->begin(1, 1);
      OpenDropDevice.dispense(3, 1200);
    }
    if ((myDrop2->position_x() == 0) && (myDrop2->position_y() == 4)) {
      myDrop2->begin(1, 6);
      OpenDropDevice.dispense(4, 1200);
    }

  }

  // Joystick control for both droplets
  JOY_value = analogRead(JOY_pin);

  if ((JOY_value < 950) && (del_counter2 == 0)) {
    if (JOY_value < 256) {
      myDrop1->move_right();
      myDrop2->move_right();
    }

    if ((JOY_value > 725) && (JOY_value < 895)) {
      myDrop1->move_up();
      myDrop2->move_up();
    }

    if ((JOY_value > 597) && (JOY_value < 725)) {
      myDrop1->move_left();
      myDrop2->move_left();
    }

    if ((JOY_value > 256) && (JOY_value < 597)) {
      myDrop1->move_down();
      myDrop2->move_down();
    }

    OpenDropDevice.update_Drops();
    OpenDropDevice.update();
    if (idle) {
      del_counter2 = 1800;
      idle = false;
    } else
      del_counter2 = 400;
  }

  if (JOY_value > 950) {
    del_counter2 = 0;
    idle = true;
  }
  if (del_counter2 > 0)
    del_counter2--;
}

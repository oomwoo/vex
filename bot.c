#pragma config(UART_Usage, UART1, uartUserControl, baudRate115200, IOPins, None, None)
#pragma config(Sensor, dgtl1,  BASE_PWR_1,     sensorDigitalIn)
#pragma config(Sensor, dgtl2,  BASE_PWR_2,     sensorDigitalIn)
#pragma config(Sensor, dgtl3,  BASE_PWR_3,     sensorDigitalIn)
#pragma config(Sensor, dgtl4,  BASE_PWR_4,     sensorDigitalIn)
#pragma config(Sensor, dgtl5,  BASE_PWR_5,     sensorDigitalIn)
#pragma config(Sensor, dgtl6,  BASE_PWR_6,     sensorDigitalIn)
#pragma config(Sensor, dgtl8,  GREEN,          sensorLEDtoVCC)
#pragma config(Sensor, dgtl12, STOP_BUTTON,    sensorDigitalIn)
#pragma config(Motor,  port1,           RIGHT,         tmotorVex393_HBridge, openLoop, reversed, driveLeft)
#pragma config(Motor,  port10,          LEFT,          tmotorVex393_HBridge, openLoop, driveRight)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX)

#include "Vex_Competition_Includes.c"   // Main competition background code...do not modify!
#include "UART_Comm_Link_Includes.c"    // Make VEX Cortex UART talk to Raspberry Pi

// Operate a VEX EDR robot manually and autonomously
// The robot must be connected to a properly-configured Raspberry Pi
// See more at https://github.com/oomwoo/
//
// Copyright (C) 2016 oomwoo.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License version 3.0
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License <http://www.gnu.org/licenses/> for details.

enum UserCommand
{
	USER_CMD_DRIVE_FORWARD,
	USER_CMD_TURN_LEFT,
	USER_CMD_TURN_RIGHT,
	USER_CMD_DRIVE_BACKWARD,
	USER_CMD_NONE
};

const short JOY_MIN_VAL = 64;
const short LED_BLINK_PERIOD = 10;
const short LEFT_MOTOR_PWR_ADJ = -10; // Negative value only
const short RIGHT_MOTOR_PWR_ADJ = 0; // Negative value only

short lcdView = 1;
short lcdViewPrev = 0;
short joyFwdBack = 0;
short joyLeftRight = 0;
short joySpin = 0;
short bmLeft = 0;
short bmRight = 0;
short base_motor_pwr = 127;
UserCommand UserCmd = USER_CMD_NONE;
LinkCommand LinkCmd = LINK_CMD_NONE;
short green_led_blink_counter = 0;
short base_pwr_table[] = {50, 60, 70, 80, 90, 100, 110}; // Selected by jumper

bool stop_button_pressed = false;
bool forget = false;
bool humanControl = true;
bool recording = false;
bool suppress_user_command;

char uartCmd, uartValue;

short isJumperInserted(tSensors port)
{
	return (1 - SensorValue[port]);
}

short GetBasePower()
{
	short idx;
	
	if (isJumperInserted(BASE_PWR_1))
		idx = 0;
	else if (isJumperInserted(BASE_PWR_2))
		idx = 1;
	else if (isJumperInserted(BASE_PWR_3))
		idx = 2;
	else if (isJumperInserted(BASE_PWR_4))
		idx = 3;
	else if (isJumperInserted(BASE_PWR_5))
		idx = 4;
	else if (isJumperInserted(BASE_PWR_5))
		idx = 5;
	else
		idx = 6;

	return idx;
}

void SetGreenLed(bool on, bool blink)
{
	if (!on)
	{
		green_led_blink_counter = LED_BLINK_PERIOD;
		SensorValue[GREEN] = false;
		return;
	}

	if (blink)
	{
		green_led_blink_counter = green_led_blink_counter - 1;
		if (green_led_blink_counter <= 0)
		{
			SensorValue[GREEN] = !SensorValue[GREEN];
			green_led_blink_counter = LED_BLINK_PERIOD;
		}
	}
	else
		SensorValue[GREEN] = true;
}

bool isButtonPressed(tSensors port)
{
	return (bool) (1 - SensorValue[port]);
}

UserCommand JoystickToCommand(short joyFwdBack, short joyLeftRight, short joySpin)
{

	if (joyFwdBack > JOY_MIN_VAL)
	{
		return USER_CMD_DRIVE_FORWARD;
	}
	else if (joyLeftRight > JOY_MIN_VAL)
	{
		return USER_CMD_TURN_RIGHT;
	}
	else if (joyLeftRight < -JOY_MIN_VAL)
	{
		return USER_CMD_TURN_LEFT;
	}
	else if (joyFwdBack < -JOY_MIN_VAL)
	{
		return USER_CMD_DRIVE_BACKWARD;
	}
	else
		return USER_CMD_NONE;
}

void CommandToBaseMotorPower(short UserCmd, short* bmLeft, short* bmRight)
{
	switch (UserCmd)
	{
		case USER_CMD_DRIVE_FORWARD:
			*bmLeft = (base_motor_pwr + LEFT_MOTOR_PWR_ADJ);
			*bmRight = (base_motor_pwr + RIGHT_MOTOR_PWR_ADJ);
			break;
		case USER_CMD_DRIVE_BACKWARD:
			*bmLeft = -(base_motor_pwr + LEFT_MOTOR_PWR_ADJ);
			*bmRight = -(base_motor_pwr + RIGHT_MOTOR_PWR_ADJ);
			break;
		case USER_CMD_TURN_RIGHT:
			*bmLeft = base_pwr_table[0];
			*bmRight = 0;
			break;
		case USER_CMD_TURN_LEFT:
			*bmLeft = 0;
			*bmRight = base_pwr_table[0];
			break;
		default:
			*bmLeft = 0;
			*bmRight = 0;
	}
}

void UpdateLCD()
{
	string mainBattery;

	if (nLCDButtons != 0)
		lcdView = nLCDButtons;

	switch (lcdView) {
	case 1:
		if (lcdView != lcdViewPrev)
		{
			displayLCDString(0, 0, "u   F   P   H   ");
			displayLCDString(1, 0, "    V       L   ");
		}
		//Display command, who has control
		displayLCDNumber(0, 1, UserCmd, 1);
		displayLCDNumber(0, 5, forget, 1);
		displayLCDNumber(0, 9, stop_button_pressed, 1);
		displayLCDNumber(0, 13, humanControl, 1);
		displayLCDNumber(1, 13, LinkCmd, 2);

		// Display battery voltage
		sprintf(mainBattery, "%1.2f", nImmediateBatteryLevel/1000.0);
		displayLCDString(1, 0, mainBattery);
		break;
	case 2:
		// UART vars
		if (lcdView != lcdViewPrev)
		{
			displayLCDString(0, 0, "Tx              ");
			displayLCDString(1, 0, "Rx              ");
		}
		displayLCDNumber(0, 3, nXmitChars, 10);
		displayLCDNumber(1, 3, nRecvChars, 10);
		break;
	case 4:
		if (lcdView != lcdViewPrev)
		{
			displayLCDString(0, 0, "F    T     S    ");
			displayLCDString(1, 0, "L       R       ");
		}
		displayLCDNumber(0, 1, joyFwdBack, 4);
		displayLCDNumber(0, 6, joyLeftRight, 4);
		displayLCDNumber(0, 12, joySpin, 4);
		displayLCDNumber(1, 2, bmLeft, 4);
		displayLCDNumber(1, 10, bmRight, 4);
		break;
	}
	lcdViewPrev = lcdView;
}

void Init()
{
	bLCDBacklight = true;				// Turn on LCD Backlight
	lcdViewPrev = 0; 						// Force full LCD update
	humanControl = true;
	nCommPort = uartOne;				// Specify UART to talk to Raspberry Pi

	short idx = GetBasePower();
	base_motor_pwr = base_pwr_table[idx];
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                          Pre-Autonomous Functions
//
// You may want to perform some actions before the competition starts. Do them in the
// following function.
//
/////////////////////////////////////////////////////////////////////////////////////////

void pre_auton()
{
	// Set bStopTasksBetweenModes to false if you want to keep user created tasks running between
	// Autonomous and Tele-Op modes. You will need to manage all user created tasks if set to false.
	bStopTasksBetweenModes = true;

	// All activities that occur before the competition starts
	// Example: clearing encoders, setting servo positions, ...
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                                 User Control Task
//
// This task is used to control your robot during the user control phase of a VEX Competition.
// You must modify the code to add your own robot specific commands here.
//
/////////////////////////////////////////////////////////////////////////////////////////

void UserControlFunction()
{
	// User control code here, inside the loop

	Init();

	while(true) {

		if (isButtonPressed(STOP_BUTTON))
			stop_button_pressed = true;

		////////////////////////////////////////////////////////////////////////////
		// User command
		////////////////////////////////////////////////////////////////////////////
		joyLeftRight = vexRT[Ch1];
		joyFwdBack = vexRT[Ch2];
		joySpin = vexRT[Ch4];
		UserCmd = JoystickToCommand(joyFwdBack, joyLeftRight, joySpin);

		// Helps recording back-out-of-dead-end situations
		suppress_user_command = vexRT[Btn5U] != 0;

		////////////////////////////////////////////////////////////////////////////
		// Control Raspberry Pi
		////////////////////////////////////////////////////////////////////////////

		LinkCmd = LINK_CMD_NONE;
		forget = false;

		if (vexRT[Btn8R])
		{
			// Forget last few seconds of training (if the human operator made a mistake)
			forget = true;
			LinkCmd = LINK_CMD_FORGET;
		}
		else if (vexRT[Btn7R])
		{
			// Have Raspberry Pi start capturing video and user commands (joystick, buttons, etc.)
			LinkCmd = LINK_CMD_START_RECORDING;
			recording = true;
		}
		else if (vexRT[Btn7L])
		{
			// Have Raspberry Pi stop capturing video, user commands (joystick, buttons, etc.)
			LinkCmd = LINK_CMD_STOP_RECORDING;
			recording = false;
		}
		else if (vexRT[Btn7U])
		{
			// Transfer control from human operator to robot (autonomous control)
			humanControl = false;
			LinkCmd = LINK_CMD_AUTONOMOUS_CONTROL;
		}
		else if (vexRT[Btn8U])
		{
			// End training successfully - Raspberry Pi disconnects and uploads captured data
			LinkCmd = LINK_CMD_TERMINATE_AND_UPLOAD;
		}
		else if (vexRT[Btn8D])
		{
			// Abort training - Raspberry Pi disconnects, does not upload captured data
			LinkCmd = LINK_CMD_TERMINATE;
		}
		else if (vexRT[Btn7D])
		{
			// Highest priority command
			// Transfer control from robot to the human operator (manual control)
			humanControl = true;
			LinkCmd = LINK_CMD_MANUAL_CONTROL;
		}

		// Send command(s) to Raspberry Pi
		bool send_user_command = UserCmd != USER_CMD_NONE && !suppress_user_command;
		if (LinkCmd != LINK_CMD_NONE)
			SendUartCmd('L', (char) LinkCmd);
		if (send_user_command)
			SendUartCmd('u', (char) UserCmd);
		if (LinkCmd != LINK_CMD_NONE || send_user_command)
			SendUartLF();		// Finish command(s) with line feed

		// Receive command from Raspberry Pi
		UserCommand UserCmdFromRpi = USER_CMD_NONE;
		while (true)
		{
			uartCmd = GetUartCmd(&uartValue);
			if (uartCmd == 0)
				break;

			switch (uartCmd)
			{
				case 'c':
					if (!humanControl)
						UserCmdFromRpi = (UserCommand) uartValue;
					break;
			}
		}

		// Driver can simply override autonomous control
		if (humanControl && UserCmd == USER_CMD_NONE)
			UserCmd = UserCmdFromRpi;

		////////////////////////////////////////////////////////////////////////////
		// Execute user command
		////////////////////////////////////////////////////////////////////////////

		CommandToBaseMotorPower(UserCmd, &bmLeft, &bmRight);

		// If something goes wrong with the bot,
		// user can stop the bot by pressing the stop_button_pressed button
		if (stop_button_pressed)
		{
			bmLeft = 0;
			bmRight = 0;
		}

		motor[LEFT] = bmLeft;
		motor[RIGHT] = bmRight;

		////////////////////////////////////////////////////////////////////////////
		// DISPLAY
		////////////////////////////////////////////////////////////////////////////

		// Blink LED
		SetGreenLed(!humanControl, !humanControl);
		
		UpdateLCD();

		// Motor values can only be updated every 20ms
		wait1Msec(20);

	}

	// Never executed; just to suppress a warning
	UserControlCodePlaceholderForTesting();
}

task usercontrol()
{
	UserControlFunction();
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//                                 Autonomous Task
//
// This task is used to control your robot during the autonomous phase of a VEX Competition.
// You must modify the code to add your own robot specific commands here.
//
/////////////////////////////////////////////////////////////////////////////////////////

task autonomous()
{
	// .....................................................................................
	// Insert user code here.
	// .....................................................................................

	AutonomousCodePlaceholderForTesting();
}

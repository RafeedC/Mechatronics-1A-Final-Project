#include "PC_FileIO.c"

const int MAXSIZE = 15;
const float PITCH = 2 * 0.15875; // in cm
const float LIFT_TIME = 0.3; // in sec
const int MAX_ULTRA_COUNT = 4;
const int LIFT_SPD = 13;
const int MIN_ULTRA = 9; // 20 cm for 1 Letter/ 7cm for 5 Letters
const tSensors TOUCH_X = S2;
const tSensors TOUCH_Y = S1;
const tSensors ULTRA = S3;
const tMotor MOTOR_Y1 = motorA; // Negative for pos y
const tMotor MOTOR_Y2 = motorD; // Positive for pos y
const tMotor MOTOR_X = motorB; // Negative for pos x
const tMotor MOTOR_UP = motorC;


// Reset Pencil
void resetPencil()
{
	while(!getButtonPress(buttonUp) && !getButtonPress(buttonEnter))
	{}

	motor[MOTOR_UP] = -1;

	while(getButtonPress(buttonUp))
	{}

	motor[MOTOR_UP] = 0;
}


//Displays average time for letter
void averageTime (TTimers timer, int characters)
{
	eraseDisplay();

	int totalTime = 0;
	float averageTime = 0;

	totalTime = time1[timer]/1000;
	averageTime = totalTime/characters;

	displayString(1, "The total time is %d", totalTime);
	displayString(2, "The average time is %d", averageTime);
}

//Moves Pencil Up or Down
void movePencilUporDown (int motorSpeed, tMotor motorUp, float time, bool TrueorFalse) // time in secs
{
	float timeMilSec = 0;
	timeMilSec = time*1000;

	time1[T2] = 0;

	if (TrueorFalse == 1) // 1 means pencil down
	{
		motor[motorUp] = motorSpeed;
		wait1Msec(timeMilSec);
		motor[motorUp] = 0;
	}
	else
	{
		motor[motorUp] = -motorSpeed;
		while (nMotorEncoder[motorUp]!=0)
		{
			if(time1[T2] > 5000)
				nMotorEncoder[motorUp] = 0;
		}
		motor[motorUp] = 0;
	}
}

//Reset to Origin
void resetToOrigin ()
{
	motor[MOTOR_X] = 100;
	while (SensorValue[TOUCH_X] == 0)
	{}
	motor[MOTOR_X] = 0;
	motor[MOTOR_Y1] = 100;
	motor[MOTOR_Y2] = -motor[MOTOR_Y1];
	while (SensorValue[TOUCH_Y] == 0)
	{}
	motor[MOTOR_Y1] = motor[MOTOR_Y2] = 0;
}

//Moves Marker linearly
void markerLine(float x_init, float y_init, float x_fin, float y_fin)
{
	nMotorEncoder[MOTOR_X] = 0;
	nMotorEncoder[MOTOR_Y1] = 0;
	float delX = 0;
	float delY = 0;
	delX = x_fin - x_init;
	delY = y_fin - y_init;

	if(fabs(delX) > fabs(delY))
	{
		if(delX > 0)
		{
			motor[MOTOR_X] = -100;
			motor[MOTOR_Y1] = 100 * delY/delX;
			motor[MOTOR_Y2] = - motor[MOTOR_Y1];

			while(nMotorEncoder[MOTOR_X] > -delX * 360 / PITCH)
			{}
		}
		else if(delX < 0)
		{
			motor[MOTOR_X] = 100;
			motor[MOTOR_Y1] = -100 * delY/delX;
			motor[MOTOR_Y2] = - motor[MOTOR_Y1];

			while(nMotorEncoder[MOTOR_X] < -delX * 360 / PITCH)
			{}
		}
		motor[MOTOR_X] = motor[MOTOR_Y1] = motor[MOTOR_Y2] = 0;
	}

	else if (fabs(delX) < fabs(delY))
	{
		if(delY > 0)
		{
			motor[MOTOR_X] = -100 * delX/delY;
			motor[MOTOR_Y1] = 100;
			motor[MOTOR_Y2] = - motor[MOTOR_Y1];

			while(nMotorEncoder[MOTOR_Y1] < delY * 360 / PITCH)
			{}
		}

		else if(delY <0)
		{
			motor[MOTOR_X] = 100 * delX/delY;
			motor[MOTOR_Y1] = -100;
			motor[MOTOR_Y2] = - motor[MOTOR_Y1];

			while(nMotorEncoder[MOTOR_Y1] > delY * 360 / PITCH)
			{}
		}
		motor[MOTOR_X] = motor[MOTOR_Y1] = motor[MOTOR_Y2] = 0;
		wait1Msec(50);
	}
}

//Setup next letter
void nextLetter(float x_fin, float y_fin)
{
	movePencilUporDown(LIFT_SPD, MOTOR_UP, LIFT_TIME, 0);
	markerLine(x_fin, y_fin, 4.3, 0);
}

//Move to next line
void nextLine ()
{
	nMotorEncoder[MOTOR_Y1] = 0;
	motor[MOTOR_Y1] = -100;
	motor[MOTOR_Y2] = -motor[MOTOR_Y1];
	while (nMotorEncoder[MOTOR_Y1] > - 5 * 360/ PITCH)
	{}
	motor[MOTOR_Y1] = motor[MOTOR_Y2] = 0;
	motor[MOTOR_X] = 100;
	while (!SensorValue[TOUCH_X])
	{}
	motor[MOTOR_X] = 0;
}

//Wait for centre button press
void centreButton()
{
	while(!getButtonPress(buttonEnter))
	{}

	while(getButtonPress(buttonEnter))
	{}
}

//MAIN
task main()
{
	displayString(5, "Press centre button to start!");
	displayString(6, "or reset pencil");
	resetPencil();
	nMotorEncoder[MOTOR_UP] = 0;
	centreButton();

	eraseDisplay();
	displayString(5, "Initializing...");

	TFileHandle fin_txt;
	bool fileOkay = openReadPC(fin_txt, "input.txt");

	SensorType[TOUCH_X] = sensorEV3_Touch;
	wait1Msec(50);
	SensorType[TOUCH_Y] = sensorEV3_Touch;
	wait1Msec(50);
	SensorType[ULTRA] = sensorEV3_Ultrasonic;
	wait1Msec(50);

	resetToOrigin();

	int letterCount = 0;
	char letter = ' ';
	int ultraCount = 0;
	float x_coord[MAXSIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	float y_coord[MAXSIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	nMotorEncoder[MOTOR_Y1] = 0;
	motor[MOTOR_Y1] = -100;
	motor[MOTOR_Y2] = -motor[MOTOR_Y1];
	while(nMotorEncoder[MOTOR_Y1] > -4.1 * 360/PITCH)
	{}
	eraseDisplay();
	motor[MOTOR_Y1] = motor[MOTOR_Y2] = 0;
	wait1Msec(1000);

	displayString(5, "Writing in Progress...");
	time1[T1] = 0;

	while(readCharPC(fin_txt, letter) && ultraCount < MAX_ULTRA_COUNT) // Writing four lines max
	{
		displayString(7, "Writing %c", letter);
		char template = ' ';
		int strokeNum = 0;
		int isDown = 0;
		int pencilCurr = 0;

		TFileHandle fin_instruct;
		bool fileOk = openReadPC(fin_instruct, "instruction.txt");

		while(letter != template && template != 'o')
		{
			readCharPC(fin_instruct, template);
		}

		readIntPC(fin_instruct, strokeNum);
		for(int index = 1; index <= strokeNum; index++)
		{
			readIntPC(fin_instruct, isDown);

			if(isDown == 1 && pencilCurr != 1)
			{
				movePencilUporDown(LIFT_SPD, MOTOR_UP, LIFT_TIME, 1);
				pencilCurr = 1;
			}
			else if(isDown == 0 && pencilCurr != 0)
			{
				movePencilUporDown(LIFT_SPD, MOTOR_UP, LIFT_TIME, 0);
				pencilCurr = 0;
			}

			readFloatPC(fin_instruct, x_coord[index]);
			readFloatPC(fin_instruct, y_coord[index]);

			markerLine(x_coord[index-1], y_coord[index-1], x_coord[index], y_coord[index]);
		}

		wait1Msec(100);

		nextLetter(x_coord[strokeNum], y_coord[strokeNum]);

		wait1Msec(100);

		if(ultraCount < MAX_ULTRA_COUNT && SensorValue[ULTRA] < MIN_ULTRA)
		{
			if(ultraCount < MAX_ULTRA_COUNT - 1)
			{
				nextLine();
			}
			ultraCount++;
		}

		closeFilePC(fin_instruct);
		letterCount++;

	}

	averageTime(T1, letterCount);
	resetToOrigin();

	displayString(5, "Press centre button to end!");
	displayString(7, "Characters written: %d", letterCount);
	centreButton();
}

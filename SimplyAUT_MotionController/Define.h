#pragma once

// for now just use a fixed max temperature
// it is probably possible to ask the laser what it is
// if the laser is changed, then that method would change though
#define MAX_LASER_TEMPERATURE	50

// after every write to the socket wait this long before trying to read the socket
// reading of the socket is in a thread, after reading an event is set, wait this long only for that evebnt to be set
// maximum time to wait for the MAG socket to connect before timing out
// MagControl.cpp
#define SOCKET_RECV_DELAY       1
#define SOCKET_RECV_TIMEOUT     500
#define SOCKET_CONNECT_TIMEOUT  500

// this is a best estimate until the encoder tick reading is fixed
// currently they are polled, and if too fast ticks are losrt
// the documentation said (768000.0) counts per revolution of a motor
// experimentation required that value to be modified
// the wheels were measured to 50 mm, used to calibrate a turn to forward/back distance
#define ENCODER_TICKS_PER_MM	16.79
#define COUNTS_PER_TURN			(768000.0 * 1.4903) //m 1.5096)
#define WHEEL_DIAMETER			50.0 // MM

// how many laser profile hits can be invalid in a row before truying for a new profile
#define PROFILE_VALIDATE_LENGTH		20

// used for navigation
#define GAP_PREDICT					5.0 // while not used ast this timne, using the 2mnd order filkter, prdict the laser position 5 mm in the future
#define NAVIGATION_GROW_BY			2048 // at 1 per mm, this would be 2 metres, do not want to re-allocate the memory too often
#define GAP_FILTER_MIN__WIDTH		10 // mm (filter over last 10-50 mm, 2nd order, so can handle variations)
#define GAP_FILTER_MAX__WIDTH		20 // 50 
#define VEL_FILTER_MIN_WIDTH		10 // only used in _DEBUG as velocity not used at this time for navighation
#define VEL_FILTER_MAX_WIDTH		50
#define GAP_BUFFER_LEN              200 // samples used in the 2nd order polynomiazl to model the gap at less than 1 sample per mm and 50 mm max, this is lots
#define DIVE_STRAIGHT_DIST			50 // in the non-PID navigation, drive stright this far before checking the weld cap gap again
#define MAX_GAP_CHANGE_PER_MM		1.0 // assume that the laser weld cap gap cannot change faster than 0.5 mm/mm, can only failt max of twice before taking new value regardless
#define MIN_GAP_TOLERANCE			0.1  // do nothing if offset below this
#define MAX_GAP_TOLERANCE			0.5  // try to keep offset within this

#define MIN_TURN_RATE1               99 // the unit turns by varying L/R wheel speeds
#define MAX_TURN_RATE1               70 // the greater the variance, the greater the slip, the slower, the long it takes to navigate
#define DFLT_TURN_RATE1				 85 // will use 70 for 1st 100 mm, then what user selects (this 
#define DFLT_MAX_TURN_RATE_LEN		200
#define NAVIGATION_P_OSCILLATE		2.0
#define NAVIGATION_P				0.9	// default constants for PID navigation
#define NAVIGATION_I				0.72 // ideally these would be modified during the cvalibration run
#define NAVIGATION_D				0.0
#define NAVIGATION_D_LEN            375
#define NAVIGATION_I_ACCUMULATE		750
#define NAVIGATION_PIVOT			50
#define NAVIGATION_TURN_DIST		2.5 // mm

#define CRAWLER_WIDTH				26.0 // mm // the wheels are measured to be this distance apart outer 'O' ring gasker to gasket
#define CRAWLER_LENGTH				23.2 // mm // dsistance wheel centre to centre if fully articulated
#define LASER_OFFSET				(CRAWLER_LENGTH/2 + 18.0) // distance the laser is infront of the centred crawler
#define RGB_OFFSET					(CRAWLER_LENGTH/2 - 65.0) // likewise the RGB sensor

#define TORQUE_LIMIT				"TL*=9.9982"	// this is the maximum torque that allowed, both turning and driving vertical require significant torque
#define MAX_LASER_PROFILE_TRIES		10

#define MAX_LASER_OFFSET			10.0 // mm can request the crawler to offset from the weld cap
#define MIN_PIPE_CIRCUMFEENCE		100.0 // mm
#define MAX_PIPE_CIRCUMFEENCE		10000.0 // mm should never exceed 10 metres
#define DFLT_PIPE_CIRCUMFERENCE		1000.0
#define MIN_SCAN_DISTANCE			10.0 // mm if not a pipe, can scan a shorter distance
#define MAX_SCAN_DISTANCE			10000.0
#define DFLT_SCAN_DISTANCE			1000.0
#define SCAN_TYPE_CIRCUMFERENCE		0
#define SCAN_TYPE_DISTANCE			1
#define MIN_SCAN_OVERLAP			0.0 // no requirement for overlap
#define MAX_SCAN_OVERLAP			250.0 //
#define DFLT_SCAN_OVERLAP			50.0
#define MIN_MOTOR_SPEED				10.0 // mm/S cannot be zero
#define MAX_MOTOR_SPEED				100.0 // mm/S cannot be too fast
#define DFLT_START_SPEED			20.0
#define MIN_START_DIST				0.0
#define MAX_START_DIST				200.0
#define DFLT_START_DIST				100.0
#define DFLT_MOTOR_SPEED			50.0
#define MIN_MOTOR_ACCEL				5.0	// mm/S/S, do not want to accelerate too slow
#define MAX_MOTOR_ACCEL				100.0	// mm/S/S, too fast and may damage motors or wheels
#define DFLT_MOTOR_ACCELERATION		25.0
#define MIN_PREDRIVE_DIST			0.0 // mm, optioon to back up to before the start, gioves time to navigate to the weld before scan starts
#define MAX_PREDRIVE_DIST			1000.0 // mm
#define MIN_SEEK_START_LINE_DIST	50.0 // mm, distance to drive when seeking the start line
#define MAX_SEEK_START_LINE_DIST	250.0 // mm, distance to drive when seeking the start line

#define TIMER_GET_LASER_PROFILE_INTERVAL	1 // mm
#define TIMER_LASER_STATUS1_INTERVAL		500 // ms
#define TIMER_SHOW_MOTOR_SPEEDS_INTERVAL	500
#define TIMER_LASER_TEMPERATURE_INTERVAL	500
#define TIMER_RGB_STATUS_INTERVAL			250
#define TIMER_NOTE_RGB_INTERVAL				500 // ms takes about 200 ms, so can not call too often (911)
//#define TIMER_ARE_MOTORS_RUNNING_INTERVAL	100	// the main thread checks if the motors are running to a variable that all threads can look at

#define LASER_SERIAL_NUMBER					10357 // as noted on the body of the laser, not used so does not matter if correct


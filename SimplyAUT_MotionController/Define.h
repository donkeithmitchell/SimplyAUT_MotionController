#pragma once

#define ENCODER_TICKS_PER_MM	17.09
#define MAX_LASER_TEMPERATURE	50

#define SOCKET_RECV_DELAY       1
#define SOCKET_RECV_TIMEOUT     500
#define SOCKET_CONNECT_TIMEOUT  500

#define COUNTS_PER_TURN			(768000.0 * 1.5096)
#define WHEEL_DIAMETER			50.0 // MM

#define MAX_LASER_CAP_DEVIATION		100

// used for navigation
#define GAP_PREDICT					5 // mm
#define MIN_TRAVEL_DIST				20 // minimum trav el dist of 20 mm befiore navigation kicks in
#define NAVIGATION_GROW_BY			2048 // at 1 per mm, this would be 2 metres
#define GAP_FILTER_MIN__WIDTH		10 // mm (filter over last 50 mm, 2nd order, so can handle variations)
#define GAP_FILTER_MAX__WIDTH		50 // 100 // mm (filter over last 50 mm, 2nd order, so can handle variations)
#define VEL_FILTER_MIN_WIDTH		10
#define VEL_FILTER_MAX_WIDTH		50
#define GAP_VELOCITY_WIDTH			20 // mm 
#define GAP_BUFFER_LEN              200 // samples
#define VELOCITY_MIN_COUNT          5
#define UPDATE_DISTANCE_MM			1 // mm
#define NAVIGATION_TARGET_DIST_MM	100
#define WHEEL_SPACING				(10.0 * 2.54)
#define MIN_STEERING_TIME			100
#define MAX_STEERING_TIME			2000
#define DESIRED_SPEED_VARIATION		5 // percent
#define MAX_SPEED_VARIATION			20 // percent
#define DIVE_STRAIGHT_DIST			50
#define MAX_GAP_CHANGE_PER_MM		0.5

#define CRAWLER_WIDTH				26.0 // mm
#define CRAWLER_LENGTH				23.0 // mm

#define MIN_GAP_TOLERANCE			0.1  // do nothing below this
#define MAX_GAP_TOLERANCE			0.5  // try to keep within this

#define MIN_TURN_RATE               0.99
#define MAX_TURN_RATE               0.70

#define NAVIGATION_P				2.0
#define NAVIGATION_I				0.005
#define NAVIGATION_D				200

#define TORQUE_LIMIT				"TL*=9.9982"


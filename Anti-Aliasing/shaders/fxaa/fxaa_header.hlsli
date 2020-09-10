// Tuning parameters
//#define EDGE_THRESHOLD (1.0 / 3.0)	// Too little
//#define EDGE_THRESHOLD (1.0 / 4.0)	// Low quality
//#define EDGE_THRESHOLD (1.0 / 8.0)	// High quality
#define EDGE_THRESHOLD (1.0 / 16.0)	// Overkill

#define EDGE_THRESHOLD_MIN (1.0 / 32.0)	// Visible limit
//#define EDGE_THRESHOLD_MIN (1.0 / 16.0) // High quality
//#define EDGE_THRESHOLD_MIN (1.0 / 12.0) // Upper limit

#define TAVERSAL_MAX_STEPS 16

#define SUBPIXEL_QUALITY 0.75

// Debug flags
//#define DEBUG_NO_FXAA
//#define DEBUG_SHOW_EDGES
//#define DEBUG_SHOW_EDGE_DIRECTION
//#define DEBUG_SHOW_EDGE_SIDE
//#define DEBUG_SHOW_CLOSEST_END
//#define DEBUG_NO_SUBPIXEL_ANTIALIASING
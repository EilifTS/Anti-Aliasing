// Tuning parameters

#ifndef EDGE_THRESHOLD
#define EDGE_THRESHOLD (1.0 / 8.0)
#endif

#ifndef EDGE_THRESHOLD_MIN
#define EDGE_THRESHOLD_MIN (1.0 / 32.0)
#endif

#define TRAVERSAL_MAX_STEPS 16

#define SUBPIXEL_QUALITY 0.75

// Debug flags
//#define DEBUG_SHOW_EDGES
//#define DEBUG_SHOW_EDGE_DIRECTION
//#define DEBUG_SHOW_EDGE_SIDE
//#define DEBUG_SHOW_CLOSEST_END
//#define DEBUG_NO_SUBPIXEL_ANTIALIASING
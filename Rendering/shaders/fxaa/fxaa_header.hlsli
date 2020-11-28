// Tuning parameters

//#ifndef EDGE_THRESHOLD
//#define EDGE_THRESHOLD (1.0 / 8.0)
//#endif

//#ifndef EDGE_THRESHOLD_MIN
//#define EDGE_THRESHOLD_MIN (1.0 / 32.0)
//#endif

//#define TRAVERSAL_MAX_STEPS 16

//#define SUBPIXEL_QUALITY 0.75

// Debug flags
#define DEBUG_SHOW_EDGES
//#define DEBUG_SHOW_EDGE_DIRECTION
//#define DEBUG_SHOW_EDGE_SIDE
//#define DEBUG_SHOW_CLOSEST_END
//#define DEBUG_NO_SUBPIXEL_ANTIALIASING

// For report
#define FXAA_SETUP 10

#if FXAA_SETUP > 0
#define EDGE_THRESHOLD (1.0 / 4.0)
#define EDGE_THRESHOLD_MIN (1.0 / 12.0)
#define TRAVERSAL_MAX_STEPS 4
#define SUBPIXEL_QUALITY 0.25
#endif
#if FXAA_SETUP == 2
#define EDGE_THRESHOLD (1.0 / 8.0)
#endif
#if FXAA_SETUP == 3
#define EDGE_THRESHOLD (1.0 / 16.0)
#endif
#if FXAA_SETUP == 4
#define EDGE_THRESHOLD_MIN (1.0 / 16.0)
#endif
#if FXAA_SETUP == 5
#define EDGE_THRESHOLD_MIN (1.0 / 32.0)
#endif
#if FXAA_SETUP == 6
#define TRAVERSAL_MAX_STEPS 8
#endif
#if FXAA_SETUP == 7
#define TRAVERSAL_MAX_STEPS 16
#endif
#if FXAA_SETUP == 8
#define SUBPIXEL_QUALITY 0.5
#endif
#if FXAA_SETUP == 9
#define SUBPIXEL_QUALITY 0.75
#endif
#if FXAA_SETUP == 10
#define EDGE_THRESHOLD (1.0 / 16.0)
#define EDGE_THRESHOLD_MIN (1.0 / 32.0)
#define TRAVERSAL_MAX_STEPS 16
#define SUBPIXEL_QUALITY 0.75
#endif
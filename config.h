// Device ID used for MQTT 
#define DEVICE_ID        "signal-tower"

// Configure Dweet
#define DWEET   // To enable Dweet operations
#define DWEET_HOST       "dweet.io"
#define DWEET_DEVICE     "orangemoose-signaltower"
// Maximum time allowed without dweeting, to provide a status heartbeat
const uint16_t dweetInterval = 600;   // In seconds

// Enable debug output (via Serial port) if desired
// #define DEBUG

// Enable built-in Neopixel to simulate signal tower control
// #define USE_NEOPIXEL
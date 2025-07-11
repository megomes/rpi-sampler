#ifndef JACK_CLIENT_H
#define JACK_CLIENT_H

#include <jack/jack.h>

// JACK client configuration
typedef struct {
    const char *client_name;    // JACK client name
    int auto_connect;           // Auto-connect to system outputs
    int output_channels;        // Number of output channels (1=mono, 2=stereo)
} jack_config_t;

// JACK client state
typedef struct {
    jack_client_t *client;      // JACK client handle
    jack_port_t *output_left;   // Left output port
    jack_port_t *output_right;  // Right output port (NULL for mono)
    int is_active;              // 1 if client is active
    int sample_rate;            // Current JACK sample rate
    int buffer_size;            // Current JACK buffer size
} jack_state_t;

// Callback function types
typedef int (*jack_process_callback_t)(jack_nframes_t nframes, void *arg);
typedef void (*jack_shutdown_callback_t)(void *arg);

// JACK client functions
int jack_client_init(jack_config_t *config);
void jack_client_cleanup(void);

// Callback registration
int jack_client_set_process_callback(jack_process_callback_t callback, void *arg);
int jack_client_set_shutdown_callback(jack_shutdown_callback_t callback, void *arg);

// Client control
int jack_client_activate(void);
int jack_client_deactivate(void);

// Port management
int jack_client_connect_outputs(void);
int jack_client_disconnect_all(void);

// State queries
int jack_client_is_active(void);
int jack_client_get_sample_rate(void);
int jack_client_get_buffer_size(void);
jack_port_t* jack_client_get_output_port(int channel);

// Utility functions
const char* jack_client_get_name(void);
void jack_client_print_connections(void);

// Default configuration
jack_config_t jack_get_default_config(void);

#endif // JACK_CLIENT_H
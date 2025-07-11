#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <jack/jack.h>
#include "jack_client.h"

// Global JACK state
static jack_state_t jack_state = {0};
static jack_config_t jack_config = {0};

// User callbacks
static jack_process_callback_t user_process_callback = NULL;
static void *user_process_arg = NULL;
static jack_shutdown_callback_t user_shutdown_callback = NULL;
static void *user_shutdown_arg = NULL;

// Internal JACK callbacks
static int internal_process_callback(jack_nframes_t nframes, void *arg) {
    (void)arg;
    
    if (user_process_callback) {
        return user_process_callback(nframes, user_process_arg);
    }
    
    // Default: output silence
    jack_default_audio_sample_t *left = (jack_default_audio_sample_t*)
        jack_port_get_buffer(jack_state.output_left, nframes);
    
    if (jack_state.output_right) {
        jack_default_audio_sample_t *right = (jack_default_audio_sample_t*)
            jack_port_get_buffer(jack_state.output_right, nframes);
        
        // Clear stereo buffers
        memset(left, 0, nframes * sizeof(jack_default_audio_sample_t));
        memset(right, 0, nframes * sizeof(jack_default_audio_sample_t));
    } else {
        // Clear mono buffer
        memset(left, 0, nframes * sizeof(jack_default_audio_sample_t));
    }
    
    return 0;
}

static void internal_shutdown_callback(void *arg) {
    (void)arg;
    
    printf("JACK server shut down!\n");
    jack_state.is_active = 0;
    
    if (user_shutdown_callback) {
        user_shutdown_callback(user_shutdown_arg);
    }
}

// Check if JACK server is running
static int is_jack_running(void) {
    jack_client_t *test_client = jack_client_open("test_connection", JackNoStartServer, NULL);
    if (test_client) {
        jack_client_close(test_client);
        return 1;
    }
    return 0;
}


// Start JACK daemon automatically
static int start_jack_daemon(void) {
    printf("Starting JACK daemon automatically...\n");
    
    // Use the auto-detection script
    const char *start_script = "./start_jack.sh";
    
    // Check if script exists
    if (access(start_script, F_OK) != 0) {
        printf("Error: start_jack.sh not found\n");
        return -1;
    }
    
    // Execute the script and show output
    printf("Executing: %s\n", start_script);
    int result = system(start_script);
    
    if (result != 0) {
        printf("Failed to execute start_jack.sh (exit code: %d)\n", result);
        printf("Try running manually: %s\n", start_script);
        return -1;
    }
    
    // Wait longer for JACK to start (JACK can be slow on Pi)
    printf("Waiting for JACK to initialize");
    for (int i = 0; i < 20; i++) {
        sleep(1);
        if (is_jack_running()) {
            printf(" done!\n");
            printf("JACK daemon started successfully\n");
            return 0;
        }
        printf(".");
        fflush(stdout);
    }
    
    printf(" timeout!\n");
    printf("JACK failed to start automatically. Try manual start:\n");
    printf("  ./start_jack.sh\n");
    printf("  OR\n");
    printf("  jackd -dalsa -dhw:0 -r48000 -p1024 -n2 &\n");
    return -1;
}

// Default configuration
jack_config_t jack_get_default_config(void) {
    jack_config_t config = {
        .client_name = "rpi-sampler",
        .auto_connect = 1,
        .output_channels = 2
    };
    return config;
}

// Initialize JACK client
int jack_client_init(jack_config_t *config) {
    if (jack_state.client) {
        printf("JACK client already initialized\n");
        return 0;
    }
    
    if (!config) {
        jack_config = jack_get_default_config();
    } else {
        jack_config = *config;
    }
    
    printf("Initializing JACK client...\n");
    printf("Client name: %s\n", jack_config.client_name);
    printf("Output channels: %d\n", jack_config.output_channels);
    
    // Check if JACK is running, if not try to start it
    jack_status_t status;
    jack_state.client = jack_client_open(jack_config.client_name, JackNoStartServer, &status);
    
    if (!jack_state.client && (status & JackServerFailed)) {
        printf("JACK server not running, attempting to start...\n");
        
        if (start_jack_daemon() < 0) {
            printf("\n=== JACK AUTO-START FAILED ===\n");
            printf("The auto-detection couldn't find a working audio configuration.\n");
            printf("\nTry manually:\n");
            printf("1. Check your audio devices: aplay -l\n");
            printf("2. Start JACK manually:\n");
            printf("   ./start_jack.sh\n");
            printf("   OR try specific commands:\n");
            printf("   jackd -dalsa -dhw:0 -r48000 -p1024 -n2 &\n");
            printf("   jackd -dalsa -dhw:1 -r48000 -p1024 -n2 &\n");
            printf("   jackd -dalsa -dplughw:0 -r44100 -p1024 -n2 &\n");
            printf("\n3. Then restart the sampler: ./sampler\n");
            return -1;
        }
        
        // Try to connect again after successful JACK start
        printf("JACK started, attempting to connect...\n");
        jack_state.client = jack_client_open(jack_config.client_name, JackNullOption, &status);
    }
    
    if (!jack_state.client) {
        printf("Error: Cannot create JACK client\n");
        printf("Status: 0x%x\n", status);
        return -1;
    }
    
    if (status & JackNameNotUnique) {
        printf("Warning: JACK client name was not unique, assigned: %s\n", 
               jack_get_client_name(jack_state.client));
    }
    
    // Get JACK settings
    jack_state.sample_rate = jack_get_sample_rate(jack_state.client);
    jack_state.buffer_size = jack_get_buffer_size(jack_state.client);
    
    printf("JACK sample rate: %d Hz\n", jack_state.sample_rate);
    printf("JACK buffer size: %d frames\n", jack_state.buffer_size);
    
    // Register output ports
    jack_state.output_left = jack_port_register(jack_state.client, "out_left",
                                               JACK_DEFAULT_AUDIO_TYPE,
                                               JackPortIsOutput, 0);
    if (!jack_state.output_left) {
        printf("Error: Cannot register left output port\n");
        jack_client_close(jack_state.client);
        jack_state.client = NULL;
        return -1;
    }
    
    if (jack_config.output_channels == 2) {
        jack_state.output_right = jack_port_register(jack_state.client, "out_right",
                                                    JACK_DEFAULT_AUDIO_TYPE,
                                                    JackPortIsOutput, 0);
        if (!jack_state.output_right) {
            printf("Error: Cannot register right output port\n");
            jack_client_close(jack_state.client);
            jack_state.client = NULL;
            return -1;
        }
    }
    
    // Set internal callbacks
    jack_set_process_callback(jack_state.client, internal_process_callback, NULL);
    jack_on_shutdown(jack_state.client, internal_shutdown_callback, NULL);
    
    printf("JACK client initialized successfully\n");
    return 0;
}

// Cleanup JACK client
void jack_client_cleanup(void) {
    if (!jack_state.client) {
        return;
    }
    
    printf("Cleaning up JACK client...\n");
    
    if (jack_state.is_active) {
        jack_client_deactivate();
    }
    
    jack_client_close(jack_state.client);
    memset(&jack_state, 0, sizeof(jack_state));
    
    printf("JACK client cleaned up\n");
}

// Set process callback
int jack_client_set_process_callback(jack_process_callback_t callback, void *arg) {
    user_process_callback = callback;
    user_process_arg = arg;
    return 0;
}

// Set shutdown callback
int jack_client_set_shutdown_callback(jack_shutdown_callback_t callback, void *arg) {
    user_shutdown_callback = callback;
    user_shutdown_arg = arg;
    return 0;
}

// Activate client
int jack_client_activate(void) {
    if (!jack_state.client) {
        printf("Error: JACK client not initialized\n");
        return -1;
    }
    
    if (jack_state.is_active) {
        printf("JACK client already active\n");
        return 0;
    }
    
    printf("Activating JACK client...\n");
    
    if (jack_activate(jack_state.client) != 0) {
        printf("Error: Cannot activate JACK client\n");
        return -1;
    }
    
    jack_state.is_active = 1;
    printf("JACK client activated\n");
    
    // Auto-connect if requested
    if (jack_config.auto_connect) {
        jack_client_connect_outputs();
    }
    
    return 0;
}

// Deactivate client
int jack_client_deactivate(void) {
    if (!jack_state.client || !jack_state.is_active) {
        return 0;
    }
    
    printf("Deactivating JACK client...\n");
    
    jack_deactivate(jack_state.client);
    jack_state.is_active = 0;
    
    printf("JACK client deactivated\n");
    return 0;
}

// Connect outputs to system
int jack_client_connect_outputs(void) {
    if (!jack_state.client || !jack_state.is_active) {
        printf("Error: JACK client not active\n");
        return -1;
    }
    
    printf("Auto-connecting outputs...\n");
    
    const char **system_ports = jack_get_ports(jack_state.client, NULL, NULL,
                                              JackPortIsPhysical | JackPortIsInput);
    
    if (!system_ports) {
        printf("Warning: No physical playback ports found\n");
        return -1;
    }
    
    // Connect left output
    const char *left_port = jack_port_name(jack_state.output_left);
    if (jack_connect(jack_state.client, left_port, system_ports[0]) == 0) {
        printf("Connected %s -> %s\n", left_port, system_ports[0]);
    } else {
        printf("Warning: Could not connect left output\n");
    }
    
    // Connect right output (if stereo)
    if (jack_state.output_right && system_ports[1]) {
        const char *right_port = jack_port_name(jack_state.output_right);
        if (jack_connect(jack_state.client, right_port, system_ports[1]) == 0) {
            printf("Connected %s -> %s\n", right_port, system_ports[1]);
        } else {
            printf("Warning: Could not connect right output\n");
        }
    }
    
    jack_free(system_ports);
    return 0;
}

// Disconnect all connections
int jack_client_disconnect_all(void) {
    if (!jack_state.client) {
        return -1;
    }
    
    jack_port_disconnect(jack_state.client, jack_state.output_left);
    
    if (jack_state.output_right) {
        jack_port_disconnect(jack_state.client, jack_state.output_right);
    }
    
    return 0;
}

// State queries
int jack_client_is_active(void) {
    return jack_state.is_active;
}

int jack_client_get_sample_rate(void) {
    return jack_state.sample_rate;
}

int jack_client_get_buffer_size(void) {
    return jack_state.buffer_size;
}

jack_port_t* jack_client_get_output_port(int channel) {
    if (channel == 0) {
        return jack_state.output_left;
    } else if (channel == 1) {
        return jack_state.output_right;
    }
    return NULL;
}

const char* jack_client_get_name(void) {
    if (!jack_state.client) {
        return "not initialized";
    }
    return jack_get_client_name(jack_state.client);
}

// Print current connections
void jack_client_print_connections(void) {
    if (!jack_state.client) {
        printf("JACK client not initialized\n");
        return;
    }
    
    printf("Current JACK connections:\n");
    
    // Print left connections
    const char **connections = jack_port_get_connections(jack_state.output_left);
    if (connections) {
        for (int i = 0; connections[i]; i++) {
            printf("  %s -> %s\n", jack_port_name(jack_state.output_left), connections[i]);
        }
        jack_free(connections);
    } else {
        printf("  %s: not connected\n", jack_port_name(jack_state.output_left));
    }
    
    // Print right connections (if stereo)
    if (jack_state.output_right) {
        connections = jack_port_get_connections(jack_state.output_right);
        if (connections) {
            for (int i = 0; connections[i]; i++) {
                printf("  %s -> %s\n", jack_port_name(jack_state.output_right), connections[i]);
            }
            jack_free(connections);
        } else {
            printf("  %s: not connected\n", jack_port_name(jack_state.output_right));
        }
    }
}
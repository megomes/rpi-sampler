#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "midi.h"

// Internal state
static snd_seq_t *seq_handle = NULL;
static int midi_port = -1;
static char connected_device_name[256] = "";
static int connected_device_client = -1;

// Callback pointers
static midi_note_callback_t note_callback = NULL;
static midi_cc_callback_t cc_callback = NULL;
static midi_pitch_callback_t pitch_callback = NULL;
static midi_program_callback_t program_callback = NULL;
static midi_pressure_callback_t pressure_callback = NULL;
static midi_key_pressure_callback_t key_pressure_callback = NULL;
static midi_transport_callback_t start_callback = NULL;
static midi_transport_callback_t stop_callback = NULL;
static midi_transport_callback_t continue_callback = NULL;

// Internal function prototypes
static void verify_connections(void);
static int find_first_midi_device(void);

// Callback registration functions
void midi_set_note_callback(midi_note_callback_t callback) {
    note_callback = callback;
}

void midi_set_cc_callback(midi_cc_callback_t callback) {
    cc_callback = callback;
}

void midi_set_pitch_callback(midi_pitch_callback_t callback) {
    pitch_callback = callback;
}

void midi_set_program_callback(midi_program_callback_t callback) {
    program_callback = callback;
}

void midi_set_pressure_callback(midi_pressure_callback_t callback) {
    pressure_callback = callback;
}

void midi_set_key_pressure_callback(midi_key_pressure_callback_t callback) {
    key_pressure_callback = callback;
}

void midi_set_start_callback(midi_transport_callback_t callback) {
    start_callback = callback;
}

void midi_set_stop_callback(midi_transport_callback_t callback) {
    stop_callback = callback;
}

void midi_set_continue_callback(midi_transport_callback_t callback) {
    continue_callback = callback;
}

// Utility functions
const char* midi_get_connected_device_name(void) {
    return connected_device_name;
}

int midi_get_connected_device_id(void) {
    return connected_device_client;
}

// Connection verification
static void verify_connections(void) {
    snd_seq_query_subscribe_t *subs;
    snd_seq_addr_t root_addr;
    int err;
    
    snd_seq_query_subscribe_alloca(&subs);
    
    printf("Active connections to our port:\n");
    
    // Set up root address (our port)
    root_addr.client = snd_seq_client_id(seq_handle);
    root_addr.port = midi_port;
    
    // Query write subscriptions (who is sending to us)
    snd_seq_query_subscribe_set_root(subs, &root_addr);
    snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(subs, 0);
    
    while ((err = snd_seq_query_port_subscribers(seq_handle, subs)) >= 0) {
        const snd_seq_addr_t *addr = snd_seq_query_subscribe_get_addr(subs);
        printf("  <- %d:%d is sending to us\n", addr->client, addr->port);
        snd_seq_query_subscribe_set_index(subs, snd_seq_query_subscribe_get_index(subs) + 1);
    }
    
    if (snd_seq_query_subscribe_get_index(subs) == 0) {
        printf("  No active write connections found!\n");
    }
}

// Find and connect to first MIDI device
static int find_first_midi_device(void) {
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int client;
    int devices_found = 0;

    snd_seq_client_info_malloc(&cinfo);
    snd_seq_port_info_malloc(&pinfo);

    printf("\nScanning for MIDI devices...\n");
    printf("============================\n");

    // Iterate through all clients to find first MIDI device
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq_handle, cinfo) >= 0) {
        client = snd_seq_client_info_get_client(cinfo);
        const char* client_name = snd_seq_client_info_get_name(cinfo);
        
        printf("Client %d: %s", client, client_name);
        
        // Skip system clients but show them for debugging
        if (client == 0 || client == 14 || client == 15) {
            printf(" [SYSTEM - SKIPPED]\n");
            continue;
        }
        printf("\n");

        // Check if this client has any ports
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        
        int port_count = 0;
        while (snd_seq_query_next_port(seq_handle, pinfo) >= 0) {
            unsigned int capability = snd_seq_port_info_get_capability(pinfo);
            int port = snd_seq_port_info_get_port(pinfo);
            const char* port_name = snd_seq_port_info_get_name(pinfo);
            
            printf("  Port %d: %s", port, port_name);
            
            // Show capabilities
            printf(" [");
            if (capability & SND_SEQ_PORT_CAP_READ) printf("READ ");
            if (capability & SND_SEQ_PORT_CAP_WRITE) printf("WRITE ");
            if (capability & SND_SEQ_PORT_CAP_SUBS_READ) printf("SUBS_READ ");
            if (capability & SND_SEQ_PORT_CAP_SUBS_WRITE) printf("SUBS_WRITE ");
            if (capability & SND_SEQ_PORT_CAP_NO_EXPORT) printf("NO_EXPORT ");
            printf("]");
            
            // Look for writable ports (MIDI input from our perspective)
            if ((capability & SND_SEQ_PORT_CAP_WRITE) && 
                !(capability & SND_SEQ_PORT_CAP_NO_EXPORT)) {
                
                printf(" <- SUITABLE FOR CONNECTION");
                
                if (devices_found == 0) {
                    printf("\n  -> Attempting connection to this device...");
                    
                    // Connect to this port
                    int connect_result = snd_seq_connect_from(seq_handle, midi_port, client, port);
                    if (connect_result >= 0) {
                        printf(" SUCCESS!\n");
                        printf("\nConnected to: %s - %s (Client: %d, Port: %d)\n", 
                               client_name, port_name, client, port);
                        
                        // Store connection info
                        snprintf(connected_device_name, sizeof(connected_device_name), 
                                "%s - %s", client_name, port_name);
                        connected_device_client = client;
                        
                        // Verify connection
                        printf("\nVerifying connection...\n");
                        verify_connections();
                        
                        snd_seq_client_info_free(cinfo);
                        snd_seq_port_info_free(pinfo);
                        return 0;
                    } else {
                        printf(" FAILED: %s\n", snd_strerror(connect_result));
                    }
                    devices_found++;
                }
            }
            printf("\n");
            port_count++;
        }
        
        if (port_count == 0) {
            printf("  No ports found\n");
        }
        printf("\n");
    }

    snd_seq_client_info_free(cinfo);
    snd_seq_port_info_free(pinfo);
    
    if (devices_found == 0) {
        printf("No suitable MIDI devices found!\n");
        printf("Make sure your USB MIDI device is connected and recognized by the system.\n");
        printf("Try running './list_midi' to see available devices.\n");
    }
    
    return -1; // No MIDI device found
}

// Initialize MIDI system
int midi_init(void) {
    int err;
    
    // Open ALSA sequencer
    err = snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
    if (err < 0) {
        printf("Error opening ALSA sequencer: %s\n", snd_strerror(err));
        return -1;
    }

    // Set client name
    snd_seq_set_client_name(seq_handle, "RPi Sampler");

    // Create input port
    midi_port = snd_seq_create_simple_port(seq_handle, "Input",
                                          SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                          SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    if (midi_port < 0) {
        printf("Error creating MIDI port: %s\n", snd_strerror(midi_port));
        snd_seq_close(seq_handle);
        return -1;
    }

    printf("MIDI system initialized. Port ID: %d\n", midi_port);
    
    // Try to connect to first available MIDI device
    if (find_first_midi_device() < 0) {
        printf("No MIDI devices found. Connect a USB MIDI device and restart.\n");
        return -1;
    }

    return 0;
}

// Process MIDI events
int midi_process_events(void) {
    snd_seq_event_t *ev;
    int err;
    int events_processed = 0;
    
    // Process MIDI events (non-blocking)
    while ((err = snd_seq_event_input(seq_handle, &ev)) >= 0) {
        events_processed++;
        
        switch (ev->type) {
            case SND_SEQ_EVENT_NOTEON:
                if (note_callback) {
                    midi_note_event_t event;
                    event.note = ev->data.note.note;
                    event.velocity = ev->data.note.velocity;
                    event.channel = ev->data.note.channel;
                    event.is_note_on = (ev->data.note.velocity > 0) ? 1 : 0;
                    note_callback(&event);
                }
                break;
                
            case SND_SEQ_EVENT_NOTEOFF:
                if (note_callback) {
                    midi_note_event_t event;
                    event.note = ev->data.note.note;
                    event.velocity = 0;
                    event.channel = ev->data.note.channel;
                    event.is_note_on = 0;
                    note_callback(&event);
                }
                break;
                
            case SND_SEQ_EVENT_CONTROLLER:
                if (cc_callback) {
                    midi_cc_event_t event;
                    event.controller = ev->data.control.param;
                    event.value = ev->data.control.value;
                    event.channel = ev->data.control.channel;
                    cc_callback(&event);
                }
                break;
                
            case SND_SEQ_EVENT_PITCHBEND:
                if (pitch_callback) {
                    midi_pitch_event_t event;
                    event.value = ev->data.control.value;
                    event.channel = ev->data.control.channel;
                    pitch_callback(&event);
                }
                break;
                
            case SND_SEQ_EVENT_PGMCHANGE:
                if (program_callback) {
                    midi_program_event_t event;
                    event.program = ev->data.control.value;
                    event.channel = ev->data.control.channel;
                    program_callback(&event);
                }
                break;
                
            case SND_SEQ_EVENT_CHANPRESS:
                if (pressure_callback) {
                    midi_pressure_event_t event;
                    event.pressure = ev->data.control.value;
                    event.channel = ev->data.control.channel;
                    pressure_callback(&event);
                }
                break;
                
            case SND_SEQ_EVENT_KEYPRESS:
                if (key_pressure_callback) {
                    midi_key_pressure_event_t event;
                    event.note = ev->data.note.note;
                    event.pressure = ev->data.note.velocity;
                    event.channel = ev->data.note.channel;
                    key_pressure_callback(&event);
                }
                break;
                
            case SND_SEQ_EVENT_START:
                if (start_callback) {
                    start_callback();
                }
                break;
                
            case SND_SEQ_EVENT_STOP:
                if (stop_callback) {
                    stop_callback();
                }
                break;
                
            case SND_SEQ_EVENT_CONTINUE:
                if (continue_callback) {
                    continue_callback();
                }
                break;
                
            // Ignore clock and active sensing to keep output clean
            case SND_SEQ_EVENT_CLOCK:
            case SND_SEQ_EVENT_SENSING:
                break;
                
            default:
                // Ignore unknown events
                break;
        }
        
        snd_seq_free_event(ev);
    }
    
    // Handle error cases (but -EAGAIN is normal for non-blocking)
    if (err < 0 && err != -EAGAIN) {
        printf("MIDI input error: %s\n", snd_strerror(err));
        return -1;
    }
    
    return events_processed;
}

// Cleanup MIDI system
void midi_cleanup(void) {
    if (seq_handle) {
        snd_seq_close(seq_handle);
        seq_handle = NULL;
    }
    
    // Clear connection info
    connected_device_name[0] = '\0';
    connected_device_client = -1;
    midi_port = -1;
    
    // Clear callbacks
    note_callback = NULL;
    cc_callback = NULL;
    pitch_callback = NULL;
    program_callback = NULL;
    pressure_callback = NULL;
    key_pressure_callback = NULL;
    start_callback = NULL;
    stop_callback = NULL;
    continue_callback = NULL;
}
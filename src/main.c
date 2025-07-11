#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "midi.h"
#include "jack_client.h"
#include "audio_engine.h"
#include "sample_loader.h"

static int running = 1;

void signal_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("\nShutting down...\n");
    running = 0;
}

// MIDI event callback functions
void on_midi_note(midi_note_event_t *event) {
    if (event->is_note_on) {
        printf("Note ON:  Channel=%d, Note=%d, Velocity=%d -> Playing sample\n",
               event->channel, event->note, event->velocity);
        
        // Play the loaded sample
        audio_sample_t *sample = sample_loader_get_first_sample();
        if (sample) {
            if (audio_engine_play_sample(sample) < 0) {
                printf("Error playing sample\n");
            }
        } else {
            printf("No sample loaded to play\n");
        }
    } else {
        printf("Note OFF: Channel=%d, Note=%d\n",
               event->channel, event->note);
    }
}

void on_midi_cc(midi_cc_event_t *event) {
    printf("CC:       Channel=%d, Controller=%d, Value=%d\n",
           event->channel, event->controller, event->value);
}

void on_midi_pitch(midi_pitch_event_t *event) {
    printf("Pitch:    Channel=%d, Value=%d\n",
           event->channel, event->value);
}

void on_midi_program(midi_program_event_t *event) {
    printf("Program:  Channel=%d, Program=%d\n",
           event->channel, event->program);
}

void on_midi_pressure(midi_pressure_event_t *event) {
    printf("ChanPress: Channel=%d, Value=%d\n",
           event->channel, event->pressure);
}

void on_midi_key_pressure(midi_key_pressure_event_t *event) {
    printf("KeyPress: Channel=%d, Note=%d, Value=%d\n",
           event->channel, event->note, event->pressure);
}

void on_midi_start(void) {
    printf("MIDI Start\n");
}

void on_midi_stop(void) {
    printf("MIDI Stop\n");
}

void on_midi_continue(void) {
    printf("MIDI Continue\n");
}


int main(void) {
    printf("Raspberry Pi MIDI Sampler - JACK Audio System\n");
    printf("==============================================\n");
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize JACK client
    printf("\nInitializing JACK client...\n");
    jack_config_t jack_config = jack_get_default_config();
    if (jack_client_init(&jack_config) < 0) {
        printf("Failed to initialize JACK client\n");
        printf("Make sure JACK is running: jackd -dalsa -dhw:0 -r48000 -p1024 -n2\n");
        return 1;
    }
    
    // Initialize audio engine
    printf("\nInitializing audio engine...\n");
    audio_engine_config_t engine_config = audio_engine_get_default_config();
    if (audio_engine_init(&engine_config) < 0) {
        printf("Failed to initialize audio engine\n");
        jack_client_cleanup();
        return 1;
    }
    
    // Connect audio engine to JACK
    jack_client_set_process_callback(audio_engine_process, NULL);
    jack_client_set_shutdown_callback(audio_engine_shutdown, NULL);
    
    // Initialize sample loader
    printf("\nInitializing sample loader...\n");
    char samples_path[256];
    snprintf(samples_path, sizeof(samples_path), "%s/samples", getenv("HOME"));
    
    if (sample_loader_init(samples_path) < 0) {
        printf("Failed to initialize sample loader\n");
        audio_engine_cleanup();
        jack_client_cleanup();
        return 1;
    }
    
    // Show loaded sample information
    printf("\nSample information:\n");
    sample_loader_list_samples();
    
    // Initialize MIDI system
    printf("\nInitializing MIDI system...\n");
    if (midi_init() < 0) {
        sample_loader_cleanup();
        audio_engine_cleanup();
        jack_client_cleanup();
        return 1;
    }
    
    // Register MIDI event callbacks
    midi_set_note_callback(on_midi_note);
    midi_set_cc_callback(on_midi_cc);
    midi_set_pitch_callback(on_midi_pitch);
    midi_set_program_callback(on_midi_program);
    midi_set_pressure_callback(on_midi_pressure);
    midi_set_key_pressure_callback(on_midi_key_pressure);
    midi_set_start_callback(on_midi_start);
    midi_set_stop_callback(on_midi_stop);
    midi_set_continue_callback(on_midi_continue);
    
    // Activate JACK client (start audio processing)
    printf("\nActivating JACK client...\n");
    if (jack_client_activate() < 0) {
        printf("Failed to activate JACK client\n");
        midi_cleanup();
        sample_loader_cleanup();
        audio_engine_cleanup();
        jack_client_cleanup();
        return 1;
    }
    
    printf("\nSystem ready!\n");
    printf("Connected to MIDI: %s\n", midi_get_connected_device_name());
    printf("Loaded sample: %s\n", sample_loader_get_first_sample_name());
    printf("JACK client: %s (%d Hz, %d frames)\n", 
           jack_client_get_name(), jack_client_get_sample_rate(), jack_client_get_buffer_size());
    printf("\nJACK connections:\n");
    jack_client_print_connections();
    printf("\nPress keys on MIDI device to trigger samples (Ctrl+C to stop)...\n");
    printf("================================================================\n");
    
    // Main real-time loop
    while (running) {
        // Process MIDI events (non-blocking)
        midi_process_events();
        
        // Sleep for 1ms to maintain ~1000Hz polling rate
        // This gives us sub-5ms latency while keeping CPU usage reasonable
        usleep(1000);
    }
    
    // Cleanup
    printf("\nShutting down systems...\n");
    midi_cleanup();
    sample_loader_cleanup();
    audio_engine_cleanup();
    jack_client_cleanup();
    printf("Sampler stopped.\n");
    
    return 0;
}
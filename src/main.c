#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "midi.h"

static int running = 1;

void signal_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    printf("\nShutting down...\n");
    running = 0;
}

// MIDI event callback functions
void on_midi_note(midi_note_event_t *event) {
    if (event->is_note_on) {
        printf("Note ON:  Channel=%d, Note=%d, Velocity=%d\n",
               event->channel, event->note, event->velocity);
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
    printf("Raspberry Pi MIDI Sampler - Real-time MIDI Monitor\n");
    printf("==================================================\n");
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize MIDI system
    if (midi_init() < 0) {
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
    
    printf("\nConnected to: %s\n", midi_get_connected_device_name());
    printf("Monitoring MIDI input (Press Ctrl+C to stop)...\n");
    printf("-----------------------------------------------\n");
    
    // Main real-time loop
    while (running) {
        // Process MIDI events (non-blocking)
        midi_process_events();
        
        // Sleep for 1ms to maintain ~1000Hz polling rate
        // This gives us sub-5ms latency while keeping CPU usage reasonable
        usleep(1000);
    }
    
    // Cleanup
    midi_cleanup();
    printf("MIDI monitor stopped.\n");
    
    return 0;
}
#ifndef MIDI_H
#define MIDI_H

// MIDI event structures
typedef struct {
    int note;
    int velocity;
    int channel;
    int is_note_on;  // 1 for note on, 0 for note off
} midi_note_event_t;

typedef struct {
    int controller;
    int value;
    int channel;
} midi_cc_event_t;

typedef struct {
    int value;
    int channel;
} midi_pitch_event_t;

typedef struct {
    int program;
    int channel;
} midi_program_event_t;

typedef struct {
    int pressure;
    int channel;
} midi_pressure_event_t;

typedef struct {
    int note;
    int pressure;
    int channel;
} midi_key_pressure_event_t;

// Callback function types
typedef void (*midi_note_callback_t)(midi_note_event_t *event);
typedef void (*midi_cc_callback_t)(midi_cc_event_t *event);
typedef void (*midi_pitch_callback_t)(midi_pitch_event_t *event);
typedef void (*midi_program_callback_t)(midi_program_event_t *event);
typedef void (*midi_pressure_callback_t)(midi_pressure_event_t *event);
typedef void (*midi_key_pressure_callback_t)(midi_key_pressure_event_t *event);
typedef void (*midi_transport_callback_t)(void);

// MIDI system functions
int midi_init(void);
void midi_cleanup(void);
int midi_process_events(void);

// Callback registration functions
void midi_set_note_callback(midi_note_callback_t callback);
void midi_set_cc_callback(midi_cc_callback_t callback);
void midi_set_pitch_callback(midi_pitch_callback_t callback);
void midi_set_program_callback(midi_program_callback_t callback);
void midi_set_pressure_callback(midi_pressure_callback_t callback);
void midi_set_key_pressure_callback(midi_key_pressure_callback_t callback);
void midi_set_start_callback(midi_transport_callback_t callback);
void midi_set_stop_callback(midi_transport_callback_t callback);
void midi_set_continue_callback(midi_transport_callback_t callback);

// Utility functions
const char* midi_get_connected_device_name(void);
int midi_get_connected_device_id(void);

#endif // MIDI_H
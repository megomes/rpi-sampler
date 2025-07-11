#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <jack/jack.h>

// Audio sample structure (same as before)
typedef struct {
    float *data;        // Audio data (interleaved for stereo)
    int frames;         // Number of frames (samples per channel)
    int channels;       // Number of channels (1=mono, 2=stereo)
    int sample_rate;    // Sample rate in Hz
} audio_sample_t;

// Audio voice structure for polyphonic playback
typedef struct {
    audio_sample_t *sample;     // Sample being played
    float playback_position;    // Current position in sample (frames, float for resampling)
    float sample_rate_ratio;    // Sample rate conversion ratio
    float volume;               // Voice volume (0.0 - 1.0)
    int active;                 // 1 if voice is active, 0 if free
    int voice_id;               // Unique voice identifier
} audio_voice_t;

// Audio engine configuration
typedef struct {
    int max_voices;             // Maximum number of simultaneous voices
    float master_gain;          // Master volume (0.0 - 1.0)
    int auto_gain_control;      // Enable automatic gain control for polyphony
} audio_engine_config_t;

#define MAX_VOICES 8

// Audio engine functions
int audio_engine_init(audio_engine_config_t *config);
void audio_engine_cleanup(void);

// JACK integration
int audio_engine_process(jack_nframes_t nframes, void *arg);
void audio_engine_shutdown(void *arg);

// Voice management
int audio_engine_trigger_sample(audio_sample_t *sample, float volume);
void audio_engine_stop_voice(int voice_id);
void audio_engine_stop_all_voices(void);
int audio_engine_get_active_voices(void);

// Legacy compatibility
int audio_engine_play_sample(audio_sample_t *sample);

// Configuration
audio_engine_config_t audio_engine_get_default_config(void);
int audio_engine_set_master_gain(float gain);
float audio_engine_get_master_gain(void);

// Sample management
void audio_sample_free(audio_sample_t *sample);
audio_sample_t* audio_sample_clone(audio_sample_t *sample);

// Statistics and monitoring
void audio_engine_print_stats(void);
int audio_engine_get_cpu_load(void);

#endif // AUDIO_ENGINE_H
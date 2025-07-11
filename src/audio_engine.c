#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <jack/jack.h>
#include "audio_engine.h"
#include "jack_client.h"

// Engine state
static audio_engine_config_t engine_config;
static int engine_initialized = 0;

// Voice management
static audio_voice_t voices[MAX_VOICES];
static int next_voice_id = 1;
static pthread_mutex_t voice_mutex = PTHREAD_MUTEX_INITIALIZER;

// Statistics
static unsigned long total_frames_processed = 0;
static int last_active_voices = 0;

// Default configuration
audio_engine_config_t audio_engine_get_default_config(void) {
    audio_engine_config_t config = {
        .max_voices = MAX_VOICES,
        .master_gain = 0.7f,
        .auto_gain_control = 1
    };
    return config;
}

// Initialize voice management
static void init_voices(void) {
    pthread_mutex_lock(&voice_mutex);
    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].sample = NULL;
        voices[i].playback_position = 0.0f;
        voices[i].sample_rate_ratio = 1.0f;
        voices[i].volume = 1.0f;
        voices[i].active = 0;
        voices[i].voice_id = 0;
    }
    pthread_mutex_unlock(&voice_mutex);
}

// Find free voice slot
static int find_free_voice(void) {
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) {
            return i;
        }
    }
    return -1; // No free voices
}

// Get sample with simple access (no interpolation for now - JACK handles sample rate)
static inline float get_sample_simple(audio_sample_t *sample, int position, int channel) {
    if (position >= sample->frames) {
        return 0.0f;
    }
    
    if (sample->channels == 1) {
        return sample->data[position];
    } else {
        return sample->data[position * 2 + channel];
    }
}

// Initialize audio engine
int audio_engine_init(audio_engine_config_t *config) {
    if (engine_initialized) {
        printf("Audio engine already initialized\n");
        return 0;
    }
    
    if (!config) {
        engine_config = audio_engine_get_default_config();
    } else {
        engine_config = *config;
    }
    
    printf("Initializing audio engine...\n");
    printf("Max voices: %d\n", engine_config.max_voices);
    printf("Master gain: %.2f\n", engine_config.master_gain);
    printf("Auto gain control: %s\n", engine_config.auto_gain_control ? "enabled" : "disabled");
    
    // Initialize voice management
    init_voices();
    
    engine_initialized = 1;
    printf("Audio engine initialized successfully\n");
    
    return 0;
}

// Cleanup audio engine
void audio_engine_cleanup(void) {
    if (!engine_initialized) {
        return;
    }
    
    printf("Cleaning up audio engine...\n");
    
    // Stop all voices
    audio_engine_stop_all_voices();
    
    engine_initialized = 0;
    printf("Audio engine cleaned up\n");
}

// Main JACK process callback
int audio_engine_process(jack_nframes_t nframes, void *arg) {
    (void)arg;
    
    if (!engine_initialized) {
        return 0;
    }
    
    // Get output buffers from JACK
    jack_default_audio_sample_t *left_out = 
        (jack_default_audio_sample_t*)jack_port_get_buffer(jack_client_get_output_port(0), nframes);
    jack_default_audio_sample_t *right_out = 
        (jack_default_audio_sample_t*)jack_port_get_buffer(jack_client_get_output_port(1), nframes);
    
    // Clear output buffers
    memset(left_out, 0, nframes * sizeof(jack_default_audio_sample_t));
    if (right_out) {
        memset(right_out, 0, nframes * sizeof(jack_default_audio_sample_t));
    }
    
    // Quick check: skip processing if no voices are active
    pthread_mutex_lock(&voice_mutex);
    int has_active_voices = 0;
    for (int v = 0; v < MAX_VOICES; v++) {
        if (voices[v].active) {
            has_active_voices = 1;
            break;
        }
    }
    
    if (!has_active_voices) {
        pthread_mutex_unlock(&voice_mutex);
        total_frames_processed += nframes;
        return 0;
    }
    
    // First pass: count active voices for efficient processing
    int active_voices[MAX_VOICES];
    int active_count = 0;
    for (int v = 0; v < MAX_VOICES; v++) {
        if (voices[v].active && voices[v].sample) {
            active_voices[active_count++] = v;
        }
    }
    
    // Get current JACK sample rate for conversion
    int jack_sample_rate = jack_client_get_sample_rate();
    
    // Second pass: mix only active voices
    for (int i = 0; i < active_count; i++) {
        int v = active_voices[i];
        
        audio_voice_t *voice = &voices[v];
        audio_sample_t *sample = voice->sample;
        
        // Calculate sample rate ratio
        voice->sample_rate_ratio = (float)sample->sample_rate / (float)jack_sample_rate;
        
        // Check if voice has finished playing
        int int_position = (int)voice->playback_position;
        if (int_position >= sample->frames) {
            voice->active = 0;
            continue;
        }
        
        // Mix this voice into the output buffers
        for (jack_nframes_t f = 0; f < nframes; f++) {
            int_position = (int)voice->playback_position;
            if (int_position >= sample->frames) {
                voice->active = 0;
                break;
            }
            
            if (sample->channels == 1) {
                // Mono sample
                float sample_value = get_sample_simple(sample, int_position, 0) * voice->volume;
                
                left_out[f] += sample_value;
                if (right_out) {
                    right_out[f] += sample_value;  // Duplicate to right for stereo
                }
            } else {
                // Stereo sample
                float left = get_sample_simple(sample, int_position, 0) * voice->volume;
                float right = get_sample_simple(sample, int_position, 1) * voice->volume;
                
                left_out[f] += left;
                if (right_out) {
                    right_out[f] += right;
                } else {
                    // Mono output: mix stereo to mono
                    left_out[f] += (left + right) * 0.5f;
                }
            }
            
            // Advance voice playback position with sample rate conversion
            voice->playback_position += voice->sample_rate_ratio;
        }
    }
    
    pthread_mutex_unlock(&voice_mutex);
    
    // Apply master gain and auto gain control
    float gain = engine_config.master_gain;
    if (engine_config.auto_gain_control && active_count > 1) {
        gain *= (1.0f / sqrtf(active_count));  // Reduce gain with more voices
    }
    
    // Apply gain and soft limiting
    for (jack_nframes_t f = 0; f < nframes; f++) {
        left_out[f] *= gain;
        
        // Soft limiting to prevent clipping
        if (left_out[f] > 0.95f) left_out[f] = 0.95f;
        else if (left_out[f] < -0.95f) left_out[f] = -0.95f;
        
        if (right_out) {
            right_out[f] *= gain;
            if (right_out[f] > 0.95f) right_out[f] = 0.95f;
            else if (right_out[f] < -0.95f) right_out[f] = -0.95f;
        }
    }
    
    // Update statistics
    total_frames_processed += nframes;
    last_active_voices = active_count;
    
    return 0;
}

// JACK shutdown callback
void audio_engine_shutdown(void *arg) {
    (void)arg;
    printf("Audio engine: JACK shutdown detected\n");
    audio_engine_stop_all_voices();
}

// Trigger a sample to play
int audio_engine_trigger_sample(audio_sample_t *sample, float volume) {
    if (!engine_initialized || !sample) {
        return -1;
    }
    
    pthread_mutex_lock(&voice_mutex);
    
    int voice_slot = find_free_voice();
    if (voice_slot < 0) {
        pthread_mutex_unlock(&voice_mutex);
        printf("Warning: No free voices available\n");
        return -1;
    }
    
    // Setup new voice
    voices[voice_slot].sample = sample;
    voices[voice_slot].playback_position = 0.0f;
    voices[voice_slot].sample_rate_ratio = 1.0f;  // Will be calculated in process callback
    voices[voice_slot].volume = volume;
    voices[voice_slot].voice_id = next_voice_id++;
    voices[voice_slot].active = 1;
    
    int voice_id = voices[voice_slot].voice_id;
    
    pthread_mutex_unlock(&voice_mutex);
    
    printf("Triggered sample (Voice ID: %d, Slot: %d)\n", voice_id, voice_slot);
    printf("  Sample: %d frames, %d channels, %d Hz\n", 
           sample->frames, sample->channels, sample->sample_rate);
    
    return voice_id;
}

// Stop a specific voice
void audio_engine_stop_voice(int voice_id) {
    pthread_mutex_lock(&voice_mutex);
    
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].active && voices[i].voice_id == voice_id) {
            voices[i].active = 0;
            printf("Stopped voice ID: %d\n", voice_id);
            break;
        }
    }
    
    pthread_mutex_unlock(&voice_mutex);
}

// Stop all voices
void audio_engine_stop_all_voices(void) {
    pthread_mutex_lock(&voice_mutex);
    
    for (int i = 0; i < MAX_VOICES; i++) {
        voices[i].active = 0;
    }
    
    pthread_mutex_unlock(&voice_mutex);
    printf("Stopped all voices\n");
}

// Get number of active voices
int audio_engine_get_active_voices(void) {
    pthread_mutex_lock(&voice_mutex);
    
    int count = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].active) {
            count++;
        }
    }
    
    pthread_mutex_unlock(&voice_mutex);
    return count;
}

// Legacy function for compatibility
int audio_engine_play_sample(audio_sample_t *sample) {
    return audio_engine_trigger_sample(sample, 1.0f);
}

// Set master gain
int audio_engine_set_master_gain(float gain) {
    if (gain < 0.0f || gain > 1.0f) {
        return -1;
    }
    
    engine_config.master_gain = gain;
    return 0;
}

// Get master gain
float audio_engine_get_master_gain(void) {
    return engine_config.master_gain;
}

// Free audio sample
void audio_sample_free(audio_sample_t *sample) {
    if (sample) {
        if (sample->data) {
            free(sample->data);
        }
        free(sample);
    }
}

// Clone audio sample
audio_sample_t* audio_sample_clone(audio_sample_t *sample) {
    if (!sample) {
        return NULL;
    }
    
    audio_sample_t *clone = malloc(sizeof(audio_sample_t));
    if (!clone) {
        return NULL;
    }
    
    *clone = *sample;
    
    size_t data_size = sample->frames * sample->channels * sizeof(float);
    clone->data = malloc(data_size);
    if (!clone->data) {
        free(clone);
        return NULL;
    }
    
    memcpy(clone->data, sample->data, data_size);
    return clone;
}

// Print engine statistics
void audio_engine_print_stats(void) {
    printf("Audio Engine Statistics:\n");
    printf("  Total frames processed: %lu\n", total_frames_processed);
    printf("  Active voices: %d\n", last_active_voices);
    printf("  Master gain: %.2f\n", engine_config.master_gain);
    printf("  Auto gain control: %s\n", engine_config.auto_gain_control ? "enabled" : "disabled");
    
    if (jack_client_is_active()) {
        printf("  JACK sample rate: %d Hz\n", jack_client_get_sample_rate());
        printf("  JACK buffer size: %d frames\n", jack_client_get_buffer_size());
    }
}

// Get CPU load (placeholder)
int audio_engine_get_cpu_load(void) {
    // Could implement actual CPU monitoring here
    return last_active_voices * 10; // Rough estimate
}
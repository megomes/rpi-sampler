#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sndfile.h>
#include "sample_loader.h"

// Internal state
static char samples_directory[512] = "";
static audio_sample_t *first_sample = NULL;
static char first_sample_name[256] = "";
static int sample_count = 0;
static int loader_initialized = 0;

// Helper function to check if file is a WAV file
static int is_wav_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return 0;
    
    const char *ext = filename + len - 4;
    return (strcasecmp(ext, ".wav") == 0);
}

// Load a WAV file into memory
static audio_sample_t* load_wav_file(const char *filepath) {
    SF_INFO info;
    SNDFILE *file;
    audio_sample_t *sample = NULL;
    
    // Initialize info structure
    memset(&info, 0, sizeof(info));
    
    // Open file
    file = sf_open(filepath, SFM_READ, &info);
    if (!file) {
        printf("Error opening WAV file '%s': %s\n", filepath, sf_strerror(NULL));
        return NULL;
    }
    
    printf("Loading WAV file: %s\n", filepath);
    printf("  Frames: %ld\n", info.frames);
    printf("  Channels: %d\n", info.channels);
    printf("  Sample Rate: %d Hz\n", info.samplerate);
    printf("  Format: 0x%08X\n", info.format);
    
    // Allocate sample structure
    sample = malloc(sizeof(audio_sample_t));
    if (!sample) {
        printf("Error allocating memory for sample structure\n");
        sf_close(file);
        return NULL;
    }
    
    // Fill sample information
    sample->frames = info.frames;
    sample->channels = info.channels;
    sample->sample_rate = info.samplerate;
    
    // Allocate memory for audio data
    size_t data_size = info.frames * info.channels * sizeof(float);
    sample->data = malloc(data_size);
    if (!sample->data) {
        printf("Error allocating memory for sample data (%zu bytes)\n", data_size);
        free(sample);
        sf_close(file);
        return NULL;
    }
    
    // Read audio data
    sf_count_t frames_read = sf_readf_float(file, sample->data, info.frames);
    if (frames_read != info.frames) {
        printf("Warning: Read %ld frames, expected %ld\n", frames_read, info.frames);
        sample->frames = frames_read;
    }
    
    sf_close(file);
    
    printf("  Successfully loaded %d frames of audio data\n", sample->frames);
    return sample;
}

// Scan directory for WAV files and load the first one
static int scan_and_load_first_sample(void) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char filepath[768];
    int found = 0;
    
    dir = opendir(samples_directory);
    if (!dir) {
        printf("Error opening samples directory: %s\n", samples_directory);
        return -1;
    }
    
    printf("Scanning directory: %s\n", samples_directory);
    
    sample_count = 0;
    
    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files and directories
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        // Check if it's a WAV file
        if (!is_wav_file(entry->d_name)) {
            continue;
        }
        
        // Build full path
        snprintf(filepath, sizeof(filepath), "%s/%s", samples_directory, entry->d_name);
        
        // Check if it's a regular file
        if (stat(filepath, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
            continue;
        }
        
        sample_count++;
        printf("Found WAV file: %s\n", entry->d_name);
        
        // Load the first WAV file found
        if (!found) {
            first_sample = load_wav_file(filepath);
            if (first_sample) {
                strncpy(first_sample_name, entry->d_name, sizeof(first_sample_name) - 1);
                first_sample_name[sizeof(first_sample_name) - 1] = '\0';
                found = 1;
                printf("Loaded first sample: %s\n", first_sample_name);
            }
        }
    }
    
    closedir(dir);
    
    printf("Total WAV files found: %d\n", sample_count);
    
    if (sample_count == 0) {
        printf("No WAV files found in directory: %s\n", samples_directory);
        return -1;
    }
    
    if (!found) {
        printf("Failed to load any samples\n");
        return -1;
    }
    
    return 0;
}

// Initialize sample loader
int sample_loader_init(const char *samples_dir) {
    if (loader_initialized) {
        printf("Sample loader already initialized\n");
        return 0;
    }
    
    if (!samples_dir) {
        printf("Error: samples directory not specified\n");
        return -1;
    }
    
    // Copy directory path
    strncpy(samples_directory, samples_dir, sizeof(samples_directory) - 1);
    samples_directory[sizeof(samples_directory) - 1] = '\0';
    
    printf("Initializing sample loader with directory: %s\n", samples_directory);
    
    // Check if directory exists
    struct stat dir_stat;
    if (stat(samples_directory, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode)) {
        printf("Error: samples directory does not exist or is not a directory: %s\n", samples_directory);
        return -1;
    }
    
    // Scan directory and load first sample
    if (scan_and_load_first_sample() < 0) {
        return -1;
    }
    
    loader_initialized = 1;
    printf("Sample loader initialized successfully\n");
    
    return 0;
}

// Cleanup sample loader
void sample_loader_cleanup(void) {
    if (first_sample) {
        audio_sample_free(first_sample);
        first_sample = NULL;
    }
    
    samples_directory[0] = '\0';
    first_sample_name[0] = '\0';
    sample_count = 0;
    loader_initialized = 0;
    
    printf("Sample loader cleaned up\n");
}

// Get first sample
audio_sample_t* sample_loader_get_first_sample(void) {
    return first_sample;
}

// Get first sample name
const char* sample_loader_get_first_sample_name(void) {
    return loader_initialized ? first_sample_name : "not initialized";
}

// Get samples directory
const char* sample_loader_get_samples_directory(void) {
    return loader_initialized ? samples_directory : "not initialized";
}

// Get sample count
int sample_loader_get_sample_count(void) {
    return loader_initialized ? sample_count : 0;
}

// List all samples
void sample_loader_list_samples(void) {
    if (!loader_initialized) {
        printf("Sample loader not initialized\n");
        return;
    }
    
    printf("Samples directory: %s\n", samples_directory);
    printf("Total samples found: %d\n", sample_count);
    
    if (first_sample) {
        printf("First sample loaded: %s\n", first_sample_name);
        printf("  Frames: %d\n", first_sample->frames);
        printf("  Channels: %d\n", first_sample->channels);
        printf("  Sample Rate: %d Hz\n", first_sample->sample_rate);
    } else {
        printf("No samples loaded\n");
    }
}

// Check if initialized
int sample_loader_is_initialized(void) {
    return loader_initialized;
}
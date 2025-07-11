#ifndef SAMPLE_LOADER_H
#define SAMPLE_LOADER_H

#include "audio_engine.h"

// Sample loader functions
int sample_loader_init(const char *samples_dir);
void sample_loader_cleanup(void);

// Sample access functions
audio_sample_t* sample_loader_get_first_sample(void);
const char* sample_loader_get_first_sample_name(void);
const char* sample_loader_get_samples_directory(void);

// Sample discovery functions
int sample_loader_get_sample_count(void);
void sample_loader_list_samples(void);

// Utility functions
int sample_loader_is_initialized(void);

#endif // SAMPLE_LOADER_H
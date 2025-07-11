#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

void list_midi_devices() {
    snd_seq_t *seq_handle;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int client;
    int err;

    // Open ALSA sequencer
    err = snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (err < 0) {
        printf("Error opening ALSA sequencer: %s\n", snd_strerror(err));
        return;
    }

    // Allocate client and port info structures
    snd_seq_client_info_malloc(&cinfo);
    snd_seq_port_info_malloc(&pinfo);

    printf("=== USB MIDI Devices Connected ===\n\n");

    // Iterate through all clients
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq_handle, cinfo) >= 0) {
        client = snd_seq_client_info_get_client(cinfo);
        
        // Skip system clients (0, 14, 15)
        if (client == 0 || client == 14 || client == 15) {
            continue;
        }

        // Check if this client has any ports
        int port_count = 0;
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);
        
        while (snd_seq_query_next_port(seq_handle, pinfo) >= 0) {
            unsigned int capability = snd_seq_port_info_get_capability(pinfo);
            
            // Only show hardware MIDI ports
            if (!(capability & SND_SEQ_PORT_CAP_NO_EXPORT)) {
                if (port_count == 0) {
                    printf("Client %d: %s\n", client, snd_seq_client_info_get_name(cinfo));
                }
                
                int port = snd_seq_port_info_get_port(pinfo);
                const char* port_name = snd_seq_port_info_get_name(pinfo);
                
                printf("  Port %d: %s", port, port_name);
                
                // Show capabilities
                if (capability & SND_SEQ_PORT_CAP_READ) {
                    printf(" [OUTPUT]");
                }
                if (capability & SND_SEQ_PORT_CAP_WRITE) {
                    printf(" [INPUT]");
                }
                
                printf("\n");
                printf("    Device: hw:%d,%d,0\n", client, port);
                
                port_count++;
            }
        }
        
        if (port_count > 0) {
            printf("\n");
        }
    }

    // Free allocated memory
    snd_seq_client_info_free(cinfo);
    snd_seq_port_info_free(pinfo);
    snd_seq_close(seq_handle);
}

int main() {
    printf("Raspberry Pi MIDI Sampler - MIDI Device Scanner\n");
    printf("================================================\n\n");
    
    list_midi_devices();
    
    printf("Copy the 'hw:X,Y,0' device string to your sampler_config_file.txt\n");
    printf("Example: midi_device = hw:2,0,0\n\n");
    
    return 0;
}
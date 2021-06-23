// ====================================================================================
//
// Author: Zack Kollar 2021
//
// ====================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// ====================================================================================

#include <portsf.h>

// ====================================================================================
// Declarations
// ====================================================================================

// Global methods
static void usage();

// The audio file structure
struct audio_file {
    PSF_PROPS   properties;
    const char *file_name;
    int         file;
};

// Audio file structure methods
struct audio_file *audio_file_new               (const char *infile           );
void               audio_file_info              (struct audio_file *audio_file);
static inline bool audio_file_valid_sample_type (PSF_PROPS  *properties       );

// Local methods
float get_max_gain   (struct audio_file *audio_file);
void  process_signal (struct audio_file *audio_file);

// ====================================================================================

/**
 * @brief ENTRYPOINT
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {
    // PortSF initialization
    psf_init();

    // Values to be set by arguments
    char *infile = NULL;
    float gain = 1.0f;

    // ==================================
    // Notes about getopt:
    // ==================================
    // getopt flags are delineated by :
    // Things below no longer make sense...
    // first is required
    // second is optional
    // third is something?? // TODO: figure this out
    // ==================================

    // Parse command line arguments with ancient MAGIC
    char flag; // Current flag for the option parsing
    while ((flag = getopt(argc, argv, "f:g:")) != -1) {
        switch(flag) {
            case 'f':
                infile = optarg;
                break; // NOTE TO SELF: Forgot this and thought I was losing my mind
            case 'g':
                if(optarg) { // How to handle an optional field postfix'd with :
                    gain = atof(optarg);
                }
                break;
            case '?':
                usage();
                return 1;
        }
    }
    
    // Is a file specified?
    if(!infile) {
        printf("No file to load specified, use the -f flag\n");
        usage();
        return 1;
    }

    // Open the file from the CLI args
    struct audio_file *audio_file = audio_file_new(infile);
    if(audio_file == NULL) {
        return 1;
    }

    // Print audio file info
    audio_file_info(audio_file);
    printf("Adjusted Gain: %f\n", gain);

    // Shutdown PortSF
    if (psf_finish() != 0) {
        printf("Failed to clean up portsf...");
        return 1;
    }

    // Destroy the pointer to the audio file
    free(audio_file);

    return 0;
}

// ====================================================================================

/**
 * @brief Create a new audio_file struct
 * 
 * @param infile 
 * @return struct audio_file* 
 */
struct audio_file *audio_file_new(const char *infile) {
    // Create the audio file struct
    struct audio_file *audio_file = malloc(sizeof(struct audio_file));
    if(audio_file == NULL) return NULL;

    // Setup the audio file
    audio_file->file_name = infile;
    audio_file->file = psf_sndOpen(infile, &audio_file->properties, 0); // Open the sound file
    if(audio_file->file < 0) {
        printf("Cannot open file: %s\n", infile);
        return NULL;
    }

    return audio_file;
}

/**
 * @brief Print audio file information
 * 
 * @param audio_file 
 */
void audio_file_info(struct audio_file *audio_file) {
    printf("File: %s\n",          audio_file->file_name       );
    printf("Sample Rate: %d\n",   audio_file->properties.srate);
    printf("Channel Count: %d\n", audio_file->properties.chans);
}

// ====================================================================================

static void usage() {
    printf("audio-gain -f <FILE_NAME> [-g <GAIN>]\n");
}

// ====================================================================================

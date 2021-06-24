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
#include <stddef.h>
#include <stdint.h>

// ====================================================================================

#include <portsf.h>

// ====================================================================================

size_t FRAME_BUFFER_SIZE = 128; 

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
struct audio_file *audio_file_open   (const char *infile                                   );
struct audio_file *audio_file_create (struct audio_file *audio_file_in, const char *outfile);
void               audio_file_info   (struct audio_file *audio_file                        );
static inline int  audio_file_free   (struct audio_file *audio_file                        );

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
    if(psf_init()) {
        printf("Failed to start portsf...");
        return 1;
    }

    // Values to be set by arguments
    bool debug = false;
    char *infile = NULL, *outfile = NULL;
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
    while ((flag = getopt(argc, argv, "i:o:g:d")) != -1) {
        switch(flag) {
            case 'i':
                infile = optarg;
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'd':
                debug = true;
                break;
            case 'g':
                if(optarg) { // How to handle an optional field postfix'd with :
                    gain = atof(optarg);
                }
                break;
            case '?':
                printf("Invalid flag: %s\n", optarg);
                usage();
                return 1;
        }
    }
    
    if(!infile) {
        printf("No file to load specified, use the -i flag\n");
        usage();
        return 1;
    }
    if(!outfile) {
        printf("No file to create specified, use the -o flag\n");
        usage();
        return 1;
    }

    // Open the file from the CLI args
    struct audio_file *audio_file = audio_file_open(infile);
    if(audio_file == NULL) {
        return 1;
    }

    // Print audio file info
    if(debug) {
        audio_file_info(audio_file);
        printf("Adjusted Gain: %f\n", gain);
        printf("Output file: %s\n", outfile);
    }

    // Open a new file
    struct audio_file *audio_file_out = audio_file_create(audio_file, outfile);
    if(audio_file_out == NULL) {
        return 1;
    }


    // Prepare to process the audio data
    float *frame = malloc(audio_file->properties.chans * sizeof(float));
    long total_frames = 0;

    // Read each frame
    while(psf_sndReadFloatFrames(audio_file->file, frame, 1) == 1) {
        total_frames++;

        // For each channel of the frame
        for(int i = 0; i < audio_file->properties.chans; i++) {
            // TODO: Process here
            frame[i] *= gain;
        }

        // Write our frames to the output file
        if(psf_sndWriteFloatFrames(audio_file_out->file, frame, 1) != 1) {
            printf("Failed to write to outfile %s\n", outfile);
            break;
        }
    }

    free(frame);

    // Close the files
    if(audio_file_free(audio_file) != 0) {
        printf("Failed to close file %s\n", infile);
        audio_file_free(audio_file_out);
        return 1;
    }
    if(audio_file_free(audio_file_out) != 0) {
        printf("Failed to close file %s\n", infile);
        return 1;
    }

    // Shutdown PortSF
    if (psf_finish() != 0) {
        printf("Failed to clean up portsf...");
        return 1;
    }

    return 0;
}

// ====================================================================================

/**
 * @brief Create a new audio_file struct
 * 
 * @param infile 
 * @return struct audio_file* 
 */
struct audio_file *audio_file_open(const char *infile) {
    // Create the audio file struct
    struct audio_file *audio_file = malloc(sizeof(struct audio_file));
    if(audio_file == NULL) return NULL;

    // Setup the existing audio file
    audio_file->file_name = infile;
    audio_file->file = psf_sndOpen(infile, &audio_file->properties, 0); // Open the sound file
    if(audio_file->file < 0) {
        printf("Cannot open file: %s\n", infile);
        free(audio_file);
        return NULL;
    }

    return audio_file;
}

struct audio_file *audio_file_create(struct audio_file *audio_file_in, const char *outfile) {
    // Create the audio file struct
    struct audio_file *audio_file = malloc(sizeof(struct audio_file));
    if(audio_file == NULL) return NULL;

    // Setup the new audio file
    audio_file->file_name = outfile;
    audio_file->properties = audio_file_in->properties;
    audio_file->properties.samptype = PSF_SAMP_IEEE_FLOAT;
    audio_file->properties.format = psf_getFormatExt(outfile);

    if(audio_file->properties.format == PSF_FMT_UNKNOWN) {
        printf("Unrecognized file format (use .aiff or .wav): %s\n", outfile);
        goto fail;
    }

    audio_file->file = psf_sndCreate("./test.wav", &audio_file->properties, 0, 0, PSF_CREATE_RDWR); // Create a sound file
    if(audio_file->file < 0) {
        printf("Cannot create file: %s\n", outfile);
        goto fail;
    }

    return audio_file;

fail:
    free(audio_file);
    return NULL;
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

/**
 * @brief Free the audio file
 * 
 * @param audio_file 
 */
static inline int audio_file_free(struct audio_file *audio_file) {
    int result = psf_sndClose(audio_file->file);
    free(audio_file);
    return result;
}


// ====================================================================================

static void usage() {
    printf("audio-gain -f <FILE_NAME> [-g <GAIN> | -d <DEBUG> ]\n");
}

// ====================================================================================

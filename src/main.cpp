
#include <time.h>
#include <errno.h>
#include <signal.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>
// #include <pthread.h>

#include "types.h"
#include "main.h"
#include "cpu/cpu.h"
#include "dev.h"
#include "cpu/mem.h"
#include "cpu/ops.h"

#include "gui/win/opencon.h"

int console_enabled = 1;
int video_enabled = 1;
int verbosity = 0;
int dump_on_quit = 0;
int dump_bank_number = 0;

#include <SDL3/SDL.h>
#include "video.h"

SDL_Thread* ChasmCPU;
SDL_Thread* ChasmVideo;
SDL_Thread* ChasmInput;

#define DEFAULT_ROM "C:\\Lethe\\rom\\test.rom"

octet* load_file(const char* filename, regt& filesize) {
    FILE* f = fopen(filename, "rb");
    
    if(f == NULL) {
		fprintf(stderr, "Could not open file: %s\n", filename);
	} else {
        fseek(f, 0, SEEK_END);
		fpos_t pos;
		fgetpos64(f, &pos);
        filesize = pos;
        fpos_t file_length = pos;
		// printf(" -- file length: %i\n", pos);
        octet* filedata = (octet*)malloc_aligned(file_length);
        fseek(f, 0, 0);

		fread(filedata, sizeof(octet), file_length, f);
		fclose(f);
        return filedata;
    }
    return NULL;
}

int cpu_return = 0;
int cpu_priority = 1;

int write_dump(const int bank, const char* filename) {
    FILE* f = fopen(filename, "wb");
    if(f == NULL) {
        fprintf(stderr, " -- could not open output file: %s\n", filename);
        return 1;
    } else {
        for(regt o = 0; o < mem_size[bank] * 2; ++o) {
            octet b = mem_bank[bank][o];
            fwrite(&b, sizeof(octet), 1, f);
        }
        fclose(f);
        return 0;
    }
}

void hang() {
    fprintf(stderr, "Press RETURN to quit.\n");
    char* buffer = (char*)malloc(sizeof(char) * 32);
    for(memt bi = 0; bi < 32; ++bi)
        buffer[bi] = 0;
    fgets(buffer, 31, stdin);
}

static int cpu(void *p) {
    /* int conc = pthread_set_concurrency(cpu_priority);
    fprintf(stderr, "CPU priority set to: %d, result = %d\n", cpu_priority, conc); */
    /* bool conc = SDL_SetThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL);
    if(!conc)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not set CPU thread priority: %s", SDL_GetError()); */
    
    init_cpu();
    hang();

    if(dump_on_quit) {
        printf(" -- dumping memory bank %u...\n", dump_bank_number);
        write_dump(dump_bank_number, "mem.dmp");
    }

    return cpu_return;
}

int main(int argc, char* argv[]) {
    /*
    int argi = 1;
    printf(" -- invoked with arguments:");
    while(argi < argc) {
        printf(" %s", argv[argi]);
        ++argi;
    }
    if(argc == 1)
        printf(" (none)");
    printf("\n");
    */
    int no_rom = 0;
	int argi = 1;
	while(argi < argc) {
		int argj = 0;
		if(argv[argi][0] == ':')
			argj = 1;
		else if(argv[argi][0] == '-' && argv[argi][1] == '-')
			argj = 2;
		
		char* chars = argv[argi] + argj;
		if(argj) {
            if(strcmp(chars, "rom") == 0) {
                if(argi < argc - 1) {
                    rom_path = argv[argi + 1];
                    ++argi;
                } else {
                    no_rom = 1;
                }
            /*} else if(strcmp(chars, "priority") == 0) {
                if(argi < argc - 1) {
                    cpu_priority = strtoull(argv[argi + 1], NULL, 0);
                    ++argi;
                }*/
            } else if((strcmp(chars, "d") == 0) || (strcmp(chars, "diagnostic-metrics") == 0)) {
                printf("Size of register: %u bytes\n", sizeof(regt));
                printf("Size of memory word: %u bytes\n", sizeof(memt));
                printf("Size of byte: %u byte(s)\n", sizeof(octet));
                printf("Size of double word: %u bytes\n", sizeof(dwordt));
                printf("Size of real: %u bytes\n", sizeof(fregt));
                printf("Size of opfunc pointer: %u bytes\n", sizeof(opfunc*));
                printf("Size of operation table entry: %u bytes\n", sizeof(struct opt));
                printf("Total registers: %u\n", REG_COUNT);
                return 0;
            } else if((strcmp(chars, "v") == 0) || (strcmp(chars, "verbose") == 0)) {
                verbosity = 1;
            } else if((strcmp(chars, "vv") == 0) || (strcmp(chars, "narrative") == 0)) {
                verbosity = 2;
            } else if((strcmp(chars, "vvv") == 0) || (strcmp(chars, "handcrank") == 0)) {
                verbosity = 3;
            } else if((strcmp(chars, "dump") == 0)) {
                dump_on_quit = 1;
                if(argi < argc - 1) {
                    dump_bank_number = strtoul(argv[argi + 1], NULL, 0);
                    ++argi;
                }
            } else if(strcmp(chars, "no-video") == 0) {
                video_enabled = 0;
            } else if(strcmp(chars, "no-console") == 0) {
                console_enabled = 0;
            }
		}
		++argi;
	}
    
    #ifdef WIN32_MODE
        Win_OpenCon();
    #endif
    
    puts("chasm " VERSION " built " __DATE__ " " __TIME__ "\n");
	
    if(rom_path == NULL || no_rom) {
        rom_path = DEFAULT_ROM;
        if(verbosity)
            printf(" -- no ROM specified; using default (%s)\n", rom_path);
    }

    rom_image = load_file(rom_path, rom_size);
    if(rom_size % 2)
        fprintf(stderr, " -- warning: ROM is not 16-bit\n");
    
    fprintf(stderr, " -- loaded ROM %s (%llu bytes)\n", rom_path, rom_size);
    rom_size >>= 1;
    if(verbosity)
        printf(" -- loaded %s (%llu words)\n", rom_path, (regt)rom_size);

    init_memory();
    create_default_interrupts();
    create_device_table();

    int ret_video, ret_cpu;
    //pthread_t thread_video, thread_cpu;

    /* ChasmCPU = SDL_CreateThread(cpu, "CPUThread", NULL);

    if(ChasmCPU == NULL)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CPU thread failed to initialize: %s", SDL_GetError()); */

    if(video_enabled) {
        ChasmVideo = SDL_CreateThread(video, "VideoThread", NULL);
        if(ChasmVideo == NULL)
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Video thread failed to initialize: %s", SDL_GetError());
    }

    cpu_return = cpu(NULL);

    /*
    if(ChasmCPU != NULL)
        SDL_WaitThread(ChasmCPU, &ret_cpu); */
    
    int video_return = 0;
    if(ChasmVideo != NULL)
        SDL_WaitThread(ChasmVideo, &video_return);

    /*
    if(video_enabled) {
        // ret_video = pthread_create(&thread_video, NULL, video, NULL);
        
    }
    //pthread_attr_t cpu_attr;
    //int retcpuval = pthread_attr_init(&cpu_attr);
    //ret_cpu = pthread_create(&thread_cpu, &cpu_attr, cpu, NULL);
    
    if(video_enabled)
        pthread_join(thread_video, NULL);
    pthread_join(thread_cpu, NULL);
    pthread_attr_destroy(&cpu_attr);

    if(video_enabled)
        printf(" -- video thread returns: %d\n", thread_video);
    printf(" -- CPU thread returns: %d\n", thread_cpu);
    */
    
    exit(video_return + cpu_return);
}
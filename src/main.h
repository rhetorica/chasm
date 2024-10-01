#ifndef MAIN_H
#define MAIN_H

#define VERSION "4"

extern int video_enabled;
extern int console_enabled;

extern int video_return;
extern int cpu_return;

extern int dump_on_quit;
extern int dump_bank_number;

void hang();
void init_cpu();
void dump_registers();

#endif // MAIN_H

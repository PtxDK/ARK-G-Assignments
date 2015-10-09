#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "mips32.h"
#include "elf.h"

#define ERROR_INVALID_ARGS (1)
#define ERROR_IO_ERROR (1)
#define MEMSZ (640*1024)
#define FOURTH_LAST (MIPS_RESERVE-4-1+MEMSZ)


// Declarations
static uint32_t regs[32];
static uint32_t pc;
static size_t instr_cnt;
int read_config (const char *path);
int read_config_stream (FILE * fp);
int interp();
int show_status ();
unsigned char mem[MEMSZ];



// Main
int main(int argc, char *argv[]) {
  int res;
  //stub
  pc = pc;

  









  if (argc != 3) {
    printf("Wrong number of argument.\nsim CONFIG_FILE DEST_FILE\n");
    return (ERROR_INVALID_ARGS);
  }  
  
  res = read_config(argv[1]);

  if((elf_dump(argv[2], &pc, &mem[0], MEMSZ))!= 0){
      return (ERROR_IO_ERROR);   
    }

  //sætter min SP (som ligger  regs[29]) til fjerde sidste i memory.
  SET_BIGWORD(mem, regs[29], FOURTH_LAST);

  printf("Hello, World!\n");
  
  show_status();
  
  return res;
   
}


// open and close the startup file
int read_config (const char *path){
  FILE *fdesc = fopen(path, "r");

  if( fdesc == NULL){ // if the file is empty.
 return (ERROR_IO_ERROR);
  }
  else{
    read_config_stream(fdesc); // if the file is not empty read it.

    if (fclose(fdesc) != 0) //if the file failed to close.
    return (ERROR_IO_ERROR);
  }

  return read_config_stream(fdesc);
}

//read the startup file
int read_config_stream (FILE * fp){
  int i = 0;
  uint32_t v;

    for( i = 0 ; i <= 8; i++ )
      {	
	if (fscanf(fp, "%u", &v) != 1 ){
	  return (ERROR_IO_ERROR);
	}
	regs[8+i] = v;
      }
    return 0;
}

int show_status(){
  printf("Executed %zu instructions", instr_cnt);
  printf("pc = 0x%x \n", pc);
  printf("at = 0x%x \n", pc);
  printf("v0 = 0x%x \n", regs[2]);
  printf("v1 = 0x%x \n", regs[3]);
  printf("t0 = 0x%x \n", regs[8]);
  printf("t1 = 0x%x \n", regs[9]);
  printf("t2 = 0x%x \n", regs[10]);
  printf("t3 = 0x%x \n", regs[11]);
  printf("t4 = 0x%x \n", regs[12]);
  printf("t5 = 0x%x \n", regs[13]);
  printf("t6 = 0x%x \n", regs[14]);
  printf("t7 = 0x%x \n", regs[15]);
  printf("sp = 0x%x \n", regs[29]);
  printf("ra = 0x%x \n", regs[31]);
  return 0;
}

int interp(){

  return 0;
}

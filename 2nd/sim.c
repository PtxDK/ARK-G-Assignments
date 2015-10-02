#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mips32.h"
#include "elf.h"


//Defines macros needed by the system.
#define ERROR_INVALID_ARGS (-1)
#define ERROR_IO_ERROR (-2)
#define ERROR_UNKNOWN_OPCODE (-3)
#define ERROR_UNKNOWN_FUNCT (-4)
#define MEMSZ (640*1024)
#define FOURTH_LAST (MIPS_RESERVE-4+MEMSZ)
#define STATUS_SYSCALL (1)
#define STATUS_ERROR (2)
#define SAW_SYSCALL (3)



// Declarations
static uint32_t regs[32];
uint32_t pc;
int alu_return;
static size_t instr_counter;
static size_t cycles;
uint32_t adress;
unsigned char mem[MEMSZ];
int read_config (const char *path);
int read_config_stream (FILE * fp);
int interp();
int show_status ();
int interp_instr(uint32_t inst);
int interp_r(uint32_t inst);
int interp_j(uint32_t inst);
int interp_jal(uint32_t inst);
int interp_beq(uint32_t inst);
int interp_addiu(uint32_t inst);
int interp_lw(uint32_t inst);
int interp_sw(uint32_t inst);
int interp_syscall(uint32_t inst);
int cycle();
void interp_if();
int interp_control();
int prep_id_ex();
int interp_id();
int alu();
int interp_ex();
void interp_mem();
void interp_wb();
void write_reg();


struct preg_if_id {
  uint32_t inst;
  uint32_t next_pc;
  //...
};
static struct preg_if_id if_id;

struct preg_id_ex {
  bool mem_read; // whether we should read from memory
  bool mem_write; // whether we should write from memory
  bool reg_write; // whether we should write back to a register
  bool mem_to_reg;
  bool alu_src;
  bool branch;
  bool jump;
  uint32_t rt;
  uint32_t rs_value;
  uint32_t rt_value;
  uint32_t sign_ext_imm;
  uint32_t funct;
  int reg_dst;
  int shamt;
  uint32_t next_pc;
  uint32_t jump_target;
};
static struct preg_id_ex id_ex;

struct preg_ex_mem {
  bool mem_read; // whether we should read from memory
  bool mem_write; // whether we should write from memory
  bool reg_write; // whether we should write back to a register
  bool mem_to_reg;
  bool branch;
  uint32_t rt;
  uint32_t rt_value;
  int alu_res;
  int reg_dst;
  uint32_t branch_target;
};

static struct preg_ex_mem ex_mem;

struct preg_mem_wb {
  bool mem_to_reg;
  bool reg_write;
  uint32_t rt;
  uint32_t read_data;
  int alu_res;
  int reg_dst;
};

static struct preg_mem_wb mem_wb;




// Main
int main(int argc, char *argv[]) {
  printf("main start");
  int res;
  res = 0;
  

  // check if the nuber of arguments is correct.
  if (argc != 3) {
    printf("Wrong number of arguments.\nsim CONFIG_FILE DEST_FILE\n");
    return (ERROR_INVALID_ARGS);
  } 


  // Initiate registers.
  read_config(argv[1]);

  // check if we can call elf_dum, if not possible throw error.
  if((elf_dump(argv[2], &pc, &mem[0], MEMSZ))!= 0){
    return (ERROR_IO_ERROR);   
  }

  // sets the SP (that is located in regs[29]) to the 4th last in the memory.
  regs[29]= FOURTH_LAST;

  res = interp();

  //Show status of the registers.
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

// prints the status of the registers.
int show_status(){
  printf("Executed %zu instructions \n", instr_counter);
  printf("pc = 0x%x \n", pc);       // Program Couner
  printf("at = 0x%x \n", pc);       // Assembler Temp
  printf("v0 = 0x%x \n", regs[2]);  // Values for funtion results
  printf("v1 = 0x%x \n", regs[3]);
  printf("t0 = 0x%x \n", regs[8]);  // Temps
  printf("t1 = 0x%x \n", regs[9]);
  printf("t2 = 0x%x \n", regs[10]);
  printf("t3 = 0x%x \n", regs[11]);
  printf("t4 = 0x%x \n", regs[12]);
  printf("t5 = 0x%x \n", regs[13]);
  printf("t6 = 0x%x \n", regs[14]);
  printf("t7 = 0x%x \n", regs[15]);
  printf("sp = 0x%x \n", regs[29]); // Stack Pointer
  printf("ra = 0x%x \n", regs[31]); // Return Address
  return 0;
}


int cycle(){
  printf("PC=%x\n", pc);
  write_reg();
  getchar();
  //calling interp_wb
  interp_wb();
  printf("wb end \n");
  //calling interp_mem
  interp_mem();
  printf("mem end \n");
  //calling interp_ex
  if (interp_ex() != 0){
    return (alu_return);
  }
  printf("ex end \n");
  //calling interp_id
  if(interp_id() != 0){
    return (ERROR_UNKNOWN_OPCODE);
  }
  printf("id end \n");
  //----------------------------------------------------------------------FEJL?!??! gav SAW_SYSCALL?
  //calling interp_if
  interp_if();
  //----------------------------------------------------------------------FEJL?!?!?
  printf("if end \n");
  if ((ex_mem.branch == true) & (ex_mem.alu_res == 0)){
    pc =ex_mem.branch_target;
    if_id.inst = 0;
    instr_counter -= 1; 
  }
  if ((ex_mem.branch == true) & (ex_mem.alu_res != 0)){
    pc =ex_mem.branch_target;
    if_id.inst = 0;
    instr_counter -= 2; 
  }
  if (id_ex.jump == true){
    pc = id_ex.jump_target;
  }
  printf("cycle end \n");
  return 0;
}


//ikke færdig stub
// runs the infinite loop and checks if there is more instructions to do.
int interp(){
  cycles = 0;
  int retval;
    
  while (1){
    retval = cycle();
    if (retval != 0){
      printf ("retval = %d \n", retval);
      return retval;
    }
    cycles ++;
  }
  return 0;
}

//  finds the opcode for a 32 bit mips instruction
int interp_instr(uint32_t inst){
  uint32_t opcode;
  int result;
  opcode = GET_OPCODE(inst);
  // checks which opcode it responds too and runs the appropriate function
  switch(opcode)
    {
    case OPCODE_R :
      result = interp_r(inst);
      if (result != 0)
	return result;

    case OPCODE_J :
      interp_j(inst);
      break;
     
    case OPCODE_JAL :
      interp_jal(inst);
      break;
      
    case OPCODE_BEQ :
      interp_beq(inst);
      break;

    case OPCODE_BNE :
      interp_beq(inst);
      break;
      
    case OPCODE_ADDIU :
      interp_addiu(inst);
      break;
      
    case OPCODE_SLTI :
      interp_beq(inst);
      break;

    case OPCODE_ANDI :
      interp_beq(inst);
      break;

    case OPCODE_ORI :
      interp_beq(inst);
      break;

    case OPCODE_LUI :
      interp_beq(inst);
      break;
      
    case OPCODE_LW :
      interp_lw(inst);
      break;
      
    case OPCODE_SW :
      interp_sw(inst);
      break;      
      
    default : 
      return(ERROR_UNKNOWN_OPCODE);
    }
  return 0;
}

// if the opcode had the R format, it runs this function
int interp_r(uint32_t inst) {
  // finds the funct code for the instruction
  uint32_t f_code;
  f_code = GET_FUNCT(inst);

  // Runs the function corresponding to the f_code
  switch(f_code) {
  case FUNCT_JR :
    pc = regs[GET_RS(inst)]; // Sets pc = value from register
    break;

  case FUNCT_SYSCALL :
    return(STATUS_SYSCALL);

  case FUNCT_ADDU :
    regs[GET_RD(inst)] = regs[GET_RS(inst)]+regs[GET_RT(inst)];
    break;

  case FUNCT_SUBU :
    regs[GET_RD(inst)] = regs[GET_RS(inst)]-regs[GET_RT(inst)];
    break;

  case FUNCT_AND :
    regs[GET_RD(inst)] = regs[GET_RS(inst)] & regs[GET_RT(inst)];
    break;

  case FUNCT_OR :
    regs[GET_RD(inst)] = regs[GET_RS(inst)] | regs[GET_RT(inst)];
    break;

  case FUNCT_NOR :
    regs[GET_RD(inst)] = ~(regs[GET_RS(inst)] | regs[GET_RT(inst)]);
    break;

  case FUNCT_SLT :
    if (regs[GET_RS(inst)] < regs[GET_RT(inst)]) { regs[GET_RD(inst)] = 1;}
    else {regs[GET_RD(inst)] = 0;}
    break;

  case FUNCT_SLL :
    regs[GET_RD(inst)] = regs[GET_RS(inst)] << regs[GET_SHAMT(inst)];
    break;

  case FUNCT_SRL :
    regs[(GET_RD(inst))] = regs[GET_RS(inst)] >> regs[GET_SHAMT(inst)];
    break;
    
  default :
    printf(" at default");
    return(ERROR_UNKNOWN_FUNCT);
  }
  return 0;
}


int interp_j(uint32_t inst) {
  adress = GET_ADDRESS(inst);
  adress = adress << 2;
  pc = (pc & MS_4B) | adress;
  return 0;
}

int interp_jal(uint32_t inst) {
  regs[31] = pc + 4;
  
  adress = GET_ADDRESS(inst);
  adress = adress << 2;
  pc = (pc & MS_4B) | adress;
  return 0;
}

int interp_beq(uint32_t inst) {
  if (GET_RS(inst) == GET_RT(inst)){
    pc = pc + (SIGN_EXTEND(GET_IMM(inst)) << 2);
  }
  return 0;
}

int interp_bne(uint32_t inst) {
  if (GET_RS(inst) != GET_RT(inst)){
    pc = pc + (SIGN_EXTEND(GET_IMM(inst)) << 2);
  } 
  return 0;
}

int interp_lw(uint32_t inst) {
  regs[GET_RT(inst)] = GET_BIGWORD(mem, regs[GET_RS(inst)] + SIGN_EXTEND(GET_IMM(inst)));
  return 0;
}

int interp_sw(uint32_t inst) {
  SET_BIGWORD(mem, regs[GET_RS(inst)] + SIGN_EXTEND(GET_IMM(inst)), regs[GET_RT(inst)]);
  return 0;
}

int interp_addiu(uint32_t inst) {
  regs[GET_RT(inst)]= regs[GET_RS(inst)] + SIGN_EXTEND(GET_IMM(inst)); 
  return 0;
}


int interp_andi(uint32_t inst) {
  regs[GET_RT(inst)] = regs[GET_RS(inst)] & ZERO_EXTEND(GET_IMM(inst));
  return 0;
}


int interp_lui(uint32_t inst) {
  regs[GET_RT(inst)] = GET_IMM(inst) << 16;
  return 0;
}

int interp_ori(uint32_t inst) {
  regs[GET_RT(inst)] = regs[GET_RS(inst)] | ZERO_EXTEND(GET_IMM(inst));
  return 0;
}


int interp_slti(uint32_t inst) {
  uint32_t signed_extended_immidiate;
  signed_extended_immidiate = SIGN_EXTEND(GET_IMM(inst));

  if (regs[GET_RS(inst)] < signed_extended_immidiate){
    regs[GET_RT(inst)] = 1;
  }
  else regs[GET_RT(inst)] = 0;
  return 0;
}

// interp instruction fetch
void interp_if(){
  //printf("in if \n");
  if_id.inst = GET_BIGWORD(mem, pc);
  //printf("inst set \n");
  pc += 4;
  if_id.next_pc = pc;
  //printf("pc incrementet \n");
  instr_counter++;
  //printf("instr counter incrementet");
}

//prepping the id_ex struct.
int prep_id_ex(){
  id_ex.rt = GET_RT(if_id.inst);
  id_ex.rs_value = regs[GET_RS(if_id.inst)];
  id_ex.rt_value = regs[GET_RT(if_id.inst)];
  id_ex.sign_ext_imm = SIGN_EXTEND(GET_IMM(if_id.inst));
  return 0;
}

// controls everything
int interp_control(){
  uint32_t opcode;
  uint32_t tempJ_adress;
  opcode = GET_OPCODE(if_id.inst);

  switch(opcode)
    {
    case OPCODE_LW :
      id_ex.mem_read = true;
      id_ex.mem_write = false;
      id_ex.reg_write = true;
      id_ex.alu_src = true;
      id_ex.mem_to_reg = true;
      id_ex.branch = false;
      id_ex.jump = false;
      id_ex.funct = FUNCT_ADD;
      id_ex.reg_dst = GET_RT(if_id.inst);
      break;
	
    case OPCODE_SW :
      id_ex.mem_read = false;
      id_ex.mem_write = true;
      id_ex.reg_write = false;
      id_ex.alu_src = true;
      id_ex.mem_to_reg = false;
      id_ex.branch = false;
      id_ex.jump = false;
      id_ex.funct = FUNCT_ADD;
      break;

    case OPCODE_R :
      id_ex.mem_read = false;
      id_ex.mem_write = false;
      id_ex.reg_write = true;
      id_ex.alu_src = false;
      id_ex.mem_to_reg = false;
      id_ex.branch = false;
      id_ex.jump = false;
      id_ex.funct = GET_FUNCT(if_id.inst);
      id_ex.reg_dst = GET_RD(if_id.inst);
      break;
	
    case OPCODE_BEQ :
      id_ex.mem_read = false;
      id_ex.mem_write = false;
      id_ex.reg_write = false;
      id_ex.alu_src = false;
      id_ex.branch = true;
      id_ex.jump = false;
      id_ex.funct = FUNCT_SUB;
      break;

    case OPCODE_BNE :
      id_ex.mem_read = false;
      id_ex.mem_write = false;
      id_ex.reg_write = false;
      id_ex.alu_src = false;
      id_ex.branch = true;
      id_ex.jump = false;
      id_ex.funct = FUNCT_SUB;
      break;

    case OPCODE_J :
      id_ex.mem_read = false;
      id_ex.mem_write = false;
      id_ex.reg_write = false;
      id_ex.alu_src = false;
      id_ex.branch = false;
      id_ex.jump = true;
      tempJ_adress = (GET_ADDRESS(if_id.inst));
      tempJ_adress = tempJ_adress << 2;
      id_ex.jump_target = (tempJ_adress) | (if_id.next_pc & MS_4B);
      break;
      

      // HER FEJLER! når ikke videre end ex.
    case OPCODE_JAL :
      id_ex.mem_read = false;
      id_ex.mem_write = false;
      id_ex.reg_write = true;
      id_ex.alu_src = false;
      id_ex.branch = false;
      id_ex.jump = true;
      id_ex.funct = FUNCT_ADD;
      id_ex.rs_value = 0;
      id_ex.rt_value = pc;
      tempJ_adress = (GET_ADDRESS(if_id.inst));
      tempJ_adress = tempJ_adress << 2;
      id_ex.jump_target = (tempJ_adress) | (if_id.next_pc & MS_4B);
      break;


    default :
      return (ERROR_UNKNOWN_OPCODE);
    }
  return 0;
  
}

// indførte if else check, pludselig intet uendelig loop, dog ingen fejl?
int interp_id(){
  prep_id_ex();
  id_ex.shamt = GET_SHAMT(if_id.inst);
  id_ex.next_pc = if_id.next_pc;
  if (interp_control() == 0){
    return 0;
  }
  else {
    return (ERROR_UNKNOWN_OPCODE);
  }
}

int alu(){
  uint32_t second_op;

  if (id_ex.alu_src == true){
    second_op = id_ex.sign_ext_imm;
  }
  else {
   second_op = id_ex.rt_value;
  }

  // checks which opcode it responds too and runs the appropriate function
  switch(id_ex.funct)
    {

    case FUNCT_ADD :
      ex_mem.alu_res = id_ex.rs_value + second_op;
      break;

    case FUNCT_SYSCALL :
      printf("SYSCALL!");
      return SAW_SYSCALL;

    case FUNCT_ADDU :
      ex_mem.alu_res = id_ex.rs_value + second_op;
      break;

    case FUNCT_NOR :
      ex_mem.alu_res = ~(id_ex.rs_value | second_op);
      break;
      
    case FUNCT_OR :
      ex_mem.alu_res = id_ex.rs_value | second_op;
	break;

    case FUNCT_SLL :
      ex_mem.alu_res = second_op << id_ex.shamt;

    case FUNCT_SLT :
      if (id_ex.rs_value < second_op){ex_mem.alu_res = 1;}
      else {ex_mem.alu_res = 1;}
      break;

    case FUNCT_SLTU :
      if (id_ex.rs_value < second_op){ex_mem.alu_res = 1;}
      else {ex_mem.alu_res = 1;}
      break;

    case FUNCT_SRL :
      ex_mem.alu_res = second_op >> id_ex.shamt;
      break;
      
    case FUNCT_SUB :
      ex_mem.alu_res = id_ex.rs_value - second_op;
      break;
      
    case FUNCT_SUBU :
      ex_mem.alu_res = id_ex.rs_value - second_op;
      break;

    default :
      return (ERROR_UNKNOWN_FUNCT);
    }
  


  return 0;
}

// simulates the ex stage.
int interp_ex(){
  ex_mem.branch = id_ex.branch; 
  ex_mem.mem_to_reg = id_ex.mem_to_reg;
  ex_mem.mem_read = id_ex.mem_read;
  ex_mem.mem_write = id_ex.mem_write;
  ex_mem.reg_write = id_ex.reg_write;
  ex_mem.rt = id_ex.rt;
  ex_mem.rt_value = id_ex.rt_value;
  ex_mem.reg_dst = id_ex.reg_dst;
  ex_mem.branch_target = (id_ex.next_pc) +  (id_ex.sign_ext_imm << 2);
  //calling alu() to fill the last variable with data.
  alu_return = alu();
  if (alu_return != 0){
    return (alu_return);
  }
  else {
    return 0;
  }
}

// simulates the mem stage.
void interp_mem(){
  //printf("in interp mem \n");
  mem_wb.mem_to_reg = ex_mem.mem_to_reg;
  //printf("mem_to_reg set \n");
  mem_wb.reg_write = ex_mem.reg_write;
  //printf("reg_write set \n");
  mem_wb.rt = ex_mem.rt;
  //printf("no rt set \n");
  mem_wb.alu_res = ex_mem.alu_res;
  //printf("no alu_res set \n");
  mem_wb.reg_dst = ex_mem.reg_dst;
  //printf("no reg_dst set \n");
  
  if (ex_mem.mem_read == true){
    //printf("in if mem_read == true \n");
    mem_wb.read_data = GET_BIGWORD(mem, ex_mem.alu_res);
    //printf("done if mem_read == true \n");
  }
  
  if (ex_mem.mem_write == true){
    //printf("in if mem_write == true \n");
    SET_BIGWORD(mem, ex_mem.alu_res, ex_mem.rt_value);
    //printf("done if mem_write == true \n");
  }
}

// simulates the wb stage.
void interp_wb(){
  printf("in wb \n");
  if ((mem_wb.reg_write == false) | (mem_wb.reg_dst == 0)){
    printf("no writeback \n");
  }
  else {
    if (mem_wb.mem_to_reg == true){
      regs[mem_wb.reg_dst] = mem_wb.read_data; 
      printf(" reg_dst = read_data \n");
    }
    else{
      regs[mem_wb.reg_dst] = mem_wb.alu_res;
      printf(" reg_dst = alu_res \n");
    }
  }
}
// prints everything, debugging.
void write_reg(){
  printf("if_id:\n");
  printf("inst: %x\n\n",if_id.inst);

  printf("id_ex\n");
  printf("mem_read: %x\n", id_ex.mem_read);
  printf("mem_write: %x\n", id_ex.mem_write);
  printf("reg_write: %x\n", id_ex.reg_write);
  printf("rt: %x\n", id_ex.rt);
  printf("rs_value: %x\n",id_ex.rs_value);
  printf("rt_value: %x\n",id_ex.rt_value);
  printf("sign_ext_imm: %x\n",id_ex.sign_ext_imm);
  printf("funct: %x\n\n",id_ex.funct);

  printf("ex_mem:\n");
  printf("ex_mem.mem_read: %x\n",ex_mem.mem_read);
  printf("mem_write: %x\n",ex_mem.mem_write);
  printf("reg_write: %x\n",ex_mem.reg_write);
  printf("rt:%x\n",ex_mem.rt);
  printf("rt_value: %x\n",ex_mem.rt_value);
  printf("alu_res: %x\n",ex_mem.alu_res);

  printf("mem_wb: \n");
  printf("regwrite: %x\n",mem_wb.reg_write);
  printf("rt: %x\n",mem_wb.rt);
  printf("read_data: %x\n",mem_wb.read_data);
}

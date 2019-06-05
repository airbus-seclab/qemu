/*
 * (c) 2018, Airbus A. Gantet
 */
#include "qemu/pok.h"
#include <string.h>

#if defined(GENERATOR_RAND_ID_ARGS_VALID_USERMEM)
static size_t generate_rand_but_valid_memory_areas(
   uint8_t *in, size_t in_size,
   uint8_t* out, size_t out_max)
{
   size_t in_cur = 0;  // cursor in input memory
   size_t out_cur = 0; // cursot in output memory
   int f_id;          // parsed syscall id
   uint32_t expected;
   uint8_t reg = 0;    // register counter to store param addresses
   uint32_t syscall_args[5] = {0,0,0,0,0}; // push instructions with 1) immediate value or 2) registers storing param address
   bool_t syscall_args_types[5] = {0,0,0,0,0}; // boolean table: 0: no pointer; 1: pointer

   if (in_size == 0) {
      return 0;
   }

   while (in_cur < in_size && out_cur < out_max) {

      if (in[in_cur] >= 45) {
#ifdef POK_GEN_DEBUG
         printf("Warning: Invalid ID in input file; parsing stopped\n");
#endif
         return out_cur;
      }

      f_id = in[in_cur++];

#ifdef POK_GEN_DEBUG
      printf("f_id 0x%x\n", f_id);
#endif

      expected = compute_expected_size(f_id);

      if ((in_cur == in_size) ||
          (in_cur+expected > in_size + 1)) {
#ifdef POK_GEN_DEBUG
         printf("Warning: Invalid argument size in input file; parsing stopped\n");
#endif
         return out_cur;
      }

      // correct syscall but disabled in generator configuration
      // silently drop its generation
      if (!pok_syscall_table[f_id].enabled) {
#ifdef POK_GEN_DEBUG
         printf("Warning: Dropped syscall 0x%x\n", f_id);
#endif
         in_cur += expected;
         continue;
      }

      // Parse and build syscall arguments to be pushed
      for (int i=0; i<(pok_syscall_table[f_id]).pok_syscall_nbargs; i++) {
        if ((pok_syscall_table[f_id]).pok_param_sizes[i] == 0) {
            // Build "push 0xXXXXXXXX" instruction for classic syscall parameters
            memcpy(&(syscall_args[i]), &(in[in_cur]), 4);
            in_cur+= 4;

        } else {
            // Write "push instructions" to push structure content on the stack
            for (int j = 0; j < (pok_syscall_table[f_id]).pok_param_sizes[i]; j+=4) {
             out[out_cur++] = 0x68; /*push opcode*/
             memcpy(&(out[out_cur]), &(in[in_cur]), 4);
             out_cur+=4;
             in_cur+=4;
            }
            // Write "mov %esp, %e{a,c,d,b}x" instruction to store structure address into a register
            out[out_cur] = 0x89;
            out[out_cur+1] = 0xe0 | reg;
            out_cur += 2;
            // Build "push %e{a,c,d,b}x" instruction
            syscall_args[i] = (0x50 | reg);
            syscall_args_types[i] = 1;
            reg++;
        }
      }

      // Write "push" instructions to push final arguments
      for (int i=5; i>0; i--) {
         if (i > pok_syscall_table[f_id].pok_syscall_nbargs) {
            out[out_cur++] = 0x68; //push opcode
            memset(&(out[out_cur]), 0, 4);
            out_cur+=4;
         } else {
            if (syscall_args_types[i-1] == 0) {
              out[out_cur++] = 0x68; //push opcode
              memcpy(&(out[out_cur]), &(syscall_args[(i-1)]), 4);
              out_cur+=4;
            } else {
              out[out_cur++] = (uint8_t) syscall_args[i-1];
            }
         }
      }

      // Write "push" instruction to push nbargs
      out[out_cur++] = 0x68; //push opcode
      *((uint32_t*)(out+out_cur)) = pok_syscall_table[f_id].pok_syscall_nbargs;
      out_cur+=4;

      // Write "mov %esp, %ebx" instruction to store stack content in %ebx
      out[out_cur] = 0x89;
      out[out_cur+1] = 0xe3;
      out_cur += 2;

      //Write "mov (id), %eax" to store syscall in %eax
      out[out_cur++] = 0xb8; //mov opcode
      *((uint32_t*)(out+out_cur)) = pok_syscall_table[f_id].pok_syscall_id;
      out_cur += 4;

      //Write "int 42" instruction to perform syscall
      out[out_cur] = 0xcd; //int opcode
      out[out_cur+1] = 42;
      out_cur+=2;

      for (int i = 0; i < 5; i++) {
          syscall_args[i] = 0;
          syscall_args_types[i] = 0;
      }
      reg = 0;
   }

   return out_cur;
}
#endif

#ifdef GENERATOR_FIXED_RAND_ID_RAND_ARGS
size_t generate_rand_id_rand_args(
   uint8_t *in, size_t in_size,
   uint8_t* out, size_t out_max)
{
}
#endif

#ifdef GENERATOR_FIXED_ID_RAND_ARGS
size_t generate_fixed_id_rand_args(
   uint8_t *in, size_t in_size,
   uint8_t* out, size_t out_max)
{
   size_t out_cur = 0; // cursot in output memory

   if (in_size != 4) {
      // Bad input format (expected input: 4 bytes)
      return 0;
   }

   // Write "mov XXXXXXXX in ebx
   out[out_cur++] = 0xbb; //mov opcode
   memcpy(&(out[out_cur]), &(in[0]), 4);
   out_cur += 4;

   //Write "mov (id), %eax" to store syscall in %eax
   out[out_cur++] = 0xb8; //mov opcode
   *((uint32_t*)(out+out_cur)) = GENERATOR_FIXED_ID;
   out_cur += 4;

   //Write "int 42" instruction to perform syscall
   out[out_cur] = 0xcd; //int opcode
   out[out_cur+1] = 42;
   out_cur+=2;

   return out_cur;
}
#endif

size_t afl_gen_code(uint8_t *in, size_t in_size, uint8_t* out, size_t out_max)
{
   return
#if defined(GENERATOR_FIXED_RAND_ID_RAND_ARGS)
   generate_rand_id_rand_args(in, in_size, out, out_max);
#elif defined(GENERATOR_RAND_ID_ARGS_VALID_USERMEM)
   generate_rand_but_valid_memory_areas(in, in_size, out, out_max);
#elif defined(GENERATOR_FIXED_ID_RAND_ARGS)
   generate_fixed_id_rand_args(in, in_size, out, out_max);
#endif
}

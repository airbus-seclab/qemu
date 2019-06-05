/*
 * (c) 2019, Airbus A. Gantet
 */
#include "qemu/pok.h"
#include <string.h>

#ifdef GENERATOR_FIXED_RAND_ID_RAND_ARGS
static size_t generate_rand_id_rand_args(
   uint8_t *in, size_t in_size,
   uint8_t* out, size_t out_max)
{
   size_t in_cur = 0;
   size_t out_cur = 0;
   int f_id;

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

      if ((in_cur == in_size) ||
          (in_cur+pok_syscall_table[f_id].pok_syscall_nbargs*4 > in_size + 1)) {
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
         in_cur += 4*pok_syscall_table[f_id].pok_syscall_nbargs;
         continue;
      }

      for (int i=0; i<pok_syscall_table[f_id].pok_syscall_nbargs; i++) {
        // 3c X0 YY YY   lis rX, YYYY@high (pok args)
        out[out_cur] = 0x3c;
        out[out_cur+1] = (i+4) << 5;
        out_cur+=2;
        memcpy(&(out[out_cur]), &(in[in_cur+2*(i)]), 2);
        out_cur+=2;

        // 60 XX YY YY   ori rX, rX, YYYY@low (pok args)
        out[out_cur] = 0x60;
        out[out_cur+1] = (i+4) << 5 | (i+4);
        out_cur+=2;
        memcpy(&(out[out_cur]), &(in[in_cur+2+2*(i)]), 2);
        out_cur+=2;
      }

      // 38 81 XX XX li r3, 0xXXXX (= syscall id)
      *((uint32_t*)(out+out_cur)) =
         ((pok_syscall_table[f_id].pok_syscall_id&0xFF)<<8|
          (pok_syscall_table[f_id].pok_syscall_id)>>8) << 16 | 0x6038;
      out_cur+=4;

      // 44 00 00 40 sc
      *((uint32_t*)(out+out_cur)) = 0x02000044;
      out_cur+=4;

      in_cur += 4*pok_syscall_table[f_id].pok_syscall_nbargs;
  }

   return out_cur;
}
#endif

#if defined(GENERATOR_RAND_ID_ARGS_VALID_USERMEM)
static uint32_t compute_expected_stack_size(uint8_t syscall_id) {
    uint32_t size = 0;
    for (int i = 0; i < pok_syscall_table[syscall_id].pok_syscall_nbargs; i++) {
        size += (pok_syscall_table[syscall_id]).pok_param_sizes[i];
    }
    return size;
}

static size_t generate_rand_but_valid_memory_areas(
   uint8_t *in, size_t in_size,
   uint8_t* out, size_t out_max)
{
   size_t in_cur = 0;  // cursor in input memory
   size_t out_cur = 0; // cursot in output memory
   int f_id;          // parsed syscall id
   uint32_t expected;

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

      for (int i=0; i<(pok_syscall_table[f_id]).pok_syscall_nbargs; i++) {

        // The argument is not a "pointer"
        if ((pok_syscall_table[f_id]).pok_param_sizes[i] == 0) {
          // 3c X0 YY YY   lis rX, YYYY@high (pok args)
          out[out_cur] = 0x3c;
          out[out_cur+1] = (i+4) << 5;
          out_cur+=2;
          memcpy(&(out[out_cur]), &(in[in_cur+2*(i)]), 2);
          out_cur+=2;
          // 60 XX YY YY   ori rX, rX, YYYY@low (pok args)
          out[out_cur] = 0x60;
          out[out_cur+1] = (i+4) << 5 | (i+4);
          out_cur+=2;
          memcpy(&(out[out_cur]), &(in[in_cur+2+2*(i)]), 2);
          out_cur+=2;

          in_cur+=4;

        // The argument is a "pointer"
        } else {
          // 7c XX 0b 78   mr   rXX,r1 (stack pointer): 01111100 001XXXXX 00001011 01111000 0x7c 0x20|i+4 0x0b 0x78
          out[out_cur] = 0x7c;
          out[out_cur+1] = (1 << 5) | (i+4);
          out[out_cur+2] = 0x0b;
          out[out_cur+3] = 0x78;
          out_cur+=4;

          // Store the input values on the stack (pointer content)
          // 94 21 fe 60     stwu    r1,-pok_param_sizes(r1)
          out[out_cur] = (37<<2)| (1 >> 3);
          out[out_cur+1] = 1 << 5 | 1;
          out_cur+=2;
          out[out_cur] = (-(pok_syscall_table[f_id]).pok_param_sizes[i]) >> 8;
          out[out_cur+1] = (-(pok_syscall_table[f_id]).pok_param_sizes[i]);
          out_cur+=2;

          for (int j = 0; j < (pok_syscall_table[f_id]).pok_param_sizes[i]; j+=4) {
            // 3c X0 YY YY   lis r27, YYYY@high (pok args)
            out[out_cur] = (15<<2)| (27 >> 3);
            out[out_cur+1] = (uint8_t)(27 << 5) | 0;
            out_cur+=2;
            memcpy(&(out[out_cur]), &(in[in_cur+2*(j)]), 2);
            out_cur+=2;       
            // 60 XX YY YY   ori r27, r27, YYYY@low (pok args)
            out[out_cur] = 0x60 | (27 >> 3);
            out[out_cur+1] = (uint8_t)(27 << 5) | 27;
            out_cur+=2;
            memcpy(&(out[out_cur]), &(in[in_cur+2+2*(j)]), 2);
            out_cur+=2;
            in_cur+=4;
            // 93 61 01 8c     stw     r27,j+4(r1)
            out[out_cur] = (36<<2)| (27 >> 3);
            out[out_cur+1] = (uint8_t) (27 << 5) | 1;
            out_cur+=2;
            out[out_cur] = (j+4) >> 8;
            out[out_cur+1] = (j+4);
            out_cur+=2;       
          }
        }
     }
    // 38 81 XX XX   li   r3, 0xXXXX (= syscall id)
    *((uint32_t*)(out+out_cur)) = ((pok_syscall_table[f_id].pok_syscall_id&0xFF)<<8|(pok_syscall_table[f_id].pok_syscall_id)>>8) << 16 | 0x6038;
    out_cur+=4;

    // 44 00 00 40 sc
    *((uint32_t*)(out+out_cur)) = 0x02000044;
    out_cur+=4;

    if (compute_expected_stack_size(f_id) != 0) {
      // 38 21 01 a0     addi    r1,r1,XXXX
      out[out_cur]   = 0x38;
      out[out_cur+1] = 0x21;
      out_cur+=2;
      out[out_cur] = ((compute_expected_stack_size(f_id))) >> 8;
      out[out_cur+1] = (compute_expected_stack_size(f_id));
      out_cur+=2;
    }
  }
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

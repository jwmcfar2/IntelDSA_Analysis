#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <asm/prctl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <x86intrin.h>
#include <smmintrin.h> // Include for SSE4.1 (and earlier) intrinsics
#include <immintrin.h> // Include for AVX/AMX intrinsics

#define MAX 1024
#define MAX_ROWS 16
#define MAX_COLS 64
#define STRIDE 64
#define XFEATURE_XTILEDATA 18

//Define tile config data structure
typedef struct __tile_config
{
  uint8_t palette_id;
  uint8_t start_row;
  uint8_t reserved_0[14];
  uint16_t colsb[16];
  uint8_t rows[16];
} __tilecfg;
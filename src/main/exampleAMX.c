//==============================================================
// Copyright © 2022 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <immintrin.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX 1024
#define MAX_ROWS 8
#define MAX_COLS 64
#define STRIDE 64
#define ARCH_GET_XCOMP_PERM     0x1022
#define ARCH_REQ_XCOMP_PERM     0x1023
#define XFEATURE_XTILECFG       17
#define XFEATURE_XTILEDATA      18

//Define tile config data structure 
typedef struct __tile_config
{
  uint8_t palette_id;
  uint8_t start_row;
  uint8_t reserved_0[14];
  uint16_t colsb[16]; 
  uint8_t rows[16]; 
} __tilecfg;

/* Initialize tile config */
/*static void init_tile_config (__tilecfg *tileinfo)
{
  int i;
  tileinfo->palette_id = 1;
  tileinfo->start_row = 0;
  printf("Test-a1\n");

  for (i = 0; i < 1; i++)
  {
    printf("\t>Test-a2\n");
    tileinfo->colsb[i] = MAX_ROWS;
    tileinfo->rows[i] =  MAX_ROWS;
  }

  for (i = 1; i < 4; i++)
  {
    printf("\t>>Test-a3\n");
    tileinfo->colsb[i] = MAX_COLS;
    tileinfo->rows[i] =  MAX_ROWS;
  }

  printf("Test-a4\n");
  _tile_loadconfig (tileinfo);
  printf("Test-a4\n");
}*/

/* Initialize int8_t buffer */
/*static void init_buffer (int8_t *buf, int8_t value)
{
  int rows, colsb, i, j;
  rows  = MAX_ROWS;
  colsb = MAX_COLS;

  for (i = 0; i < rows; i++)
  {
    for (j = 0; j < colsb; j++)
    {
        buf[i * colsb + j] = value;
    }
  }
}*/

/* Initialize int32_t buffer */
/*static void init_buffer32 (int32_t *buf, int32_t value)
{
  int rows, colsb, i, j;
  rows  = MAX_ROWS;
  colsb = MAX_COLS;
  int colsb2=colsb/4;

  for (i = 0; i < rows; i++)
    for (j = 0; j < (colsb2); j++)
    {
        buf[i * colsb2 + j] = value;
    }
}*/

/* Set_tiledata_use() - Invoke syscall to set ARCH_SET_STATE_USE */
static bool set_tiledata_use()
{
   if (syscall(SYS_arch_prctl, ARCH_REQ_XCOMP_PERM, XFEATURE_XTILEDATA)) 
   {
      printf("\n Fail to do XFEATURE_XTILEDATA \n\n");
      return false;
   }
   else
   {
      printf("\n TILE DATA USE SET - OK \n\n");
      return true;
   }

   return true;
}

/* Print int8_t buffer */
/*static void print_buffer(int8_t* buf, int32_t rows, int32_t colsb) 
{
   for (int i = 0; i < rows; i++) {
     for (int j = 0; j < (colsb); j++)
     {
         printf("%d ", buf[i * colsb + j]);
     }
     printf("\n");
   }
   printf("\n");
}*/

/* Print int32_t buffer */
/*static void print_buffer32(int32_t* buf, int32_t rows, int32_t colsb)
{
   for (int i = 0; i < rows; i++) {
     for (int j = 0; j < (colsb); j++)
     {
         printf("%d ", buf[i * colsb + j]);
     }
     printf("\n");
   }
   printf("\n");
}*/

int main(){

   __tilecfg tile_data = {0};
   int8_t src1[MAX];
   int8_t src2[MAX];
   //int32_t res[MAX/4];
   //int rows  = MAX_ROWS;
   //int colsb = MAX_COLS;

   // Request permission to linux kernel to run AMX 
   if (!set_tiledata_use())
      exit(-1);

   printf("Test-A\n");
   // Load tile configuration 
   //init_tile_config (&tile_data);

    tile_data.palette_id = 1;
    tile_data.start_row = 0;
    printf("Test-a1\n");

    for (int i = 0; i < 3; i++)
    {
      printf("\t>Test-a2\n");
      tile_data.colsb[i] = MAX_ROWS;
      tile_data.rows[i] =  MAX_ROWS;
    }

    /*for (int i = 2; i < 2; i++)
    {
      printf("\t>>Test-a3\n");
      tile_data.colsb[i] = MAX_COLS;
      tile_data.rows[i] =  MAX_ROWS;
    }*/

    printf("Test-a4\n");
    _tile_loadconfig (&tile_data);
    printf("Test-a4\n");

   printf("Test-B\n");
   // Init src matrix buffers with data
   //init_buffer (src1, 2);


  //int rows, colsb, i, j;
  //rows  = MAX_ROWS;
  //colsb = MAX_COLS;

  for (int i = 0; i < MAX_ROWS; i++)
  {
    for (int j = 0; j < MAX_COLS; j++)
    {
        //printf("TEST-Ba - [%d][%d] = 2\n", i, j);
        src1[i * MAX_COLS + j] = 2;
    }
  } 

   printf("Test-C\n");
   //print_buffer(src1, rows, colsb);
 
   //init_buffer (src2, 0);
   printf("Test-D\n");

  for (int i = 0; i < MAX_ROWS; i++)
  {
    for (int j = 0; j < MAX_COLS; j++)
    {
        src2[i * MAX_COLS + j] = 0;
    }
  } 
   //print_buffer(src2, rows, colsb);

   // Init dst matrix buffers with data
   //init_buffer32 (res, 0);
   printf("Test-E -- DEBUG: size(src1)=%u, src1=%u\n", sizeof(src1), MAX*sizeof(int8_t));

   // Load tile rows from memory
   _tile_loadd (2, src1, STRIDE);
   printf("Test-F\n");



   // JMAC TEST: //
   ////////////////
   _tile_stored (2, src2, STRIDE);
   printf("Test-G\n");
   printf("\n~~~~ SUCCESS! ~~~~~\n\n");


  printf("\nAFTER...\n");
  printf("\nTest3Src[0]:\n\t");
  
  for(int j=0; j<1024; j++)
  {
      printf("%u ", src1[j]);
      if(j!=0 && j%32==31)
          printf("\n\t");
  }

  printf("\nTest3Dst[0]:\n\t");
  for(int j=0; j<1024; j++)
  {
      printf("%u ", src2[j]);
      if(j!=0 && j%32==31)
          printf("\n\t");
  }

   return 0;
   ////////////////
   //~ Aww yuss ~//


/*
   _tile_loadd (3, src2, STRIDE);
   printf("Test-G2\n");
   //_tile_loadd (1, res, STRIDE);
   printf("Test-G3\n");

   // Compute dot-product of bytes in tiles 
   _tile_dpbssd (1, 2, 3);
   printf("Test-H\n");

   // Store the tile data to memory
   //_tile_stored (1, res, STRIDE);
   printf("Test-I\n");
   //print_buffer32(res, rows, colsb/4);
   printf("Test-J\n");

   // Release the tile configuration to return to the init state, 
   // which releases all storage it currently holds
   _tile_release ();

   printf("\n\t~~~~ SUCCESS! ~~~~~\n\n");*/
}
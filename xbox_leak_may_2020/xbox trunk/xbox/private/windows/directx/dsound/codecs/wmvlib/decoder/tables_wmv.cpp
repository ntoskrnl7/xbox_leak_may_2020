#include "bldsetup.h"

#include "xplatform.h"
#include "wmvdec_api.h"
#include "typedef.hpp"
#include "tables_wmv.h"

//Bool_WMV g_bSupportMMX;

I8_WMV gmvUVtableNew [128] = {
-31,-31,-31,-30,-29,-29,-29,-28,-27,-27,-27,-26,-25,-25,-25,-24,
-23,-23,-23,-22,-21,-21,-21,-20,-19,-19,-19,-18,-17,-17,-17,-16,
-15,-15,-15,-14,-13,-13,-13,-12,-11,-11,-11,-10, -9, -9, -9, -8,
 -7, -7, -7, -6, -5, -5, -5, -4, -3, -3, -3, -2, -1, -1, -1,  0,
  1,  1,  1,  2,  3,  3,  3,  4,  5,  5,  5,  6,  7,  7,  7,  8,
  9,  9,  9, 10, 11, 11, 11, 12, 13, 13, 13, 14, 15, 15, 15, 16,
 17, 17, 17, 18, 19, 19, 19, 20, 21, 21, 21, 22, 23, 23, 23, 24,
 25, 25, 25, 26, 27, 27, 27, 28, 29, 29, 29, 30, 31, 31, 31,0
};

I8_WMV* gmvUVtable4 = gmvUVtableNew + 63;

//New designed ZIGZAG matrix. 
//grgiZigzagInv is matrix for P,
//grgiZigzagInv_I and _Horizontal and _Vertical are matrixes for I,
// the following 3 are for decoder only

U8_WMV grgiHorizontalZigzagInvRotated_NEW [BLOCK_SQUARE_SIZE] = {
    0,  8,  1, 16, 24,  9,  2,  3,
   10, 17, 32, 40, 25, 18, 11,  4,
    5,  6, 12, 19, 26, 33, 48, 56,
   41, 34, 27, 20, 13,  7, 14, 15,
   21, 28, 35, 42, 49, 57, 50, 43,
   36, 29, 22, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
};
U8_WMV grgiVerticalZigzagInvRotated_NEW [BLOCK_SQUARE_SIZE] = {
     0,  1,  2,  8,  3,  4,  5,  9,
    16, 24, 17, 10, 11,  6,  7, 13,
    12, 19, 18, 25, 32, 40, 33, 26,
    27, 20, 14, 15, 22, 21, 28, 35,
    34, 41, 48, 56, 49, 42, 43, 36,
    29, 30, 23, 31, 38, 37, 44, 51,
    50, 57, 58, 59, 52, 45, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
};

U8_WMV grgiZigzagInvRotated_NEW_I [BLOCK_SQUARE_SIZE+1] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12,  6, 19, 26, 33, 40, 48, 41,
 34, 27, 20, 13,  7, 14, 15, 21,
 28, 35, 42, 49, 56, 57, 50, 43,
 36, 29, 22, 23, 30, 31, 37, 44,
 51, 58, 59, 52, 45, 38, 39, 46,
 47, 53, 60, 61, 54, 55, 62, 63,0
};
U8_WMV grgiZigzagInvRotated_NEW [BLOCK_SQUARE_SIZE] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
  6,  7, 13, 12, 19, 26, 33, 40,
 48, 41, 34, 27, 20, 14, 15, 23,
 22, 21, 28, 35, 42, 49, 56, 57,
 50, 43, 36, 29, 30, 31, 39, 38,
 37, 44, 51, 58, 59, 52, 45, 46,
 47, 55, 54, 53, 60, 61, 62, 63,
};
U8_WMV grgiZigzagInv_NEW [BLOCK_SQUARE_SIZE+1] = {
    0,    8,    1,    2,    9,   16,   24,   17,
   10,    3,    4,   11,   18,   25,   32,   40,
   48,   56,   41,   33,   26,   19,   12,    5,
    6,   13,   20,   27,   34,   49,   57,   58,
   50,   42,   35,   28,   21,   14,    7,   15,
   22,   29,   36,   43,   51,   59,   60,   52,
   44,   37,   30,   23,   31,   38,   45,   53,
   61,   62,   54,   46,   39,   47,   55,   63,0
};

U8_WMV grgiZigzagInv_NEW_I [BLOCK_SQUARE_SIZE+1] = {
    0,    8,    1,    2,    9,   16,   24,   17,
   10,    3,    4,   11,   18,   25,   32,   40,
   33,   48,   26,   19,   12,    5,    6,   13,
   20,   27,   34,   41,   56,   49,   57,   42,
   35,   28,   21,   14,    7,   15,   22,   29,
   36,   43,   50,   58,   51,   59,   44,   37,
   30,   23,   31,   38,   45,   52,   60,   53,
   61,   46,   39,   47,   54,   62,   55,   63, 0
};

U8_WMV grgiHorizontalZigzagInv_NEW [BLOCK_SQUARE_SIZE] = {
    0,    1,    8,    2,    3,    9,   16,   24,
   17,   10,    4,    5,   11,   18,   25,   32,
   40,   48,   33,   26,   19,   12,    6,    7,
   13,   20,   27,   34,   41,   56,   49,   57,
   42,   35,   28,   21,   14,   15,   22,   29,
   36,   43,   50,   58,   51,   44,   37,   30,
   23,   31,   38,   45,   52,   59,   60,   53,
   46,   39,   47,   54,   61,   62,   55,   63,
};
U8_WMV grgiVerticalZigzagInv_NEW [BLOCK_SQUARE_SIZE] = {
    0,    8,   16,    1,   24,   32,   40,    9,
    2,    3,   10,   17,   25,   48,   56,   41,
   33,   26,   18,   11,    4,    5,   12,   19,
   27,   34,   49,   57,   50,   42,   35,   28,
   20,   13,    6,    7,   14,   21,   29,   36,
   43,   51,   58,   59,   52,   44,   37,   30,
   22,   15,   23,   31,   38,   45,   60,   53,
   46,   39,   47,   54,   61,   62,   55,   63,
};
U8_WMV grgi8x4ZigzagInv[33] = { 
    0,    1,    2,    8,    3,    9,   10,   16, 
    4,   11,   17,   24,   18,   12,    5,   19,
   25,   13,   20,   26,   27,    6,   21,   28, 
   14,   22,   29,    7,   30,   15,   23,   31, 0
};
U8_WMV grgi4x8ZigzagInv[33] = {   
    0,    4,    1,    8,    5,   12,    9,    2, 
   16,    6,   13,   20,   10,   24,   17,   14, 
   28,   21,   18,    3,   25,   29,    7,   22, 
   11,   26,   15,   30,   19,   23,   27,   31, 0
};
U8_WMV grgi8x4ZigzagInvRotated[33] = { 
    0,    4,    8,    1,   12,    5,    9,    2, 
   16,   13,    6,    3,   10,   17,   20,   14,
    7,   21,   18,   11,   15,   24,   22,   19, 
   25,   26,   23,   28,   27,   29,   30,   31, 0
};
U8_WMV grgi4x8ZigzagInvRotated[33] = {   
    0,    1,    8,    2,    9,    3,   10,   16, 
    4,   17,   11,    5,   18,    6,   12,   19, 
    7,   13,   20,   24,   14,   15,   25,   21, 
   26,   22,   27,   23,   28,   29,   30,   31, 0
};

//MPEG4 Zigzag matrix
U8_WMV grgiZigzagInvRotated [BLOCK_SQUARE_SIZE] = {
	0,	8,  1,	2,	9,	16,	24,	17,	
	10,	3,	4,	11,	18,	25,	32,	40,	
	33,	26,	19,	12,	5,	6,  13, 20,
    27, 34, 41, 48, 56, 49, 42, 35,
    28, 21, 14, 7,	15, 22,	29,	36,
    43,	50,	57,	58,	51,	44,	37,	30,
    23,	31,	38,	45,	52,	59,	60,	53,
    46,	39,	47,	54,	61,	62,	55,	63,
};


U8_WMV grgiZigzagInv [BLOCK_SQUARE_SIZE+1] = {
	0,	1,	8,	16,	9,	2,	3,	10,	
	17,	24,	32,	25,	18,	11,	4,	5,	
	12,	19,	26,	33,	40,	48,	41,	34,	
	27,	20,	13,	6,	7,	14,	21,	28,	
	35,	42,	49,	56,	57,	50,	43,	36,	
	29,	22,	15,	23,	30,	37,	44,	51,	
	58,	59,	52,	45,	38,	31,	39,	46,	
	53,	60,	61,	54,	47,	55,	62,	63, 0,
};

U8_WMV grgiHorizontalZigzagInv [BLOCK_SQUARE_SIZE] = {
	0, 1, 2, 3, 8, 9, 16, 17, 
	10, 11, 4, 5, 6, 7, 15, 14, 
	13, 12, 19, 18, 24, 25, 32, 33, 
	26, 27, 20, 21, 22, 23, 28, 29, 
	30, 31, 34, 35, 40, 41, 48, 49, 
	42, 43, 36, 37, 38, 39, 44, 45, 
	46, 47, 50, 51, 56, 57, 58, 59, 
	52, 53, 54, 55, 60, 61, 62, 63
};
U8_WMV grgiVerticalZigzagInv [BLOCK_SQUARE_SIZE] = {
	0, 8, 16, 24, 1, 9, 2, 10, 
	17, 25, 32, 40, 48, 56, 57, 49, 
	41, 33, 26, 18, 3, 11, 4, 12, 
	19, 27, 34, 42, 50, 58, 35, 43, 
	51, 59, 20, 28, 5, 13, 6, 14, 
	21, 29, 36, 44, 52, 60, 37, 45, 
	53, 61, 22, 30, 7, 15, 23, 31, 
	38, 46, 54, 62, 39, 47, 55, 63
};


// sys_c

/// Tables for HighMotion
//#define numSampleNotLastIntraY 31
//#define numSampleLastIntraY 38
U8_WMV sm_rgIfNotLastNumOfLevelAtRunIntraY_HghMt [31] = { 
   19,   15,   12,   11,    6,    5,    4,    4,    4,    4,    3,
    3,    3,    3,    3,    3,    2,    2,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,
};
U8_WMV sm_rgIfLastNumOfLevelAtRunIntraY_HghMt [38] = { 
    6,    5,    4,    4,    3,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,
   };
U8_WMV sm_rgIfNotLastNumOfRunAtLevelIntraY_HghMt [20] = {
  -1,  30,  17,  15,   9,   5,   4,   3,   3,   3,   3,
   3,   2,   1,   1,   1,   0,   0,   0,   0,
};
U8_WMV sm_rgIfLastNumOfRunAtLevelIntraY_HghMt [7] = {
  -1,  37,  15,   4,   3,   1,   0,
};

//#define numSampleNotLast 27
//#define numSampleLast 37
U8_WMV sm_rgIfNotLastNumOfLevelAtRun_HghMt [27] = {
  23,   11,    8,    7,    5,    5,    4,    4,    3,    3,    3,
    3,    2,    2,    2,    2,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,
};
U8_WMV sm_rgIfLastNumOfLevelAtRun_HghMt [37] = { 
    9,    5,    4,    4,    3,    3,    3,    2,    2,    2,    2,
    2,    2,    2,    2,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,
};
U8_WMV sm_rgIfNotLastNumOfRunAtLevel_HghMt [24] = {
  -1,  26,  16,  11,   7,   5,   3,   3,   2,   1,   1,
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,
};
U8_WMV sm_rgIfLastNumOfRunAtLevel_HghMt [10] = {
  -1,  36,  14,   6,   3,   1,   0,   0,   0,   0,
};



/// Tables for __TALKING_DCTAC
//#define numSampleNotLastIntraY 21
//#define numSampleLastIntraY 27
U8_WMV sm_rgIfNotLastNumOfLevelAtRunIntraY_Talking [21] = { 
   16,   11,    8,    7,    5,    4,    4,    3,    3,    3,    3,
    3,    3,    3,    2,    2,    1,    1,    1,    1,    1,
};
U8_WMV sm_rgIfLastNumOfLevelAtRunIntraY_Talking [27] = { 
    4,    4,    3,    3,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,
};
U8_WMV sm_rgIfNotLastNumOfRunAtLevelIntraY_Talking [17] = {
	-1, 20, 15, 13, 6, 4, 3, 3, 2, 1, 1,
	 1,  0, 0, 0, 0, 0,
};

U8_WMV sm_rgIfLastNumOfRunAtLevelIntraY_Talking [5] = {
	 -1, 26, 13, 3, 1,
};

//int numSampleNotLast = 30;
//int numSampleLast = 44;
U8_WMV sm_rgIfNotLastNumOfLevelAtRun_Talking [30] = {
  14,    9,    5,    4,    4,    4,    3,    3,    3,    3,    3,
   3,    3,    2,    2,    2,    1,    1,    1,    1,    1,    1,
   1,    1,    1,    1,    1,    1,    1,    1,
};

U8_WMV sm_rgIfLastNumOfLevelAtRun_Talking [44] = {
   5,    4,    3,    3,    2,    2,    2,    2,    2,    2,    2,
   2,    2,    2,    2,    2,    1,    1,    1,    1,    1,    1,
   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
};
U8_WMV sm_rgIfNotLastNumOfRunAtLevel_Talking [15] = {
	-1, 29, 15, 12, 5, 2, 1, 1, 1, 1, 0,
	 0,  0,  0, 0, 
};

U8_WMV sm_rgIfLastNumOfRunAtLevel_Talking [6] = {
	 -1, 43, 15, 3, 1, 0,
};


/// Tables for __MPEG4
//#define numSampleNotLastIntraY 15
//#define numSampleLastIntraY 21

U8_WMV sm_rgIfNotLastNumOfLevelAtRunIntraY_MPEG4 [15] = {
	27, 10, 5, 4, 3, 3, 3, 3, 2, 2, 1,
	 1, 1, 1, 1
};

U8_WMV sm_rgIfLastNumOfLevelAtRunIntraY_MPEG4 [21] = {
	 8, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

U8_WMV sm_rgIfNotLastNumOfRunAtLevelIntraY_MPEG4 [28] = {
	-1, 14, 9, 7, 3, 2, 1, 1, 1, 1,
	 1,  0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0,  0, 0, 0, 0, 0, 0, 0
};

U8_WMV sm_rgIfLastNumOfRunAtLevelIntraY_MPEG4 [9] = {
	 -1, 20, 6, 1, 0, 0, 0, 0, 0
};

//int numSampleNotLast = 27;
//int numSampleLast = 41;
U8_WMV sm_rgIfNotLastNumOfLevelAtRun_MPEG4 [27] = {
	12, 6, 4, 3, 3, 3, 3, 2, 2, 2, 2,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1
};

U8_WMV sm_rgIfLastNumOfLevelAtRun_MPEG4 [41] = {
	 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1
};
// 0th element is NA. Level = 1..12
U8_WMV sm_rgIfNotLastNumOfRunAtLevel_MPEG4 [13] = {
	-1, 26, 10, 6, 2, 1, 1,
	 0,  0,  0, 0, 0, 0  
};
// 0th element is NA. Level = 1..3
U8_WMV sm_rgIfLastNumOfRunAtLevel_MPEG4 [4] = { 
	 -1, 40, 1, 0
};

const U8_WMV g_rgiClapTabDecWMV [1024] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

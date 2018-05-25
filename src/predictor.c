//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

#define DEBUG 1

//
// Student Information
//
const char *studentName = "Avinash Kondareddy ";
const char *studentID   = "A13074121";
const char *email       = "akondare@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

int ghr;
int * ght;

int * lhr;
int * pht;
int * cht;

unsigned int gMask;
unsigned int lMask;
unsigned int pcMask;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

void init_gshare() {
  int gNum = 1<<ghistoryBits;

  // initialize global history to zero
  ghr = 0;

  // allocate memory for ght, 
  ght = (int *)calloc(gNum, sizeof(int));

  // initialize predictions to weakly not taken (01 - two bit)
  for( int i = 0; i < gNum; i++ ) {
    ght[i] = WN;
  }

  gMask = ~( (-1) << ghistoryBits );
}

void init_tournament() {
  int gNum = 1<<ghistoryBits;
  int lNum = 1<<lhistoryBits;
  int pcNum = 1<<pcIndexBits;

  ghr = 0;

  lhr = (int *)calloc(pcNum, sizeof(int));
  pht = (int *)calloc(lNum, sizeof(int));
  ght = (int *)calloc(gNum, sizeof(int));
  cht = (int *)calloc(gNum, sizeof(int));

  // initialize predictions to weakly not taken (01 - two bit)
  for( int i = 0; i < gNum; i++ ) {
    ght[i] = WN;
    cht[i] = WT;
  }

  for( int i = 0; i < lNum; i++ ) {
    pht[i] = WN;
  }

  gMask = ~( (-1) << ghistoryBits );
  pcMask = ~( (-1) << pcIndexBits );
  lMask = ~( (-1) << lhistoryBits );
}

void init_custom() {
  ghistoryBits = 15;
  lhistoryBits = 15;
  pcIndexBits = 15;
  int gNum = 1<<ghistoryBits;
  int lNum = 1<<lhistoryBits;
  int pcNum = 1<<pcIndexBits;

  ghr = 0;

  lhr = (int *)calloc(pcNum, sizeof(int));
  pht = (int *)calloc(lNum, sizeof(int));
  ght = (int *)calloc(gNum, sizeof(int));
  cht = (int *)calloc(gNum, sizeof(int));

  // initialize predictions to weakly not taken (01 - two bit)
  for( int i = 0; i < gNum; i++ ) {
    ght[i] = WN;
    cht[i] = WT;
  }

  for( int i = 0; i < lNum; i++ ) {
    pht[i] = WN;
  }

  gMask = ~( (-1) << ghistoryBits );
  pcMask = ~( (-1) << pcIndexBits );
  lMask = ~( (-1) << lhistoryBits );
}

// Initialize the predictor
//
void init_predictor()
{
  switch (bpType) {
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      init_tournament();
      break;
    case CUSTOM:
      init_custom();
      break;
    default:
      break;
  }
}

void free_predictor()
{
  free(ght);
  free(lhr);
  free(pht);
  free(cht);
}

uint8_t local_pred(uint32_t pc) {
  return pht[ lhr[ pc & pcMask ] & lMask ] >> 1;
}
uint8_t global_pred(uint32_t pc) {
  return (( ght[(ghr) & gMask] ) >> 1);
  //return (( ght[ghr&gMask] ) >> 1);
}
uint8_t gshare_pred(uint32_t pc) {
  return (( ght[(ghr^pc) & gMask] ) >> 1);
}

uint8_t custom_pred(uint32_t pc) {
  return NOTTAKEN;
}
// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc)
{
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_pred(pc);
    case TOURNAMENT:
      return  (cht[(ghr)&gMask]>>1 == 0) ? local_pred(pc):global_pred(pc);
    case CUSTOM:
      return  (cht[(ghr^pc)&gMask]>>1 == 0) ? local_pred(pc):gshare_pred(pc);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_gshare(uint32_t pc, uint8_t outcome) {

  int * address = &ght[ (ghr^pc) & gMask ];
  int state = *address;

  switch (outcome) {
    case TAKEN:
      if ( state < ST ) {
        *address = state + 1;
      }
      break;
    case NOTTAKEN:
      if ( state > SN ) {
        *address = state - 1;
      }
      break;
    default: 
      break;
  }

  ghr = ( (ghr << 1) + outcome ) & gMask;
}

void train_local(uint32_t pc, uint8_t outcome) {

  // update bht
  int choice = pht[ lhr[ pc & pcMask ] & lMask ]; 
  if( outcome == NOTTAKEN && choice > SN ) { 
      pht[ lhr[ pc & pcMask ] & lMask ] -= 1;
  }
  if( outcome == TAKEN && choice < ST ) { 
      pht[ lhr[ pc & pcMask ] & lMask ] += 1;
  }

  // update pht
  lhr[pc & pcMask] = ( (lhr[pc & pcMask] << 1) + outcome ) & lMask;
}

void train_global(uint32_t pc, uint8_t outcome) {

  int * address = &ght[ (ghr) & gMask ];
  int state = *address;

  switch (outcome) {
    case TAKEN:
      if ( state < ST ) {
        *address = state + 1;
      }
      break;
    case NOTTAKEN:
      if ( state > SN ) {
        *address = state - 1;
      }
      break;
    default: 
      break;
  }

  ghr = ( (ghr << 1) + outcome ) & gMask;
}

void train_tournament(uint32_t pc, uint8_t outcome) {
  int pred_local, pred_global, choice;

  // get predictions and choice
  pred_local = local_pred(pc);
  pred_global = global_pred(pc);
  choice = cht[(ghr)&gMask];

  // update choice based on predictions
  if( pred_local == outcome && pred_global != outcome && choice > SN) {
    cht[(ghr)&gMask] -= 1;
  }
  if( pred_global == outcome && pred_local != outcome && choice < ST) {
    cht[(ghr)&gMask] += 1;
  }

  // train local and global predictors
  train_local(pc,outcome);
  train_global(pc,outcome);
}

void train_custom(uint32_t pc, uint8_t outcome) {
  int pred_local, pred_global, choice;

  // get predictions and choice
  pred_local = local_pred(pc);
  pred_global = gshare_pred(pc);
  choice = cht[(ghr^pc)&gMask];

  // update choice based on predictions
  if( pred_local == outcome && pred_global != outcome && choice > SN) {
    cht[(ghr^pc)&gMask] -= 1;
  }
  if( pred_global == outcome && pred_local != outcome && choice < ST) {
    cht[(ghr^pc)&gMask] += 1;
  }

  // train local and global predictors
  train_local(pc,outcome);
  train_gshare(pc,outcome);
}

void train_predictor(uint32_t pc, uint8_t outcome) {

  switch (bpType) {
    case STATIC:
      break;
    case GSHARE:
      train_gshare(pc,outcome);
      break;
    case TOURNAMENT:
      train_tournament(pc,outcome);
      break;
    case CUSTOM:
      train_custom(pc,outcome);
      break;
    default:
      break;
  }
}

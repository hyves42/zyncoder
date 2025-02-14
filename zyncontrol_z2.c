/*
 * ******************************************************************
 * ZYNTHIAN PROJECT: Zyncontrol Library for Zynthian Kits V1-V4
 * 
 * Initialize & configure control hardware for Zynthian Kits V1-V4
 * 
 * Copyright (C) 2015-2021 Fernando Moyano <jofemodo@zynthian.org>
 *
 * ******************************************************************
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE.txt file.
 * 
 * ******************************************************************
 */

#include <stdio.h>
#include <stdlib.h>

#include "gpiod_callback.h"
#include "zynpot.h"
#include "zyncoder.h"
#include "zynads1115.h"
#include "zynrv112.h"
#include "lm4811.h"

//-----------------------------------------------------------------------------
// GPIO Expander 1
//-----------------------------------------------------------------------------

#define MCP23017_1_BASE_PIN 100
#define MCP23017_1_I2C_ADDRESS 0x20
#define MCP23017_1_INTA_PIN 5
#define MCP23017_1_INTB_PIN 6

void zynmcp23017_ISR_bankA_1() {
	zynmcp23017_ISR(0, 0);
}
void zynmcp23017_ISR_bankB_1() {
	zynmcp23017_ISR(0, 1);
}
void (*zynmcp23017_ISRs_1[2]) = {
	zynmcp23017_ISR_bankA_1,
	zynmcp23017_ISR_bankB_1
};

//-----------------------------------------------------------------------------
// GPIO Expander 2
//-----------------------------------------------------------------------------

#define MCP23017_2_BASE_PIN 200
#define MCP23017_2_I2C_ADDRESS 0x21

#if Z2_VERSION==1
	#define MCP23017_2_INTA_PIN 7
	#define MCP23017_2_INTB_PIN 8
#else
	#define MCP23017_2_INTA_PIN 17
	#define MCP23017_2_INTB_PIN 27
#endif

void zynmcp23017_ISR_bankA_2() {
	zynmcp23017_ISR(1, 0);
}
void zynmcp23017_ISR_bankB_2() {
	zynmcp23017_ISR(1, 1);
}
void (*zynmcp23017_ISRs_2[2]) = {
	zynmcp23017_ISR_bankA_2,
	zynmcp23017_ISR_bankB_2
};

//-----------------------------------------------------------------------------
// 2 x zynmcp23017
//-----------------------------------------------------------------------------

void init_zynmcp23017s() {
	reset_zynmcp23017s();
	setup_zynmcp23017(0, MCP23017_1_BASE_PIN, MCP23017_1_I2C_ADDRESS, MCP23017_1_INTA_PIN, MCP23017_1_INTB_PIN, zynmcp23017_ISRs_1);
	setup_zynmcp23017(1, MCP23017_2_BASE_PIN, MCP23017_2_I2C_ADDRESS, MCP23017_2_INTA_PIN, MCP23017_2_INTB_PIN, zynmcp23017_ISRs_2);
}

//-----------------------------------------------------------------------------
// 30 x ZynSwitches (16 on MCP23017_1, 14 on MCP23017_2)
//-----------------------------------------------------------------------------

void init_zynswitches() {
	reset_zynswitches();
	int i;
	fprintf(stderr, "ZynCore: Setting-up 30 x Zynswitches...\n");
	for (i = 0; i < 16; i++) setup_zynswitch(4 + i, MCP23017_1_BASE_PIN + i, 1);
	for (i = 0; i < 14; i++) setup_zynswitch(20 + i, MCP23017_2_BASE_PIN + i, 1);
}

//-----------------------------------------------------------------------------
// 4 x Zynpòts (Analog Encoder RV112)
//-----------------------------------------------------------------------------

#define RV112_ADS1115_I2C_ADDRESS_1 0x48
#define RV112_ADS1115_I2C_ADDRESS_2 0x49

#define RV112_ADS1115_GAIN ADS1115_GAIN_VREF_4_096
//#define RV112_ADS1115_RATE ADS1115_RATE_475SPS
#define RV112_ADS1115_RATE ADS1115_RATE_860SPS

ads1115_t ads1115_nodes[MAX_NUM_ADS1115];

void init_zynpots() {
	reset_zyncoders();
	reset_zynpots();
	init_rv112s();
	init_ads1115(&(ads1115_nodes[0]), RV112_ADS1115_I2C_ADDRESS_1, RV112_ADS1115_GAIN, RV112_ADS1115_RATE);
	init_ads1115(&(ads1115_nodes[1]), RV112_ADS1115_I2C_ADDRESS_2, RV112_ADS1115_GAIN, RV112_ADS1115_RATE);

#if Z2_VERSION>2
	fprintf(stderr, "ZynCore: Setting-up Zynpots => 3 x RV112, 1 x PEC11 ...\n");
	setup_rv112(0, &(ads1115_nodes[0]), 0);
	setup_rv112(1, &(ads1115_nodes[0]), 0);
	setup_rv112(2, &(ads1115_nodes[1]), 0);
	init_poll_rv112();
	setup_zyncoder(0, MCP23017_2_BASE_PIN + 14, MCP23017_2_BASE_PIN + 15);
	setup_zynpot(0, ZYNPOT_RV112, 0);
	setup_zynpot(1, ZYNPOT_RV112, 1);
	setup_zynpot(2, ZYNPOT_RV112, 2);
	setup_zynpot(3, ZYNPOT_ZYNCODER, 0);
#else
	fprintf(stderr, "ZynCore: Setting-up Zynpots => 4 x RV112...\n");
	setup_rv112(0, &(ads1115_nodes[0]), 0);
	setup_rv112(1, &(ads1115_nodes[0]), 0);
	setup_rv112(2, &(ads1115_nodes[1]), 0);
	setup_rv112(3, &(ads1115_nodes[1]), 1);
	init_poll_rv112();
	int i;
	for (i = 0; i < 4; i++) {
		setup_zynpot(i, ZYNPOT_RV112, i);
	}
#endif
}

void end_zynpots() {
	end_rv112s();
	reset_zynpots();
}

//-----------------------------------------------------------------------------
// Zyncontrol Initialization
//-----------------------------------------------------------------------------

uint8_t set_hpvol(uint8_t vol) { return lm4811_set_volume(vol); }
uint8_t get_hpvol() { return lm4811_get_volume(); }
uint8_t get_hpvol_max() { return lm4811_get_volume_max(); }

int init_zyncontrol() {
	gpiod_init_callbacks();
	lm4811_init();
	init_zynmcp23017s();
	init_zynswitches();
	init_zynpots();
	gpiod_start_callbacks();
	return 1;
}

int end_zyncontrol() {
	gpiod_stop_callbacks();
	end_zynpots();
	reset_zyncoders();
	reset_zynswitches();
	reset_zynmcp23017s();
	lm4811_end();
	return 1;
}

//-----------------------------------------------------------------------------

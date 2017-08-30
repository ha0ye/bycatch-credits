/*
 *  vessel.cpp
 *  processor
 *
 *  Created by Hao Ye on 12/2/08.
 *  Copyright 2008. All rights reserved.
 *
 */



#include "vessel.h"

vessel::vessel()
{
	pollock.clear();
	chinook.clear();
	pollock_std.clear();
	chinook_std.clear();
	
	hit_A_limit = false;
	hit_B_limit = false;
	credits_bought_A = 0;
	credits_bought_B = 0;
}

vessel::~vessel()
{
}

void vessel::set_name(string name)
{
	this->name = name;
	return;
}

void vessel::set_coop(string coop)
{
	this->coop = coop;
	return;
}

void vessel::set_num_days(int num_days)
{
	pollock.assign(num_days, 0);
	chinook.assign(num_days, 0);
	pollock_std.assign(num_days, 0);
	chinook_std.assign(num_days, 0);
	return;
}
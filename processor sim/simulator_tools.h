/*
 *  simulator_tools.h
 *  processor
 *
 *  Created by Hao Ye on 12/2/08.
 *  Copyright 2008. All rights reserved.
 *
 */

#ifndef SIMULATOR_TOOLS_H
#define SIMULATOR_TOOLS_H

#include <string>
#include <vector>
#include "math.h"

using namespace std;

double shallow_slope(const double z_score);
double moderate_slope(const double z_score);
double linear(const double z_score);
double normal_pvalue(const double z_score, const vector<double> & z_table);

string parse_line(string line_buffer, int& position);
int parse_month(string value_buffer);
int parse_day(string value_buffer);
int day_count(int year, int month, int day);
string day_name(int day_count, int year);

#endif
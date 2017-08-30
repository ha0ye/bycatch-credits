/*
 *  simulator.h
 *  processor
 *
 *  Created by Hao Ye on 12/2/08.
 *  Copyright 2008. All rights reserved.
 *
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include "vessel.h"
#include "simulator_tools.h"

using namespace std;

struct landing
{
	int year;
	int month;
	int day;
	string ticketNumber;
	string name;
	string coop;
	double pollock;
	double chinook;
};

struct credit_factor
{
	vector<double> p_A;
	vector<double> p_B;
	vector<double> q_A;
	vector<double> q_B;
	string name;
	string coop;
	double cim_A, cim_B;
};

struct needy_struct
{
	int index;
	int amount;
	double bycatch_rate;
};

enum PenaltyType
{
	NORMAL,
	SHALLOW,
	MODERATE,
	LINEAR
};

enum TradingType
{
	DYNAMIC_SALMON_SAVINGS,
	FIXED_TRANSFER_TAX,
	NONE
};

class simulator
	{
	public:
		
		simulator();
		~simulator();
		
		void read_in_landings(istream & in);
		void load_z_table();
		void process();
		void process_year(const int year);
		void convert_data(const vector<landing> & raw_data, const int year, 
						  vector<landing> & year_data, vector<vessel> & vessel_data);
		void load_credit_factors(vector<vessel> & vessel_data);
		void process_data(vector<vessel> & vessel_data, const int year);
		void simulate_year(vector<vessel> & vessel_data, const int year, 
						   const vector<landing> & year_data);
		void simulate_A_season(vector<vessel> & vessel_data, const int year, 
							   const vector<landing> & year_data);
		void simulate_B_season(vector<vessel> & vessel_data, const int year, 
							   const vector<landing> & year_data);
		void update_credit_factors(vector<vessel> & vessel_data);
		void transfer_credits(vector<vessel> & vessel_data, const int year, const vector<landing> & year_data, const int start_index, const int day_index);
		void print_credit_data(vector<vessel> & vessel_data, const int year);
		void print_vessel_data(vector<vessel> & vessel_data, const int year);
		void print_credit_deltas(vector<vessel> & vessel_data, const int year);
		void print_unfished_data();
		void save_vessel_data(vector<vessel> & vessel_data);
		
	private:
		ofstream summary_file;
		vector<landing> raw_data;
		vector<string> column_names;
		vector<credit_factor> credit_factor_DB;
		vector<double> z_table;
		int num_days;
		int start_date;
		double season_pollock_A, season_pollock_B;
		vector<int> unfished_pollock_A;
		vector<int> unfished_pollock_B;
		vector<int> years;
		map<string, int> vessel_index;
		
		int b_season_first_catch;
		double bycatch_rate_cap_A, bycatch_rate_cap_B;
		int season_chinook_A, season_chinook_B;
		double credits_available, credits_held, credits_transferred;
		
		// parameters
		double HARD_CAP;
		double TARGET_CAP;
		double A_SEASON_FRAC;
		double B_SEASON_FRAC;
		double A_SEASON_CV_FRAC;
		double B_SEASON_CV_FRAC;
		
		double ALPHA;
		double BETA;
		double GAMMA;
		
		PenaltyType penalty_func;
		TradingType trading_rule;
		double DELTA;
		double EPSILON;
		
		double PURCHASE_LIMIT;
		double DYNAMIC_STRANDING_LIMIT;
		double stranding_rate;
		double TAX_RATE;
		
		double PSI;
		bool SSR_set;
		int SSR_set_date;
	};


#endif
/*
 *  vessel.h
 *  processor
 *
 *  Created by Hao Ye on 12/2/08.
 *  Copyright 2008. All rights reserved.
 *
 */

#ifndef VESSEL_H
#define VESSEL_H

#include <string>
#include <vector>

using namespace std;

class vessel
	{
	public:
		vessel();
		~vessel();
		
		string name;
		string coop;
		vector<double> pollock;
		vector<int> chinook;
		vector<double> pollock_std;
		vector<int> chinook_std;
		int credits;
		
		int init_credits_A, init_credits_B;
		double pollock_total, pollock_A, pollock_B;
		int chinook_total, chinook_A, chinook_B;
		double bycatch_rate_total, bycatch_rate_A, bycatch_rate_B;
		double credit_perc_A, credit_perc_B, credit_factor_A, credit_factor_B;
		double z_A, z_B, q_A, q_B;
		bool hit_A_limit, hit_B_limit;
		bool done_A, done_B;
		double credits_bought_A, credits_bought_B;
		int out_date_A, out_date_B;
		
		double actual_pollock_A, actual_pollock_B;
		int actual_chinook_A, actual_chinook_B;
		double actual_bycatch_rate_A, actual_bycatch_rate_B;
		double uncaught_pollock_A, uncaught_pollock_B;
		double cim_A, cim_B;
		
		void set_name(string name);
		void set_coop(string coop);
		void set_num_days(int num_days);
		
	private:
		
	};


#endif
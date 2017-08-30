/*
 *  simulator.cpp
 *  processor
 *
 *  Created by Hao Ye on 12/2/08.
 *  Copyright 2008. All rights reserved.
 *
 */

#include "simulator.h"

bool operator <(const needy_struct & a, const needy_struct & b);

simulator::simulator()
{
	// PPA settings
	HARD_CAP = 60000;
	TARGET_CAP = 47591;
	A_SEASON_FRAC = 0.70;
	B_SEASON_FRAC = 0.30;
	A_SEASON_CV_FRAC = 0.498;
	B_SEASON_CV_FRAC = 0.693;
	
	// reallocation function params
	ALPHA = 1.0 / 3.0;	// constant factor = 1/4 
	BETA = 1.0 / 3.0;	// legacy factor = 1/2
	GAMMA = 1.0 / 3.0;	// penalty factor = 1/4
	
	// penalty function params
	penalty_func = LINEAR;
	DELTA = 1.0 / 3.0; // constant factor = 1/3
	EPSILON = 4.0 / 3.0; // scaling factor = 4/3
	
	// trading params
	PURCHASE_LIMIT = 1.0 / 3.0; // percent of initial calculation to be bought
	DYNAMIC_STRANDING_LIMIT = 0.50; // maximum SSR
	TAX_RATE = 0.20;
	trading_rule = DYNAMIC_SALMON_SAVINGS;
	
	// incentive modeling params
	PSI = 0; // bycatch reduction factor
	
	// load table for normal distribution calculations
	load_z_table();
	
	summary_file.open("summary_output.txt");
	summary_file << "year" << "\t";
	summary_file << "target level" << "\t";
	summary_file << "credits distributed" << "\t";
	summary_file << "credits used" << "\t";
	summary_file << "credits transferred" << "\t";
	summary_file << "credits held" << "\t";
	summary_file << "total bycatch (original)" << "\n";
	
}

simulator::~simulator()
{
	summary_file.close();
}

void simulator::read_in_landings(istream & datafile)
{
	raw_data.clear();
	column_names.clear();
	
	string line_buffer;
	string value_buffer;
	int position;
	
	// process first line
	if (!datafile.eof()) // if not empty file, process first line
	{
		getline(datafile, line_buffer);
		position = 0;
		
		// count columns and check if first line has names
		while(position != line_buffer.length())
		{
			value_buffer = parse_line(line_buffer, position);
			column_names.push_back(value_buffer);
		}
	}
	
	int num_columns = column_names.size();
	
	// process rest of data
	landing temp_landing;
	
	while (!datafile.eof())
	{
		getline(datafile, line_buffer);
		
		// stop if blank line
		if (line_buffer.length() == 0)
			break;
		
		position = 0;
		for(int i = 0; i < num_columns; i++)
		{
			value_buffer = parse_line(line_buffer, position);
			switch(i)
			{
				case 0: // year
					temp_landing.year = strtod(value_buffer.c_str(), NULL);
					break;
				case 1: // date
					// grab month
					temp_landing.month = parse_month(value_buffer);
					
					// grab day
					temp_landing.day = parse_day(value_buffer);
					
					break;
				case 2: // ticket number
					break;
				case 3: // vessel
					temp_landing.name = value_buffer;
					break;
				case 4: // coop
					temp_landing.coop = value_buffer;
					break;
				case 5:
					break;
				case 6:
					temp_landing.pollock = strtod(value_buffer.c_str(), NULL);
					break;
				case 7:
					temp_landing.chinook = strtod(value_buffer.c_str(), NULL);
					break;
				default:
					cerr << "unexpected column index.\n";
					break;
			}
		}
		raw_data.push_back(temp_landing);
	}
	
	return;
}

void simulator::load_z_table()
{
	ifstream in;
	in.open("z-table.txt");
	
	z_table.resize(601);
	
	for(int i = 0; i < 601; i++)
	{
		in >> z_table[i];
	}
	in.close();
}

void simulator::process()
{
	credit_factor_DB.clear();
	
	unfished_pollock_A.clear();
	unfished_pollock_B.clear();
	
	process_year(2000);
	process_year(2001);
	process_year(2002);
	process_year(2003);
	process_year(2004);
	process_year(2005);
	process_year(2006);
	process_year(2007);
	
	print_unfished_data();
	
	return;
}

void simulator::process_year(const int year)
{
	// filter out the desired season
	vector<landing> year_data;
	vector<vessel> vessel_data;
	convert_data(raw_data, year, year_data, vessel_data);
	
	// load credit allocation factors
	load_credit_factors(vessel_data);
	
	// process
	process_data(vessel_data, year);
	
	// simulate
	simulate_year(vessel_data, year, year_data);
	
	// update credit allocation factors
	update_credit_factors(vessel_data);
	
	// print output
	print_credit_data(vessel_data, year);
	
	// print vessel data
	print_vessel_data(vessel_data, year);
	
	// print credit deltas
	//print_credit_deltas(vessel_data, year);
	
	// save data to db
	save_vessel_data(vessel_data);
	
	return;
}

void simulator::convert_data(const vector<landing> & raw_data, const int year, 
							 vector<landing> & year_data, vector<vessel> & vessel_data)
{
	int num_data = raw_data.size();
	int start_index = -1;
	int end_index = -1;
	for(int i = 0; i < num_data; i++)
	{
		if(raw_data[i].year == year)
		{
			if(start_index < 0)
				start_index = i;
			end_index = i;
		}
		if(raw_data[i].year > year)
			break;
	}
	
	year_data.assign(raw_data.begin() + start_index, raw_data.begin() + end_index + 1);
	
	vessel_data.clear();
	
	num_data = year_data.size();
	
	start_date = day_count(year_data[0].year, year_data[0].month, year_data[0].day);
	int end_date = day_count(year_data[num_data-1].year, year_data[num_data-1].month, year_data[num_data-1].day);
	
	vessel new_vessel;
	bool added;
	
	int day;
	num_days = end_date - start_date + 1;
	
	for(int i = 0; i < num_data; i++)
	{
		added = false;
		for(int j = 0; j < vessel_data.size(); j++)
		{
			if ((vessel_data[j].name.compare(year_data[i].name) == 0) && 
				(vessel_data[j].coop.compare(year_data[i].coop) == 0))
			{
				day = day_count(year_data[i].year, year_data[i].month, year_data[i].day) - start_date;
				
				vessel_data[j].pollock[day] += year_data[i].pollock;
				vessel_data[j].chinook[day] += int(year_data[i].chinook+0.5);
				
				added = true;
				break;
			}
		}
		if(!added)
		{
			day = day_count(year_data[i].year, year_data[i].month, year_data[i].day) - start_date;
			
			new_vessel.set_name(year_data[i].name);
			new_vessel.set_coop(year_data[i].coop);
			new_vessel.set_num_days(num_days);
			
			new_vessel.pollock[day] += year_data[i].pollock;
			new_vessel.chinook[day] += int(year_data[i].chinook+0.5);
			
			vessel_index[new_vessel.name] = vessel_data.size();
			
			vessel_data.push_back(new_vessel);
		}
	}
	
	return;
}

void simulator::load_credit_factors(vector<vessel> & vessel_data)
{
	int num_vessels = vessel_data.size();
	int DB_size = credit_factor_DB.size();
	int found;
	credit_factor new_credit_factor;
	
	for(int i = 0; i < num_vessels; i++)
	{
		found = -1;
		for(int j = 0; j < DB_size; j++)
		{
			if ((vessel_data[i].name.compare(credit_factor_DB[j].name) == 0) && 
				(vessel_data[i].coop.compare(credit_factor_DB[j].coop) == 0))
			{
				found = j;
				break;
			}
		}
		if(found < 0)
		{
			new_credit_factor.name = vessel_data[i].name;
			new_credit_factor.coop = vessel_data[i].coop;
			new_credit_factor.p_A.assign(1, 1);
			new_credit_factor.p_B.assign(1, 1);
			new_credit_factor.q_A.assign(1, 1);
			new_credit_factor.q_B.assign(1, 1);
			new_credit_factor.cim_A = 1;
			new_credit_factor.cim_B = 1;
			
			found = credit_factor_DB.size();
			credit_factor_DB.push_back(new_credit_factor);
		}
		
		vessel_data[i].credit_factor_A = ALPHA + 
		BETA * credit_factor_DB[found].p_A.back() + 
		GAMMA * credit_factor_DB[found].q_A.back();
		vessel_data[i].credit_factor_B = ALPHA + 
		BETA * credit_factor_DB[found].p_B.back() + 
		GAMMA * credit_factor_DB[found].q_B.back();
		vessel_data[i].cim_A = credit_factor_DB[found].cim_A;
		vessel_data[i].cim_B = credit_factor_DB[found].cim_B;
	}
	return;
}

void simulator::process_data(vector<vessel> & vessel_data, const int year)
{
	int num_vessels;
	num_vessels = vessel_data.size();
	double pollock_left;
	int chinook_left;
	
	int start_b_season = day_count(year, 6, 11) - start_date;
	
	// compute totals for A season
	season_pollock_A = 0;
	season_chinook_A = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].pollock_A = vessel_data[i].pollock[0];
		vessel_data[i].chinook_A = vessel_data[i].cim_A * vessel_data[i].chinook[0];
		vessel_data[i].pollock_std[0] = vessel_data[i].pollock[0];
		vessel_data[i].chinook_std[0] = vessel_data[i].cim_A * vessel_data[i].chinook[0];
		
		for(int j = 1; j < start_b_season; j++)
		{
			vessel_data[i].pollock_A += vessel_data[i].pollock[j];
			vessel_data[i].chinook_A += vessel_data[i].cim_A * vessel_data[i].chinook[j];
			
			vessel_data[i].pollock_std[j] = vessel_data[i].pollock_std[j-1] + vessel_data[i].pollock[j];
			vessel_data[i].chinook_std[j] = vessel_data[i].chinook_std[j-1] + vessel_data[i].cim_A * vessel_data[i].chinook[j];
		}
		season_pollock_A += vessel_data[i].pollock_A;
		season_chinook_A += vessel_data[i].chinook_A;
		
		vessel_data[i].bycatch_rate_A = vessel_data[i].chinook_A / vessel_data[i].pollock_A;
	}
	
	// compute totals for B season
	season_pollock_B = 0;
	season_chinook_B = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].pollock_B = vessel_data[i].pollock[start_b_season];
		vessel_data[i].chinook_B = vessel_data[i].cim_B * vessel_data[i].chinook[start_b_season];
		vessel_data[i].pollock_std[start_b_season] = vessel_data[i].pollock[start_b_season];
		vessel_data[i].chinook_std[start_b_season] = vessel_data[i].cim_B * vessel_data[i].chinook[start_b_season];
		
		for(int j = start_b_season+1; j < num_days; j++)
		{
			vessel_data[i].pollock_B += vessel_data[i].pollock[j];
			vessel_data[i].chinook_B += vessel_data[i].cim_B * vessel_data[i].chinook[j];
			
			vessel_data[i].pollock_std[j] = vessel_data[i].pollock_std[j-1] + vessel_data[i].pollock[j];
			vessel_data[i].chinook_std[j] = vessel_data[i].chinook_std[j-1] + vessel_data[i].cim_B * vessel_data[i].chinook[j];
		}
		season_pollock_B += vessel_data[i].pollock_B;
		season_chinook_B += vessel_data[i].chinook_B;
		
		vessel_data[i].bycatch_rate_B = vessel_data[i].chinook_B / vessel_data[i].pollock_B;
		vessel_data[i].pollock_total = vessel_data[i].pollock_A + vessel_data[i].pollock_B;
		vessel_data[i].chinook_total = vessel_data[i].chinook_A + vessel_data[i].chinook_B;
		vessel_data[i].bycatch_rate_total = vessel_data[i].chinook_total / vessel_data[i].pollock_total;
	}
	
	double pollock_std;
	for(int j = start_b_season+1; j < num_days; j++)
	{
		pollock_std = 0;
		for(int i = 0; i < num_vessels; i++)
		{
			pollock_std += vessel_data[i].pollock_std[j];
		}
		if(pollock_std >= 2.0 / 3.0 * season_pollock_B)
		{
			SSR_set_date = j;
			break;
		}
	}
	
	// compute credit allocations
	double total_credit_perc_A = 0;
	double total_credit_perc_B = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].credit_perc_A = vessel_data[i].pollock_A / season_pollock_A * vessel_data[i].credit_factor_A;
		vessel_data[i].credit_perc_B = vessel_data[i].pollock_B / season_pollock_B * vessel_data[i].credit_factor_B;
		total_credit_perc_A += vessel_data[i].credit_perc_A;
		total_credit_perc_B += vessel_data[i].credit_perc_B;
	}
	
	// compute total credits for the season
	double credits_A = TARGET_CAP * A_SEASON_FRAC * A_SEASON_CV_FRAC;
	double credits_B = TARGET_CAP * B_SEASON_FRAC * B_SEASON_CV_FRAC;
	double max_credits_A = HARD_CAP * A_SEASON_FRAC * A_SEASON_CV_FRAC;
	double max_credits_B = HARD_CAP * B_SEASON_FRAC * B_SEASON_CV_FRAC;
	
	// rescale number of credits if exceeding hard cap
	if(total_credit_perc_A > max_credits_A / credits_A)
	{
		for(int i = 0; i < num_vessels; i++)
		{
			vessel_data[i].credit_perc_A /= total_credit_perc_A * max_credits_A / credits_A;
		}
	}
	if(total_credit_perc_B > max_credits_B / credits_B)
	{
		for(int i = 0; i < num_vessels; i++)
		{
			vessel_data[i].credit_perc_B /= total_credit_perc_B * max_credits_B / credits_B;
		}
	}
	
	bycatch_rate_cap_A = credits_A / season_pollock_A;
	bycatch_rate_cap_B = credits_B / season_pollock_B;
	
	// allocate credits for each vessel
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].init_credits_A = int(vessel_data[i].credit_perc_A * credits_A);
		vessel_data[i].init_credits_B = int(vessel_data[i].credit_perc_B * credits_B);
	}
	return;
}

void simulator::simulate_year(vector<vessel> & vessel_data, const int year, 
							  const vector<landing> & year_data)
{
	int num_vessels = vessel_data.size();
	
	// initialize vessel data
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].credits = vessel_data[i].init_credits_A;
		vessel_data[i].actual_pollock_A = 0;
		vessel_data[i].actual_pollock_B = 0;
		vessel_data[i].actual_chinook_A = 0;
		vessel_data[i].actual_chinook_B = 0;
		vessel_data[i].out_date_A = 9999;
		vessel_data[i].out_date_B = 9999;
		vessel_data[i].hit_A_limit = false;
		vessel_data[i].hit_B_limit = false;
		vessel_data[i].done_A = false;
		vessel_data[i].done_B = false;
	}
	
	SSR_set = false;
	if(trading_rule == DYNAMIC_SALMON_SAVINGS)
	{
		stranding_rate = DYNAMIC_STRANDING_LIMIT;
	}
	else
	{
		stranding_rate = 0;
	}
	credits_available = 0;
	credits_held = 0;
	credits_transferred = 0;
	
	simulate_A_season(vessel_data, year, year_data);
	simulate_B_season(vessel_data, year, year_data);
	
	// compute lost revenue and bycatch rate
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].uncaught_pollock_A = vessel_data[i].pollock_A - vessel_data[i].actual_pollock_A;
		if (vessel_data[i].pollock_A > 0)
			vessel_data[i].actual_bycatch_rate_A = vessel_data[i].actual_chinook_A / vessel_data[i].actual_pollock_A;
		
		vessel_data[i].uncaught_pollock_B = vessel_data[i].pollock_B - vessel_data[i].actual_pollock_B;
		if (vessel_data[i].pollock_B > 0)
			vessel_data[i].actual_bycatch_rate_B = vessel_data[i].actual_chinook_B / vessel_data[i].actual_pollock_B;
	}
	
	double total_bycatch = 0;
	double total_init_credits = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		if (vessel_data[i].pollock_A > 0)
		{
			total_bycatch += vessel_data[i].actual_chinook_A;
			total_init_credits += vessel_data[i].init_credits_A;
		}
		if (vessel_data[i].pollock_B > 0)
		{
			total_bycatch += vessel_data[i].actual_chinook_B;
			total_init_credits += vessel_data[i].init_credits_B;
		}
	}
	summary_file << year << "\t";
	summary_file << TARGET_CAP * (A_SEASON_FRAC * A_SEASON_CV_FRAC + B_SEASON_FRAC * B_SEASON_CV_FRAC) << "\t";
	summary_file << total_init_credits << "\t";
	summary_file << total_bycatch << "\t";
	summary_file << credits_transferred << "\t";
	summary_file << credits_held << "\t";
	summary_file << season_chinook_A + season_chinook_B << "\n";

	return;
}

void simulator::simulate_A_season(vector<vessel> & vessel_data, const int year, 
								  const vector<landing> & year_data)
{
	int num_data = year_data.size();
	int num_vessels = vessel_data.size();
	int b_season_start_date = day_count(year, 6, 11) - start_date;
	int day_index, index;
	double fishable_ratio;
	int prev_day;
	int credits_needed, unused_credits;
	
	prev_day = -1;
	for(int i = 0; i < num_data; i++)
	{
		day_index = day_count(year, year_data[i].month, year_data[i].day) - start_date;
		
		// stop when at b_season
		if(day_index >= b_season_start_date)
		{
			b_season_first_catch = i;
			break;
		}
		
		// transfer credits if possible to cleanest vessels that need credits
		if(day_index != prev_day)
		{
			transfer_credits(vessel_data, year, year_data, i, day_index);
			prev_day = day_index;
		}
		
		index = vessel_index[year_data[i].name];
		
		if(vessel_data[index].credits > 0) // able to fish
		{
			credits_needed = int(vessel_data[index].cim_A * year_data[i].chinook + 0.5);
			if(credits_needed > vessel_data[index].credits) // if not enough ITEC for this haul, use fractional haul
			{
				fishable_ratio = double(vessel_data[index].credits) / credits_needed;
				vessel_data[index].actual_pollock_A += fishable_ratio * year_data[i].pollock;
				vessel_data[index].actual_chinook_A += vessel_data[index].credits;
				vessel_data[index].credits = 0;
			}
			else // use entire haul
			{
				vessel_data[index].actual_pollock_A += year_data[i].pollock;
				vessel_data[index].actual_chinook_A += credits_needed;
				vessel_data[index].credits -= credits_needed;
			}
		}
		
		if(!vessel_data[index].hit_A_limit) // check if vessel ran out
		{
			if(vessel_data[index].credits < 0) // out of credits
			{
				vessel_data[index].out_date_A = day_index;
				vessel_data[index].hit_A_limit = true;
			}
			else if(vessel_data[index].credits == 0 && vessel_data[index].pollock_A > vessel_data[index].pollock_std[day_index])
			{
				vessel_data[index].out_date_A = day_index;
				vessel_data[index].hit_A_limit = true;
			}
		}
		
		
		// handle vessel completion of A season fishing
		if(!vessel_data[index].done_A && 
		   vessel_data[index].pollock_std[day_index] > (vessel_data[index].pollock_A - 0.01)) // done fishing by this date
		{
			vessel_data[index].done_A = true; // ok, we are done counting this vessel
			unused_credits = vessel_data[index].credits;
			// cerr << vessel_data[index].name << " finished fishing ";
			// cerr << "with " << unused_credits << " credits remaining\n";
			switch(trading_rule)
			{
				case DYNAMIC_SALMON_SAVINGS:
					credits_available += unused_credits * (1 - stranding_rate);
					credits_held += unused_credits * stranding_rate;
					break;
				case FIXED_TRANSFER_TAX:
					credits_available += unused_credits / (1 + TAX_RATE);
					break;
				case NONE:
					credits_available += unused_credits;
			}
			vessel_data[index].credits = 0;
		}
	}
	
	return;
}

void simulator::simulate_B_season(vector<vessel> & vessel_data, const int year, 
								  const vector<landing> & year_data)
{
	int num_data = year_data.size();
	int num_vessels = vessel_data.size();
	int b_season_start_date = day_count(year, 6, 11) - start_date;
	int day_index, index;
	double fishable_ratio;
	int prev_day;
	int credits_needed, unused_credits;
	
	// influx of B season credits
	for(int i = 0; i < num_vessels; i++)
	{
		if(vessel_data[i].credits < 0)
			vessel_data[i].credits = 0;
		vessel_data[i].credits += vessel_data[i].init_credits_B;
	}	
	
	prev_day = -1;
	double expected_credits, chinook_std;
	double new_credits_held, credit_supply;
	for(int i = b_season_first_catch; i < num_data; i++)
	{
		day_index = day_count(year, year_data[i].month, year_data[i].day) - start_date;
		
		// set SSR if needed
		if(trading_rule == DYNAMIC_SALMON_SAVINGS && !SSR_set && day_index == SSR_set_date)
		{
			SSR_set = true;
			chinook_std = 0;
			credit_supply = credits_available;
			for(int j = 0; j < num_vessels; j++)
			{
				chinook_std += vessel_data[j].actual_chinook_B;
				credit_supply += vessel_data[j].credits;
			}
			expected_credits = chinook_std * 9 + 5000;
			/*
			 cerr << "for " << year << ":\n";
			 cerr << "bycatch to date for season B: " << chinook_std << "\n";
			 cerr << "predicted bycatch for B season remaining: " << expected_credits << "\n";
			 cerr << "credits available: " << credit_supply << "\n";
			 */
			if(expected_credits > credit_supply)
			{
				stranding_rate = 0;
				credits_available += credits_held;
				credits_held = 0;
			}
			else
			{
				stranding_rate = (1.0 * (credit_supply - expected_credits)) / credit_supply;
				if(stranding_rate > DYNAMIC_STRANDING_LIMIT) // use max SSR
					stranding_rate = DYNAMIC_STRANDING_LIMIT;
				else // correct for previous withholding
				{
					new_credits_held = credits_held / DYNAMIC_STRANDING_LIMIT * stranding_rate;
					credits_available += (credits_held - new_credits_held);
					credits_held = new_credits_held;
				}
			}
			cerr << "SSR = " << stranding_rate << "\n";
		}
		
		
		// transfer credits if possible to cleanest vessels that need credits
		if(day_index != prev_day)
		{
			transfer_credits(vessel_data, year, year_data, i, day_index);
			prev_day = day_index;
		}
		
		index = vessel_index[year_data[i].name];
		
		if(vessel_data[index].credits > 0) // able to fish
		{
			credits_needed = int(vessel_data[index].cim_B * year_data[i].chinook + 0.5);
			if(credits_needed > vessel_data[index].credits) // if not enough ITEC for this haul, use fractional haul
			{
				fishable_ratio = double(vessel_data[index].credits) / credits_needed;
				vessel_data[index].actual_pollock_B += fishable_ratio * year_data[i].pollock;
				vessel_data[index].actual_chinook_B += vessel_data[index].credits;
				vessel_data[index].credits = 0;
			}
			else // use entire haul
			{
				vessel_data[index].actual_pollock_B += year_data[i].pollock;
				vessel_data[index].actual_chinook_B += credits_needed;
				vessel_data[index].credits -= credits_needed;
			}
		}
		
		if(!vessel_data[index].hit_B_limit) // check if vessel ran out
		{
			if(vessel_data[index].credits < 0) // out of credits
			{
				vessel_data[index].out_date_B = day_index;
				vessel_data[index].hit_B_limit = true;
			}
			else if(vessel_data[index].credits == 0 && vessel_data[index].pollock_B > vessel_data[index].pollock_std[day_index])
			{
				vessel_data[index].out_date_B = day_index;
				vessel_data[index].hit_B_limit = true;
			}
		}
		
		
		// handle vessel completion of B season fishing
		if(!vessel_data[index].done_B && 
		   vessel_data[index].pollock_std[day_index] > (vessel_data[index].pollock_B - 0.01)) // done fishing by this date
		{
			vessel_data[index].done_B = true; // ok, we are done counting this vessel
			unused_credits = vessel_data[index].credits;
			// cerr << vessel_data[index].name << " finished fishing ";
			// cerr << "with " << unused_credits << " credits remaining\n";
			switch(trading_rule)
			{
				case DYNAMIC_SALMON_SAVINGS:
					credits_available += unused_credits * (1 - stranding_rate);
					credits_held += unused_credits * stranding_rate;
					break;
				case FIXED_TRANSFER_TAX:
					credits_available += unused_credits / (1 + TAX_RATE);
					break;
				case NONE:
					credits_available += unused_credits;
					break;
			}
			vessel_data[index].credits = 0;
		}
	}
	return;
}

void simulator::update_credit_factors(vector<vessel> & vessel_data)
{
	int num_vessels = vessel_data.size();
	int DB_size = credit_factor_DB.size();
	
	double mean, var, stdev, adj_stdev, z_score, p, q;
	double squared_vals, summed_vals, total_pollock;
	int count, found;
	double actual_chinook, actual_pollock;
	
	// compute stats for bycatch rate in season A
	{
		squared_vals = 0;
		summed_vals = 0;
		count = 0;
		actual_chinook = 0;
		actual_pollock = 0;
		for(int i = 0; i < num_vessels; i++)
		{
			if(vessel_data[i].pollock_A > 0)
			{
				squared_vals += vessel_data[i].actual_bycatch_rate_A * vessel_data[i].actual_bycatch_rate_A;
				summed_vals += vessel_data[i].actual_bycatch_rate_A;
				count ++;
			}
			actual_pollock += vessel_data[i].actual_pollock_A;
			actual_chinook += vessel_data[i].actual_chinook_A;
		}
		mean = summed_vals / count;
		var = squared_vals / count - mean * mean;
		stdev = sqrt(var);
		stdev = 0.6855 * actual_chinook / actual_pollock;
		if(mean > bycatch_rate_cap_A)
			mean = bycatch_rate_cap_A;
	}
	for(int i = 0; i < num_vessels; i++)
	{
		if(vessel_data[i].pollock_A > 0)
		{
			// find vessel
			{
				found = -1;
				for(int j = 0; j < DB_size; j++)
				{
					if ((vessel_data[i].name.compare(credit_factor_DB[j].name) == 0) && 
						(vessel_data[i].coop.compare(credit_factor_DB[j].coop) == 0))
					{
						found = j;
						break;
					}
				}
				if(found < 0)
				{
					cerr << "an error has occurred.\n";
					exit(-1);
				}
			}
			
			// compute penalty value
			{
				adj_stdev = stdev * sqrt(1 + 1.0/count) / sqrt(1 + vessel_data[i].pollock_A / season_pollock_A);
				z_score = (mean - vessel_data[i].actual_bycatch_rate_A) / adj_stdev;
				switch(penalty_func)
				{
					case SHALLOW:
						p = shallow_slope(z_score);
						break;
					case NORMAL:
						p = normal_pvalue(z_score, z_table);
						break;
					case MODERATE:
						p = moderate_slope(z_score);
						break;
					case LINEAR:
						p = linear(z_score);
						break;
				}
				q = EPSILON * p + DELTA;
			}
			
			credit_factor_DB[found].p_A.push_back(vessel_data[i].credit_factor_A);
			credit_factor_DB[found].q_A.push_back(q);
			credit_factor_DB[found].cim_A *= (1.0 - PSI / (1.0 + q));
			vessel_data[i].z_A = z_score;
			vessel_data[i].q_A = q;
			vessel_data[i].cim_A = credit_factor_DB[found].cim_A;
		}
	}
	
	// compute stats for bycatch rate in season B
	{
		squared_vals = 0;
		summed_vals = 0;
		count = 0;
		actual_chinook = 0;
		actual_pollock = 0;
		for(int i = 0; i < num_vessels; i++)
		{
			if(vessel_data[i].pollock_B > 0)
			{
				squared_vals += vessel_data[i].actual_bycatch_rate_B * vessel_data[i].actual_bycatch_rate_B;
				summed_vals += vessel_data[i].actual_bycatch_rate_B;
				count ++;
			}
			actual_pollock += vessel_data[i].actual_pollock_B;
			actual_chinook += vessel_data[i].actual_chinook_B;
		}
		mean = summed_vals / count; // average bycatch rate
		var = squared_vals / count - mean * mean;
		stdev = sqrt(var);
		stdev = 0.6855 * actual_chinook / actual_pollock;
		if(mean > bycatch_rate_cap_B)
			mean = bycatch_rate_cap_B;
	}
	
	for(int i = 0; i < num_vessels; i++)
	{
		if(vessel_data[i].pollock_B > 0)
		{
			// find vessel
			{
				found = -1;
				for(int j = 0; j < DB_size; j++)
				{
					if ((vessel_data[i].name.compare(credit_factor_DB[j].name) == 0) && 
						(vessel_data[i].coop.compare(credit_factor_DB[j].coop) == 0))
					{
						found = j;
						break;
					}
				}
				if(found < 0)
				{
					cerr << "an error has occurred.\n";
					exit(-1);
				}
			}
			
			// compute penalty value
			{
				adj_stdev = stdev * sqrt(1 + 1.0/count) / sqrt(1 + vessel_data[i].pollock_B / season_pollock_B);
				z_score = (mean - vessel_data[i].actual_bycatch_rate_B) / adj_stdev;
				switch(penalty_func)
				{
					case SHALLOW:
						p = shallow_slope(z_score);
						break;
					case NORMAL:
						p = normal_pvalue(z_score, z_table);
						break;
					case MODERATE:
						p = moderate_slope(z_score);
						break;
					case LINEAR:
						p = linear(z_score);
						break;
				}
				q = EPSILON * p + DELTA;
			}
			
			credit_factor_DB[found].p_B.push_back(vessel_data[i].credit_factor_B);
			credit_factor_DB[found].q_B.push_back(q);
			credit_factor_DB[found].cim_B *= (1.0 - PSI / (1.0 + q));
			vessel_data[i].z_B = z_score;
			vessel_data[i].q_B = q;
			vessel_data[i].cim_B = credit_factor_DB[found].cim_B;
		}
	}
	return;
}

void simulator::transfer_credits(vector<vessel> & vessel_data, const int year, const vector<landing> & year_data, const int start_index, const int day_index)
{
	int num_data = year_data.size();
	int num_vessels = vessel_data.size();
	int i = start_index;
	int day, index;
	vector<needy_struct> needy_db;
	needy_struct vessel_need;
	int credits_needed;
	
	if(credits_available == 0 || trading_rule == NONE)
		return;
	
	// figure out which vessels need credits
	needy_db.clear();
	day = day_count(year, year_data[i].month, year_data[i].day) - start_date;
	for(int i = start_index; i < num_data; i++)
	{
		day = day_count(year, year_data[i].month, year_data[i].day) - start_date;
		if(day != day_index)
			break;
		
		index = vessel_index[year_data[i].name];
		credits_needed = int(vessel_data[index].cim_A * year_data[i].chinook + 0.5);
		
		if(credits_needed > vessel_data[index].credits) // vessel needs credits
		{
			vessel_need.index = index;
			vessel_need.amount = credits_needed;
			vessel_need.bycatch_rate = vessel_data[index].actual_chinook_A / vessel_data[index].actual_pollock_A;
			needy_db.push_back(vessel_need);
		}
	}
	sort(needy_db.begin(), needy_db.end());
	
	for(int i = 0; i < needy_db.size(); i++)
	{
		if(credits_available == 0)
			break;
		else if(credits_available > needy_db[i].amount)
		{
			vessel_data[needy_db[i].index].credits += needy_db[i].amount;
			credits_available -= needy_db[i].amount;
			credits_transferred += needy_db[i].amount;
			if(trading_rule = FIXED_TRANSFER_TAX)
				credits_held += TAX_RATE * needy_db[i].amount;
			//cerr << "transferred " << needy_db[i].amount << " to " << vessel_data[needy_db[i].index].name << "\n";
		}
		else
		{
			vessel_data[needy_db[i].index].credits += credits_available;
			//cerr << "transferred " << credits_available << " to " << vessel_data[needy_db[i].index].name << "\n";
			credits_transferred += credits_available;
			if(trading_rule = FIXED_TRANSFER_TAX)
				credits_held += TAX_RATE * credits_available;
			credits_available = 0;
		}
	}
	
	return;
}

void simulator::print_credit_data(vector<vessel> & vessel_data, const int year)
{
	char filename[40];
	sprintf(filename, "credit_supply_demand.%d.csv", year);
	ofstream out;
	out.open(filename);
	
	int start_b_season = day_count(year, 6, 11) - start_date;
	
	// header row
	out << "Date, ";
	//out << "Credits (for sale), ";
	out << "Vessels (out of credits),";
	out << "Pollock, Pollock (std),";
	out << "Bycatch, Bycatch (std), Bycatch Rate\n";
	
	int num_vessels = vessel_data.size();
	double pollock, pollock_std;
	int bycatch, bycatch_std;
	int credits, num_limit_vessels;
	bool b_flag = false;
	
	for(int i = 0; i < num_days; i++)
	{
		if(i == start_b_season)
		{
			out << "\n";
			b_flag = true;
		}
		pollock = 0;
		bycatch = 0;
		pollock_std = 0;
		bycatch_std = 0;
		credits = 0;
		num_limit_vessels = 0;
		for(int j = 0; j < num_vessels; j++)
		{
			pollock += vessel_data[j].pollock[i];
			bycatch += vessel_data[j].chinook[i];
			pollock_std += vessel_data[j].pollock_std[i];
			bycatch_std += vessel_data[j].chinook_std[i];
			if((b_flag && (i >= vessel_data[j].out_date_B)) ||
			   (!b_flag && (i >= vessel_data[j].out_date_A)))
				num_limit_vessels ++;
		}
		
		//if((pollock > 0) || (i == start_b_season-1))
		{
			out << day_name(i + start_date, year) << ",";
			//out << credits << ",";
			out << num_limit_vessels << ",";
			out << pollock << ",";
			out << pollock_std << ",";
			out << bycatch << ",";
			out << bycatch_std << ",";
			out << bycatch / pollock << "\n";
		}
	}
	
	out.close();
	return;
}

void simulator::print_vessel_data(vector<vessel> & vessel_data, const int year)
{
	char filename[40];
	sprintf(filename, "vessel_data.%d.csv", year);
	ofstream out;
	out.open(filename);
	
	int start_b_season = day_count(year, 6, 11) - start_date;
	
	// header row
	out << ",,";
	out << "A Season,,,,,,,,";
	out << "B Season,,,,,,,,";
	out << year << "\n";
	out << "Vessel Name,Coop,";
	out << "Pollock,Bycatch,Uncaught Pollock,Bycatch Rate,Credit Factor,Credits,z-score,q-value,";
	out << "Pollock,Bycatch,Uncaught Pollock,Bycatch Rate,Credit Factor,Credits,z-score,q-value,";
	out << "Pollock,Bycatch,Uncaught Pollock,Bycatch Rate,Credits\n";
	
	int num_vessels = vessel_data.size();
	double credits_needed;
	
	for(int j = 0; j < num_vessels; j++)
	{
		out << vessel_data[j].name << ",";
		out << vessel_data[j].coop << ",";
		
		// A season data
		out << vessel_data[j].actual_pollock_A << ",";
		out << vessel_data[j].actual_chinook_A << ",";
		if(vessel_data[j].pollock_A > 0)
		{
			out << vessel_data[j].uncaught_pollock_A << ",";
			out << vessel_data[j].actual_bycatch_rate_A << ",";
			out << vessel_data[j].credit_factor_A << ",";
			out << vessel_data[j].init_credits_A << ",";
			out << vessel_data[j].z_A << ",";
			out << vessel_data[j].q_A << ",";
		}
		else
		{
			out << ",,,,,,";
		}
		
		// B season data
		out << vessel_data[j].actual_pollock_B << ",";
		out << vessel_data[j].actual_chinook_B << ",";
		if(vessel_data[j].pollock_B > 0)
		{
			out << vessel_data[j].uncaught_pollock_B << ",";
			out << vessel_data[j].actual_bycatch_rate_B << ",";
			out << vessel_data[j].credit_factor_B << ",";
			out << vessel_data[j].init_credits_B << ",";
			out << vessel_data[j].z_B << ",";
			out << vessel_data[j].q_B << ",";
		}
		else
		{
			out << ",,,,,,";
		}
		
		// yearly data
		out << vessel_data[j].actual_pollock_A + vessel_data[j].actual_pollock_B << ",";
		out << vessel_data[j].actual_chinook_A + vessel_data[j].actual_chinook_B  << ",";
		out << vessel_data[j].uncaught_pollock_A + vessel_data[j].uncaught_pollock_B << ",";
		out << vessel_data[j].bycatch_rate_total << ",";
		out << vessel_data[j].init_credits_A + vessel_data[j].init_credits_B << "\n";
	}
	
	double pollock_A = 0, pollock_B = 0, uncaught_pollock_A = 0, uncaught_pollock_B = 0;
	int chinook_A = 0, chinook_B = 0, init_credits_A = 0, init_credits_B = 0;
	double bycatch_rate_A, bycatch_rate_B;
	
	for(int j = 0; j < num_vessels; j++)
	{
		// A season data
		pollock_A += vessel_data[j].actual_pollock_A;
		chinook_A += vessel_data[j].actual_chinook_A;
		uncaught_pollock_A += vessel_data[j].uncaught_pollock_A;
		init_credits_A += vessel_data[j].init_credits_A;
		
		// B season data
		pollock_B += vessel_data[j].actual_pollock_B;
		chinook_B += vessel_data[j].actual_chinook_B;
		uncaught_pollock_B += vessel_data[j].uncaught_pollock_B;
		init_credits_B += vessel_data[j].init_credits_B;
	}
	bycatch_rate_A = chinook_A / pollock_A;
	bycatch_rate_B = chinook_B / pollock_B;
	out << "\n";
	out << "TOTAL,,";
	out << pollock_A << "," << chinook_A << "," << uncaught_pollock_A << ",";
	out << bycatch_rate_A << ",," << init_credits_A << ",,,";
	out << pollock_B << "," << chinook_B << "," << uncaught_pollock_B << ",";
	out << bycatch_rate_B << ",," << init_credits_B << ",,,";
	out << pollock_A + pollock_B << "," << chinook_A + chinook_B << ",";
	out << uncaught_pollock_A + uncaught_pollock_B << ",";
	out << (chinook_A + chinook_B) / (pollock_A + pollock_B) << ",";
	out << init_credits_A + init_credits_B << "\n";
	
	out.close();
	
	unfished_pollock_A.push_back(uncaught_pollock_A);
	unfished_pollock_B.push_back(uncaught_pollock_B);
	years.push_back(year);
	
	return;
}

void simulator::print_credit_deltas(vector<vessel> & vessel_data, const int year)
{
	char filename[40];
	sprintf(filename, "credit_delta_calc.%d.csv", year);
	ofstream out;
	out.open(filename);
	
	int start_b_season = day_count(year, 6, 11) - start_date;
	int DELTA_BYCATCH = -10;
	
	// header row
	out << ",,";
	out << "A Season,,,,,,,,,,,,,,,,,";
	out << "B Season,,,,,,,,,,,,,,,,";
	out << "\n";
	out << "Vessel Name,Coop,";
	out << "Pollock,Bycatch,Bycatch Rate,z-score,q-value,";
	out << "Credit Factor (old),Credit Factor (new),% share,Credits (old),Credits (new),";
	out << "Bycatch (adj),Bycatch Rate (adj),z-score (adj),q-value (adj),Credit Factor (adj),Credits (adj),Delta,";
	out << "Pollock,Bycatch,Bycatch Rate,z-score,q-value,";
	out << "Credit Factor (old),Credit Factor (new),% share,Credits (old),Credits (new),";
	out << "Bycatch (adj),Bycatch Rate (adj),z-score (adj),q-value (adj),Credit Factor (adj),Credits (adj),Delta";
	out << "\n";
	
	int num_vessels = vessel_data.size();
	double credits_needed;
	double new_credit_factor;
	double credits_A = TARGET_CAP * A_SEASON_FRAC * A_SEASON_CV_FRAC;
	double credits_B = TARGET_CAP * B_SEASON_FRAC * B_SEASON_CV_FRAC;
	
	double pollock_A = 0, pollock_B = 0;
	int chinook_A = 0, chinook_B = 0, init_credits_A = 0, init_credits_B = 0;
	int new_credits_A = 0, new_credits_B = 0;
	double bycatch_rate_A, bycatch_rate_B;
	double sum_A = 0, sum_B = 0;
	double count_A = 0, count_B = 0;
	double mean_A, mean_B;
	double stdev_A, stdev_B;
	double bycatch_adj, bycatch_rate_adj, credit_factor_adj;
	double mean_adj, stdev_adj, z_adj, q_adj, p_adj;;
	
	vector<int> deltas;
	deltas.clear();
	
	for(int j = 0; j < num_vessels; j++)
	{
		// A season data
		pollock_A += vessel_data[j].actual_pollock_A;
		chinook_A += vessel_data[j].actual_chinook_A;
		init_credits_A += vessel_data[j].init_credits_A;
		if(vessel_data[j].pollock_A > 0)
		{
			sum_A += vessel_data[j].actual_bycatch_rate_A;
			count_A ++;
		}
		
		// B season data
		pollock_B += vessel_data[j].actual_pollock_B;
		chinook_B += vessel_data[j].actual_chinook_B;
		init_credits_B += vessel_data[j].init_credits_B;
		if(vessel_data[j].pollock_B > 0)
		{
			sum_B += vessel_data[j].actual_bycatch_rate_B;
			count_B ++;
		}
	}
	bycatch_rate_A = chinook_A / pollock_A;
	bycatch_rate_B = chinook_B / pollock_B;
	mean_A = sum_A / count_A;
	if(mean_A > bycatch_rate_cap_A)
		mean_A = bycatch_rate_cap_A;
	mean_B = sum_B / count_B;
	if(mean_B > bycatch_rate_cap_B)
		mean_B = bycatch_rate_cap_B;
	stdev_A = 0.6855 * double(chinook_A + DELTA_BYCATCH) / pollock_A;
	stdev_B = 0.6855 * double(chinook_B + DELTA_BYCATCH) / pollock_B;
	
	for(int j = 0; j < num_vessels; j++)
	{
		out << vessel_data[j].name << ",";
		out << vessel_data[j].coop << ",";
		
		// A season data
		out << vessel_data[j].actual_pollock_A << ",";
		out << vessel_data[j].actual_chinook_A << ",";
		if(vessel_data[j].pollock_A > 0)
		{
			out << vessel_data[j].actual_bycatch_rate_A << ",";
			out << vessel_data[j].z_A << ",";
			out << vessel_data[j].q_A << ",";
			out << vessel_data[j].credit_factor_A << ",";
			new_credit_factor = ALPHA + BETA * vessel_data[j].credit_factor_A + GAMMA * vessel_data[j].q_A;
			out << new_credit_factor << ",";
			out << vessel_data[j].pollock_A / season_pollock_A << ",";
			out << vessel_data[j].init_credits_A << ",";
			out << int(vessel_data[j].pollock_A / season_pollock_A * new_credit_factor * credits_A) << ",";
			
			bycatch_adj = vessel_data[j].actual_chinook_A + DELTA_BYCATCH;
			if(bycatch_adj < 0)
				bycatch_adj = 0;
			bycatch_rate_adj = double(bycatch_adj) / vessel_data[j].actual_pollock_A;
			//mean_adj = (sum_A - vessel_data[j].actual_bycatch_rate_A + bycatch_rate_adj) / count_A;
			mean_adj = mean_A;
			stdev_adj = stdev_A * sqrt(1 + 1.0 / count_A) / sqrt(1 + vessel_data[j].pollock_A / season_pollock_A);
			z_adj = (mean_adj - bycatch_rate_adj) / stdev_adj;
			
			switch(penalty_func)
			{
				case SHALLOW:
					p_adj = shallow_slope(z_adj);
					break;
				case NORMAL:
					p_adj = normal_pvalue(z_adj, z_table);
					break;
				case MODERATE:
					p_adj = moderate_slope(z_adj);
					break;
				case LINEAR:
					p_adj = linear(z_adj);
					break;
			}
			q_adj = EPSILON * p_adj + DELTA;
			credit_factor_adj = ALPHA + BETA * vessel_data[j].credit_factor_A + GAMMA * q_adj;
			
			out << bycatch_adj << ",";
			out << bycatch_rate_adj << ",";
			out << z_adj << ",";
			out << q_adj << ",";
			out << credit_factor_adj << ",";
			out << int(vessel_data[j].pollock_A / season_pollock_A * credit_factor_adj * credits_A) << ",";
			out << int(vessel_data[j].pollock_A / season_pollock_A * credit_factor_adj * credits_A) - int(vessel_data[j].pollock_A / season_pollock_A * new_credit_factor * credits_A) << ",";
			if(vessel_data[j].chinook_A > -DELTA_BYCATCH)
				deltas.push_back(int(vessel_data[j].pollock_A / season_pollock_A * credit_factor_adj * credits_A) - int(vessel_data[j].pollock_A / season_pollock_A * new_credit_factor * credits_A));
			
		}
		else
		{
			out << ",,,,,,,,,,,,,,,";
		}
		
		
		// B season data
		out << vessel_data[j].actual_pollock_B << ",";
		out << vessel_data[j].actual_chinook_B << ",";
		if(vessel_data[j].pollock_B > 0)
		{
			out << vessel_data[j].actual_bycatch_rate_B << ",";
			out << vessel_data[j].z_B << ",";
			out << vessel_data[j].q_B << ",";
			out << vessel_data[j].credit_factor_B << ",";
			new_credit_factor = ALPHA + BETA * vessel_data[j].credit_factor_B + GAMMA * vessel_data[j].q_B;
			out << new_credit_factor << ",";
			out << vessel_data[j].pollock_B / season_pollock_B << ",";
			out << vessel_data[j].init_credits_B << ",";
			out << int(vessel_data[j].pollock_B / season_pollock_B * new_credit_factor * credits_B) << ",";
			
			bycatch_adj = vessel_data[j].actual_chinook_B + DELTA_BYCATCH;
			if(bycatch_adj < 0)
				bycatch_adj = 0;
			bycatch_rate_adj = double(bycatch_adj) / vessel_data[j].actual_pollock_B;
			//mean_adj = (sum_B - vessel_data[j].actual_bycatch_rate_B + bycatch_rate_adj) / count_B;
			mean_adj = mean_B;
			stdev_adj = stdev_B * sqrt(1 + 1.0 / count_B) / sqrt(1 + vessel_data[j].pollock_B / season_pollock_B);
			z_adj = (mean_adj - bycatch_rate_adj) / stdev_adj;
			switch(penalty_func)
			{
				case SHALLOW:
					p_adj = shallow_slope(z_adj);
					break;
				case NORMAL:
					p_adj = normal_pvalue(z_adj, z_table);
					break;
				case MODERATE:
					p_adj = moderate_slope(z_adj);
					break;
				case LINEAR:
					p_adj = linear(z_adj);
					break;
			}
			q_adj = EPSILON * p_adj + DELTA;
			credit_factor_adj = ALPHA + BETA * vessel_data[j].credit_factor_B + GAMMA * q_adj;
			
			out << bycatch_adj << ",";
			out << bycatch_rate_adj << ",";
			out << z_adj << ",";
			out << q_adj << ",";
			out << credit_factor_adj << ",";
			out << int(vessel_data[j].pollock_B / season_pollock_B * credit_factor_adj * credits_B) << ",";
			out << int(vessel_data[j].pollock_B / season_pollock_B * credit_factor_adj * credits_B) - int(vessel_data[j].pollock_B / season_pollock_B * new_credit_factor * credits_B);
			if(vessel_data[j].actual_chinook_B > -DELTA_BYCATCH)
				deltas.push_back(int(vessel_data[j].pollock_B / season_pollock_B * credit_factor_adj * credits_B) - int(vessel_data[j].pollock_B / season_pollock_B * new_credit_factor * credits_B));
			
		}
		else
		{
			out << ",,,,,,,,,,,,,,";
		}
		
		out << "\n";
	}
	
	out << "\n";
	out << "TOTAL,,";
	out << pollock_A << "," << chinook_A << "," << bycatch_rate_A << ",";
	out << ",,,,,";
	out << init_credits_A << ",,";
	out << ",,,,,,,";
	out << pollock_B << "," << chinook_B << "," << bycatch_rate_B << ",";
	out << ",,,,,";
	out << init_credits_B << ",,,,,,";
	out << "\n";
	
	out.close();
	
	sprintf(filename, "credit_deltas.%d.csv", year);
	out.open(filename);
	for(int i = 0; i < deltas.size(); i++)
	{
		out << deltas[i] << "\n";
	}
	out.close();
	return;
}

void simulator::print_unfished_data()
{
	char filename[40];
	sprintf(filename, "unfished_pollock.csv");
	ofstream out;
	out.open(filename);
	
	// header row
	out << "year,";
	out << "unfished pollock (A),";
	out << "unfished pollock (B)";
	out << "\n";
	
	int num_years = years.size();
	for(int i = 0; i < num_years; i++)
	{
		out << years[i] << ",";
		out << unfished_pollock_A[i] << ",";
		out << unfished_pollock_B[i];
		out << "\n";
	}
	out.close();
	return;
}

void simulator::save_vessel_data(vector<vessel> & vessel_data)
{
	return;
}

bool operator <(const needy_struct & a, const needy_struct & b)
{
	return a.bycatch_rate < b.bycatch_rate;
}

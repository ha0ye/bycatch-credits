#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <math.h>
#include "vessel.h"
#include "simulator.h"

using namespace std;

// function prototypes
void read_in_landings(istream & in, vector<landing> & raw_data, 
					  vector<string> & column_names);
void load_z_table(vector<double> & z_table);
void process(vector<landing> & vessel_data, vector<double> & z_table);
void process_year(const vector<landing> & raw_data, const int year, 
				  vector<credit_factor> & credit_factor_DB,
				  vector<double> & z_table);
void convert_data(const vector<landing> & raw_data, const int year, 
				  vector<landing> & year_data, vector<vessel> & vessel_data);
void load_credit_factors(vector<vessel> & vessel_data, 
						 vector<credit_factor> & credit_factor_DB);
void process_data(vector<vessel> & vessel_data, const int year);
void simulate_year(vector<vessel> & vessel_data, const int year, 
				   const vector<landing> & year_data);
void update_credit_factors(vector<vessel> & vessel_data,
						   vector<credit_factor> & credit_factor_DB, 
						   vector<double> & z_table);
void print_credit_data(vector<vessel> & vessel_data, const int year);
void print_vessel_data(vector<vessel> & vessel_data, const int year);

double shallow_slope(const double z_score);
double moderate_slope(const double z_score);
double normal_pvalue(const double z_score, const vector<double> & z_table);

string parse_line(string line_buffer, int& position);
int parse_month(string value_buffer);
int parse_day(string value_buffer);
int day_count(int year, int month, int day);
string day_name(int day_count, int year);

/**/
const double ALPHA = 4.0 / 3.0;
const double BETA = 1.0 / 3.0;
/**/

/*
const double ALPHA = 0.0;
const double BETA = 1.0;
*/

//const double GAMMA = 0.25;
const double GAMMA = 0;

const double PURCHASE_LIMIT = 1.0 / 3.0;
//const double PURCHASE_LIMIT = 1000;
const double DYNAMIC_STRANDING_LIMIT = 0.50;

const double price_A = 0.20;
const double price_B = 0.12;
const double constant_factor = 1.0 / 3.0;
const double legacy_factor = 1.0 / 3.0;
const double rate_factor = 1.0 / 3.0;

int num_days;
int start_date;
double season_pollock_A, season_pollock_B;
map<string, int> vessel_index;

int main(int argc, char** argv)
{
	ifstream datafile;
	datafile.open("cv_sector_data.csv");
	
	/*
	 if (argc > 1)
	 datafile.open(argv[1]);
	 else
	 {
	 cerr << "Usage: pollockDataProcessor datafile\n";
	 return 1;
	 }
	 */
	
	// read in raw landings data
	vector<landing> raw_data;
	vector<string> column_names;
	read_in_landings(datafile, raw_data, column_names);
	datafile.close();
	
	// read in z-table
	vector<double> z_table;
	load_z_table(z_table);
	
	// process data
	process(raw_data, z_table);
	
	return 0;
}

void read_in_landings(istream& datafile, vector<landing> & raw_data, 
					  vector<string> & column_names)
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

void load_z_table(vector<double> & z_table)
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

void process(vector<landing> & raw_data, vector<double> & z_table)
{
	vector<credit_factor> credit_factor_DB;
	credit_factor_DB.clear();
	
	process_year(raw_data, 2000, credit_factor_DB, z_table);
	process_year(raw_data, 2001, credit_factor_DB, z_table);
	process_year(raw_data, 2002, credit_factor_DB, z_table);
	process_year(raw_data, 2003, credit_factor_DB, z_table);
	process_year(raw_data, 2004, credit_factor_DB, z_table);
	process_year(raw_data, 2005, credit_factor_DB, z_table);
	process_year(raw_data, 2006, credit_factor_DB, z_table);
	process_year(raw_data, 2007, credit_factor_DB, z_table);
	
	return;
}

void process_year(const vector<landing> & raw_data, const int year, 
				  vector<credit_factor> & credit_factor_DB,
				  vector<double> & z_table)
{
	// filter out the desired season
	vector<landing> year_data;
	vector<vessel> vessel_data;
	convert_data(raw_data, year, year_data, vessel_data);
	
	// load credit allocation factors
	load_credit_factors(vessel_data, credit_factor_DB);
	
	// process
	process_data(vessel_data, year);
	
	// simulate
	simulate_year(vessel_data, year, year_data);
	
	// update credit allocation factors
	update_credit_factors(vessel_data, credit_factor_DB, z_table);
	
	// print output
	print_credit_data(vessel_data, year);
	
	// print vessel data
	print_vessel_data(vessel_data, year);
	
	return;
}

void convert_data(const vector<landing> & raw_data, const int year, 
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

void load_credit_factors(vector<vessel> & vessel_data, 
						 vector<credit_factor> & credit_factor_DB)
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
		
		vessel_data[i].credit_factor_A = constant_factor + 
		legacy_factor * credit_factor_DB[found].p_A.back() + 
		rate_factor * credit_factor_DB[found].q_A.back();
		vessel_data[i].credit_factor_B = constant_factor + 
		legacy_factor * credit_factor_DB[found].p_B.back() + 
		rate_factor * credit_factor_DB[found].q_B.back();
		vessel_data[i].cim_A = credit_factor_DB[found].cim_A;
		vessel_data[i].cim_B = credit_factor_DB[found].cim_B;
	}
	return;
}

void process_data(vector<vessel> & vessel_data, const int year)
{
	int num_vessels, season_chinook_A, season_chinook_B;
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
	
	// compute credit allocations
	double credit_perc_A = 0;
	double credit_perc_B = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].credit_perc_A = vessel_data[i].pollock_A / season_pollock_A * vessel_data[i].credit_factor_A;
		vessel_data[i].credit_perc_B = vessel_data[i].pollock_B / season_pollock_B * vessel_data[i].credit_factor_B;
		credit_perc_A += vessel_data[i].credit_perc_A;
		credit_perc_B += vessel_data[i].credit_perc_B;
	}
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].credit_perc_A /= credit_perc_A;
		vessel_data[i].credit_perc_B /= credit_perc_B;
	}
	
	// compute total credits for the season
	double credits_A = 68392 * 0.7 * 0.498;
	double credits_B = 68392 * 0.3 * 0.693;
	
	// allocate credits for each vessel
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].init_credits_A = int(vessel_data[i].credit_perc_A * credits_A);
		vessel_data[i].init_credits_B = int(vessel_data[i].credit_perc_B * credits_B);
	}
	return;
}

void simulate_year(vector<vessel> & vessel_data, const int year, 
				   const vector<landing> & year_data)
{
	int num_data = year_data.size();
	int num_vessels = vessel_data.size();
	int day_index;
	int index;
	int start_b_season = day_count(year, 6, 11) - start_date;
	int total_credits;
	
	double bycatch_rate;
	double pollock_remaining;
	int credits_needed;
	int credits_available;
	
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].credits = vessel_data[i].init_credits_A;
		vessel_data[i].actual_pollock = 0;
		vessel_data[i].actual_chinook = 0;
	}
	
	// A season
	credits_available = 0;
	for(int i = 0; i < num_data; i++)
	{
		day_index = day_count(year, year_data[i].month, year_data[i].day) - start_date;
		index = vessel_index[year_data[i].name];
		if(day_index >= start_b_season)
		{
			start_b_season = i;
			break;
		}
		
		if(vessel_data[index].credits > 0) // able to fish
		{
			vessel_data[index].actual_pollock += year_data[i].pollock;
			vessel_data[index].actual_chinook += int(vessel_data[index].cim_A * year_data[i].chinook + 0.5);
			vessel_data[index].credits -= int(vessel_data[index].cim_A * year_data[i].chinook + 0.5);
		}
		
		if(vessel_data[index].credits <= 0) // out of credits
		{
			// zero out credits
			credits_needed = -vessel_data[index].credits;
			if(credits_needed < credits_available)
			{
				for(int j = 0; j < num_vessels; j++) // search for credits
				{
					if(vessel_data[j].pollock_std[day_index] > (vessel_data[j].pollock_A - 0.01))
					{
						if(vessel_data[j].credits >= credits_needed)
						{
							vessel_data[j].credits -= credits_needed;
							vessel_data[index].credits = 0;
							break;
						}
						else
						{
							vessel_data[index].credits += vessel_data[j].credits;
							credits_needed -= vessel_data[j].credits;
							vessel_data[j].credits = 0;
						}
					}
				}
			}
			if(vessel_data[index].credits == 0)
			{
				// compute number of credits needed
				bycatch_rate = double(vessel_data[index].actual_chinook) / 
				vessel_data[index].actual_pollock;
				pollock_remaining = vessel_data[index].pollock_A - 
				vessel_data[index].pollock_std[day_index];
				credits_needed += int(pollock_remaining * bycatch_rate) - vessel_data[index].credits;
				
				if(credits_needed < credits_available) // credits able to be supplied
				{
					for(int j = 0; j < num_vessels; j++) // search for credits
					{
						if(vessel_data[j].pollock_std[day_index] > (vessel_data[j].pollock_A - 0.01))
						{
							if(vessel_data[j].credits >= credits_needed)
							{
								vessel_data[j].credits -= credits_needed;
								vessel_data[index].credits = credits_needed;
								break;
							}
							else
							{
								vessel_data[index].credits += vessel_data[j].credits;
								credits_needed -= vessel_data[j].credits;
								vessel_data[j].credits = 0;
							}
							
						}
					}
				}
			}
		}
		
		if(vessel_data[index].pollock_std[day_index] > (vessel_data[index].pollock_A - 0.01))
		{
			credits_available += vessel_data[index].credits;
		}
	}
	
	// influx of B season credits
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].uncaught_pollock_A = vessel_data[i].pollock_A - vessel_data[i].actual_pollock;
		vessel_data[i].bycatch_rate_A = vessel_data[i].actual_chinook / vessel_data[i].actual_pollock;
		if(vessel_data[i].credits < 0)
			vessel_data[i].credits = 0;
		vessel_data[i].credits += vessel_data[i].init_credits_B;
		vessel_data[i].actual_pollock = 0;
		vessel_data[i].actual_chinook = 0;
	}
	
	double current_pollock = 0;
	int fleet_credits;
	
	for(int i = start_b_season; i < num_data; i++)
	{
		day_index = day_count(year, year_data[i].month, year_data[i].day) - start_date;
		index = vessel_index[year_data[i].name];
		
		current_pollock += year_data[i].pollock;
		fleet_credits = 0;
		for(int j = 0; j < num_vessels; j++)
		{
			fleet_credits += vessel_data[j].credits;
		}
		
		if(vessel_data[index].credits > 0) // able to fish
		{
			vessel_data[index].actual_pollock += year_data[i].pollock;
			vessel_data[index].actual_chinook += int(vessel_data[index].cim_B * year_data[i].chinook + 0.5);
			vessel_data[index].credits -= int(vessel_data[index].cim_B * year_data[i].chinook + 0.5);
		}
		
		if(vessel_data[index].credits <= 0) // out of credits
		{
			// zero out credits
			credits_needed = -vessel_data[index].credits;
			if(credits_needed < credits_available)
			{
				for(int j = 0; j < num_vessels; j++) // search for credits
				{
					if(vessel_data[j].pollock_std[day_index] > (vessel_data[j].pollock_B - 0.01))
					{
						if(vessel_data[j].credits >= credits_needed)
						{
							vessel_data[j].credits -= credits_needed;
							vessel_data[index].credits = 0;
							break;
						}
						else
						{
							vessel_data[index].credits += vessel_data[j].credits;
							credits_needed -= vessel_data[j].credits;
							vessel_data[j].credits = 0;
						}
					}
				}
			}
			if(vessel_data[index].credits == 0)
			{
				// compute number of credits needed
				bycatch_rate = double(vessel_data[index].actual_chinook) / 
				vessel_data[index].actual_pollock;
				pollock_remaining = vessel_data[index].pollock_B - 
				vessel_data[index].pollock_std[day_index];
				credits_needed += int(pollock_remaining * bycatch_rate) - vessel_data[index].credits;
				
				if(credits_needed < credits_available) // credits able to be supplied
				{
					for(int j = 0; j < num_vessels; j++) // search for credits
					{
						if(vessel_data[j].pollock_std[day_index] > (vessel_data[j].pollock_B - 0.01))
						{
							if(vessel_data[j].credits >= credits_needed)
							{
								vessel_data[j].credits -= credits_needed;
								vessel_data[index].credits = credits_needed;
								vessel_data[index].credits_bought_B += credits_needed;
								break;
							}
							else
							{
								vessel_data[index].credits += vessel_data[j].credits;
								vessel_data[index].credits_bought_B += vessel_data[j].credits;
								credits_needed -= vessel_data[j].credits;
								vessel_data[j].credits = 0;
							}
							
						}
					}
				}
			}
		}
		
		if(vessel_data[index].pollock_std[day_index] > (vessel_data[index].pollock_B - 0.01)) // done fishing
		{
			credits_available += vessel_data[index].credits;
		}
	}
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].uncaught_pollock_B = vessel_data[i].pollock_B - vessel_data[i].actual_pollock;
		vessel_data[i].bycatch_rate_B = vessel_data[i].actual_chinook / vessel_data[i].actual_pollock;
	}
	
	return;
}

void update_credit_factors(vector<vessel> & vessel_data,
						   vector<credit_factor> & credit_factor_DB, 
						   vector<double> & z_table)
{
	int num_vessels = vessel_data.size();
	int DB_size = credit_factor_DB.size();
	
	double mean, var, stdev, z_score, p, q;
	double squared_vals, summed_vals;
	int count, found;
	
	squared_vals = 0;
	summed_vals = 0;
	count = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		if(vessel_data[i].pollock_A > 0)
		{
			squared_vals += vessel_data[i].bycatch_rate_A * vessel_data[i].bycatch_rate_A;
			summed_vals += vessel_data[i].bycatch_rate_A;
			count ++;
		}
	}
	mean = summed_vals / count;
	var = squared_vals / count - mean * mean;
	stdev = sqrt(var);
	for(int i = 0; i < num_vessels; i++)
	{
		if(vessel_data[i].pollock_A > 0)
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

			z_score = (mean - vessel_data[i].bycatch_rate_A) / stdev;
			
			// p = shallow_slope(z_score);
			p = moderate_slope(z_score);
			// p = normal_pvalue(z_score, z_table);
			
			q = ALPHA * p + BETA;
			credit_factor_DB[found].p_A.push_back(vessel_data[i].credit_factor_A);
			credit_factor_DB[found].q_A.push_back(q);
			credit_factor_DB[found].cim_A *= (1.0 - GAMMA / (1.0 + q));
			vessel_data[i].z_A = z_score;
			vessel_data[i].q_A = q;
			vessel_data[i].cim_A = credit_factor_DB[found].cim_A;
		}
	}
	
	squared_vals = 0;
	summed_vals = 0;
	count = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		if(vessel_data[i].pollock_B > 0)
		{
			squared_vals += vessel_data[i].bycatch_rate_B * vessel_data[i].bycatch_rate_B;
			summed_vals += vessel_data[i].bycatch_rate_B;
			count ++;
		}
	}
	mean = summed_vals / count;
	var = squared_vals / count - mean * mean;
	stdev = sqrt(var);
	for(int i = 0; i < num_vessels; i++)
	{
		if(vessel_data[i].pollock_B > 0)
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
			z_score = (mean - vessel_data[i].bycatch_rate_B) / stdev;
			
			// p = shallow_slope(z_score);
			p = moderate_slope(z_score);
			// p = normal_pvalue(z_score, z_table);
			
			q = ALPHA * p + BETA;
			credit_factor_DB[found].p_B.push_back(vessel_data[i].credit_factor_B);
			credit_factor_DB[found].q_B.push_back(q);
			credit_factor_DB[found].cim_B *= (1.0 - GAMMA / (1.0 + q));
			vessel_data[i].z_B = z_score;
			vessel_data[i].q_B = q;
			vessel_data[i].cim_B = credit_factor_DB[found].cim_B;
		}
	}
	return;
}

void print_credit_data(vector<vessel> & vessel_data, const int year)
{
	char filename[40];
	sprintf(filename, "credit_supply_demand.%d.csv", year);
	ofstream out;
	out.open(filename);
	
	int start_b_season = day_count(year, 6, 11) - start_date;
	
	// header row
	out << "Date,Credits (for sale),";
	out << "Pollock, Pollock (std),";
	out << "Bycatch, Bycatch (std), Bycatch Rate\n";
	
	int num_vessels = vessel_data.size();
	double pollock, pollock_std;
	int bycatch, bycatch_std;
	int credits, num_limit_vessels;
	
	for(int i = 0; i < num_days; i++)
	{
		if(i == start_b_season)
			out << "\n";
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
		}
		
		//if((pollock > 0) || (i == start_b_season-1))
		{
			out << day_name(i + start_date, year) << ",";
			out << credits << ",";
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

void print_vessel_data(vector<vessel> & vessel_data, const int year)
{
	char filename[40];
	sprintf(filename, "vessel_data.%d.csv", year);
	ofstream out;
	out.open(filename);
	
	int start_b_season = day_count(year, 6, 11) - start_date;
	
	// header row
	out << ",,";
	out << "A Season,,,,,,,";
	out << "B Season,,,,,,,";
	out << year << "\n";
	out << "Vessel Name,Coop,";
	out << "Pollock,Bycatch,Credit_Limited_Bycatch,Uncaught Pollock,Bycatch Rate,Credits,z-score,q-value,";
	out << "Pollock,Bycatch,Credit_Limited_Bycatch,Uncaught Pollock,Bycatch Rate,Credits,z-score,q-value,";
	out << "Pollock,Bycatch,Uncaught Pollock,Bycatch Rate,Credits\n";
	
	int num_vessels = vessel_data.size();
	double credits_needed;
	
	for(int j = 0; j < num_vessels; j++)
	{
		out << vessel_data[j].name << ",";
		out << vessel_data[j].coop << ",";
		
		// A season data
		out << vessel_data[j].pollock_A << ",";
		out << vessel_data[j].chinook_A << ",";
		out << vessel_data[j].init_credits_A * (1.0 + PURCHASE_LIMIT) << ",";
		out << vessel_data[j].uncaught_pollock_A << ",";
		out << vessel_data[j].bycatch_rate_A << ",";
		out << vessel_data[j].init_credits_A << ",";
		if(vessel_data[j].pollock_A > 0)
		{
			out << vessel_data[j].z_A << ",";
			out << vessel_data[j].q_A << ",";
		}
		else
		{
			out << ",,";
		}
		
		// B season data
		out << vessel_data[j].pollock_B << ",";
		out << vessel_data[j].chinook_B << ",";
		out << vessel_data[j].init_credits_B * (1.0 + PURCHASE_LIMIT) << ",";
		out << vessel_data[j].uncaught_pollock_B << ",";
		out << vessel_data[j].bycatch_rate_B << ",";
		out << vessel_data[j].init_credits_B << ",";
		if(vessel_data[j].pollock_B > 0)
		{
			out << vessel_data[j].z_B << ",";
			out << vessel_data[j].q_B << ",";
		}
		else
		{
			out << ",,";
		}
		
		// yearly data
		out << vessel_data[j].pollock_total << ",";
		out << vessel_data[j].chinook_total << ",";
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
		pollock_A += vessel_data[j].pollock_A;
		chinook_A += vessel_data[j].chinook_A;
		uncaught_pollock_A += vessel_data[j].uncaught_pollock_A;
		init_credits_A += vessel_data[j].init_credits_A;
		
		// A season data
		pollock_B += vessel_data[j].pollock_B;
		chinook_B += vessel_data[j].chinook_B;
		uncaught_pollock_B += vessel_data[j].uncaught_pollock_B;
		init_credits_B += vessel_data[j].init_credits_B;
	}
	bycatch_rate_A = chinook_A / pollock_A;
	bycatch_rate_B = chinook_B / pollock_B;
	out << "\n";
	out << "TOTAL,,";
	out << pollock_A << "," << chinook_A << "," << uncaught_pollock_A << ",";
	out << bycatch_rate_A << "," << init_credits_A << ",,,";
	out << pollock_B << "," << chinook_B << "," << uncaught_pollock_B << ",";
	out << bycatch_rate_B << "," << init_credits_B << ",,,";
	out << pollock_A + pollock_B << "," << chinook_A + chinook_B << ",";
	out << (chinook_A + chinook_B) / (pollock_A + pollock_B) << ",";
	out << init_credits_A + init_credits_B << "\n";
	
	out.close();
	return;
}

double shallow_slope(const double z_score)
{
	if(z_score <= -3)
	{
		return 0;
	}
	else if (z_score <= -1)
	{
		return 0.2 * tan (0.3984606 * z_score + -0.1247462) + 0.7810852;
	}
	else if (z_score <= 3)
	{
		return 1.0 / 12.0 * z_score + 0.75;
	}
	return 1;
}

double moderate_slope(const double z_score)
{
	if(z_score <= -2)
	{
		return 0;
	}
	else if (z_score <= -1)
	{
		return 0.2 * tan (0.57006 * z_score + -0.02800249) + 0.4695404;
	}
	else if (z_score <= 3)
	{
		return 1.0 / 6.0 * z_score + 0.5;
	}
	return 1;
}

double normal_pvalue(const double z_score, const vector<double> & z_table)
{
	if(z_score <= -3)
	{
		return 0;
	}
	else if (z_score <= 3)
	{
		int z_index = int(100 * (z_score + 3));
		return z_table[z_index];
	}
	return 1;
}


string parse_line(string line_buffer, int& position)
{
	char* spacers = ",";
	int begin, end;
	
	begin = line_buffer.find_first_not_of(spacers, position);
	if (begin != string::npos)
	{
		end = line_buffer.find_first_of(spacers, position + 1);
		if (end == string::npos) // no spacers found, value goes to end of line
		{
			end = line_buffer.length();
		}
		position = end;
		return line_buffer.substr(begin, end - begin); // return substring
	}
	else
	{
		return "";
	}
}

int parse_month(string value_buffer)
{
	char* spacers = "/";
	int begin, end;
	
	begin = value_buffer.find_first_not_of(spacers, 0);
	if (begin != string::npos)
	{
		end = value_buffer.find_first_of(spacers, 1);
		if (end == string::npos) // no spacers found, value goes to end of line
		{
			end = value_buffer.length();
		}
		return strtod(value_buffer.substr(begin, end - begin).c_str(), NULL); // return substring
	}
	else
	{
		return -1;
	}
}

int parse_day(string value_buffer)
{
	char* spacers = "/";
	int begin, end;
	
	begin = value_buffer.find_first_not_of(spacers, 0);
	if (begin != string::npos)
	{
		end = value_buffer.find_first_of(spacers, 1);
		begin = value_buffer.find_first_not_of(spacers, end);
		end = value_buffer.find_first_of(spacers, begin + 1);
		if (end == string::npos) // no spacers found, value goes to end of line
		{
			end = value_buffer.length();
		}
		return strtod(value_buffer.substr(begin, end - begin).c_str(), NULL); // return substring
	}
	else
	{
		return -1;
	}
}

int day_count(int year, int month, int day)
{
	int dc = day;
	
	if(month >= 2)
		dc += 31;
	if(month >= 3)
	{
		if(year == 2004)
			dc += 1;
		dc += 28;
	}
	if(month >= 4)
		dc += 31;
	if(month >= 5)
		dc += 30;
	if(month >= 6)
		dc += 31;
	if(month >= 7)
		dc += 30;
	if(month >= 8)
		dc += 31;
	if(month >= 9)
		dc += 31;
	if(month >= 10)
		dc += 30;
	if(month >= 11)
		dc += 31;
	if(month >= 12)
		dc += 30;
	return dc;
}

string day_name(int day_count, int year)
{
	string months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	int month_index = 0;
	int feb_days = 28;
	if(year == 2004 || year == 2000) // february
	{
		feb_days = 29;
	}
	if(day_count > 31) // january
	{
		day_count -= 31;
		month_index ++;
		if(day_count > feb_days)
		{
			day_count -= feb_days;
			month_index ++;
			if(day_count > 31) // march
			{
				day_count -= 31;
				month_index ++;
				
				if(day_count > 30) // april
				{
					day_count -= 30;
					month_index ++;
					
					if(day_count > 31) // may
					{
						day_count -= 31;
						month_index ++;
						
						if(day_count > 30) // june
						{
							day_count -= 30;
							month_index ++;
							
							if(day_count > 31) // july
							{
								day_count -= 31;
								month_index ++;
								
								if(day_count > 31) // august
								{
									day_count -= 31;
									month_index ++;
									
									if(day_count > 30) // september
									{
										day_count -= 30;
										month_index ++;
										
										if(day_count > 31) // october
										{
											day_count -= 31;
											month_index ++;
											
											if(day_count > 30) // november
											{
												day_count -= 30;
												month_index ++;
												
												if(day_count > 31)
												{
													return "Dec-31";
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	char date[3];
	char yr[5];
	sprintf(date, "%d", day_count);
	sprintf(yr, "%d", year);
	return months[month_index] + "-" + date + "-" + yr;
}
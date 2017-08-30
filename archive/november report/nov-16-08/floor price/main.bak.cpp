#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

enum seasonEnum
{
	A,
	B
};

struct landing
{
	int year;
	int month;
	int day;
	string ticketNumber;
	string name;
	string coop;
	seasonEnum season;
	double pollock;
	double chinook;
};

struct vessel
{
	string name;
	string coop;
	vector<double> pollock;
	vector<int> chinook;
	vector<int> dates;
	int num_landings;
	
	double pollock_total;
	int chinook_total;
	double bycatch_rate;
	
	int credits;
	int stop_index;
	double uncaught_pollock;
};

struct credit_event
{
	int date;
	double credits_needed_est;
	double credits_needed_act;
};


bool compare_credit_event(credit_event a, credit_event b) { return (a.date < b.date); }

void read_in_landings(istream& in, vector<landing>& raw_data, 
					  vector<string>& column_names);
void process_season(const vector<landing> & raw_data, const int year, 
					const seasonEnum season);
void filter_season(const vector<landing> & raw_data, const int year, 
				   const seasonEnum season, vector<landing> & season_data);
void convert_data(const vector<landing> & season_data, 
				  vector<vessel> & vessel_data);
void process_data(vector<landing> & vessel_data);
void compute_credit_demand(vector<vessel> & vessel_data, const seasonEnum season, 
					vector<credit_event> & credit_demand);
void print_credit_data(const vector<credit_event> & credit_demand, 
					   const vector<landing> & raw_data, const int year, 
					   const seasonEnum season);

string parse_line(string line_buffer, int& position);
int parse_month(string value_buffer);
int parse_day(string value_buffer);

const double STRANDING = 0.1;

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
	
	// process data
	process_data(raw_data);
	
	
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
					if (value_buffer.compare("A") == 0)
						temp_landing.season = A;
					else if (value_buffer.compare("B") == 0)
						temp_landing.season = B;
					else
						cerr << "season unknown, given was: " << value_buffer << "\n";
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

void process_data(vector<landing> & raw_data)
{
	process_season(raw_data, 2000, A);
	process_season(raw_data, 2000, B);
	process_season(raw_data, 2001, A);
	process_season(raw_data, 2001, B);
	process_season(raw_data, 2002, A);
	process_season(raw_data, 2002, B);
	process_season(raw_data, 2003, A);
	process_season(raw_data, 2003, B);
	process_season(raw_data, 2004, A);
	process_season(raw_data, 2004, B);
	process_season(raw_data, 2005, A);
	process_season(raw_data, 2005, B);
	process_season(raw_data, 2006, A);
	process_season(raw_data, 2006, B);
	process_season(raw_data, 2007, A);
	process_season(raw_data, 2007, B);

	return;
}

void process_season(const vector<landing> & raw_data, const int year, 
					const seasonEnum season)
{
	// filter out the desired season
	vector<landing> season_data;
	filter_season(raw_data, year, season, season_data);
	
	// process data to be vessel-specific
	vector<vessel> vessel_data;
	convert_data(season_data, vessel_data);
	
	// compute credit demand
	vector<credit_event> credit_demand;
	compute_credit_demand(vessel_data, season, credit_demand);
	
	// print output
	print_credit_data(credit_demand, season_data, year, season);
	
	return;
}

void filter_season(const vector<landing> & raw_data, const int year, 
				   const seasonEnum season, vector<landing> & season_data)
{
	int num_data = raw_data.size();
	int start_index = -1;
	int end_index = -1;
	for(int i = 0; i < num_data; i++)
	{
		if(raw_data[i].year == year)
		{
			if((season == A) && ((raw_data[i].month * 100 + raw_data[i].day) <= 610))
			{
				if(start_index < 0)
					start_index = i;
				end_index = i;
			}
			if((season == B) && ((raw_data[i].month * 100 + raw_data[i].day) >= 611))
			{
				if(start_index < 0)
					start_index = i;
				end_index = i;
			}
		}
	}
	
	season_data.assign(raw_data.begin() + start_index, raw_data.begin() + end_index + 1);
	
	return;
}

void convert_data(const vector<landing> & season_data, 
				  vector<vessel> & vessel_data)
{
	vessel_data.clear();
	
	vessel new_vessel;
	bool added;
	int num_data = season_data.size();
	
	for(int i = 0; i < num_data; i++)
	{
		added = false;
		for(int j = 0; j < vessel_data.size(); j++)
		{
			if ((vessel_data[j].name.compare(season_data[i].name) == 0) && 
				(vessel_data[j].coop.compare(season_data[i].coop) == 0))
			{
				vessel_data[j].pollock.push_back(season_data[i].pollock);
				vessel_data[j].chinook.push_back(int(season_data[i].chinook+0.5));
				vessel_data[j].dates.push_back(season_data[i].year*10000 + season_data[i].month*100 + season_data[i].day);
				vessel_data[j].num_landings ++;
				
				added = true;
				break;
			}
		}
		if(!added)
		{
			new_vessel.pollock.clear();
			new_vessel.chinook.clear();
			new_vessel.dates.clear();
			new_vessel.name = season_data[i].name;
			new_vessel.coop = season_data[i].coop;			
			
			new_vessel.pollock.push_back(season_data[i].pollock);
			new_vessel.chinook.push_back(int(season_data[i].chinook+0.5));
			new_vessel.dates.push_back(season_data[i].year*10000 + season_data[i].month*100 + season_data[i].day);
			new_vessel.num_landings = 1;
			
			vessel_data.push_back(new_vessel);
		}
	}
	return;
}

void compute_credit_demand(vector<vessel> & vessel_data, const seasonEnum season, 
					vector<credit_event> & credit_demand)
{
	int num_vessels, num_data, season_pollock;
	num_vessels = vessel_data.size();
	
	// compute totals for each vessel for the season
	season_pollock = 0;
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].pollock_total = 0;
		vessel_data[i].chinook_total = 0;
		
		num_data = vessel_data[i].num_landings;
		for(int j = 0; j < num_data; j++)
		{
			vessel_data[i].pollock_total += vessel_data[i].pollock[j];
			vessel_data[i].chinook_total += vessel_data[i].chinook[j];
		}
		season_pollock += vessel_data[i].pollock_total;
		
		vessel_data[i].bycatch_rate = vessel_data[i].chinook_total / vessel_data[i].pollock_total;
	}
	
	// compute total credits for the season
	double total_credits;
	if(season == A)
	{
		total_credits = 68392 * 0.7 * 0.498;
	}
	else if(season == B)
	{
		total_credits = 68392 * 0.3 * 0.693;
	}
	
	// allocate credits for each vessel
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].credits = vessel_data[i].pollock_total / season_pollock * total_credits;
	}
	
	int chinook_std;
	double pollock_std;
	credit_event temp_credit_event;
	int a;
	
	credit_demand.clear();
	// see if vessel runs out of credits
	for(int i = 0; i < num_vessels; i++)
	{
		vessel_data[i].stop_index = 9999;
		if(vessel_data[i].credits < vessel_data[i].chinook_total) // ran out of credits
		{
			chinook_std = 0;
			pollock_std = 0;
			num_data = vessel_data[i].num_landings;
			for(int j = 0; j < num_data; j++)
			{
				chinook_std += vessel_data[i].chinook[j];
				pollock_std += vessel_data[i].pollock[j];
				if(chinook_std >= vessel_data[i].credits)
				{
					temp_credit_event.credits_needed_est = chinook_std - vessel_data[i].credits + 
						(vessel_data[i].pollock_total - pollock_std) * (chinook_std / pollock_std);
					
					temp_credit_event.credits_needed_act = vessel_data[i].chinook_total - vessel_data[i].credits;
					
					temp_credit_event.date = (vessel_data[i].dates[j]);
					
					credit_demand.push_back(temp_credit_event);
					break;
				}
			}
		}
	}
	
	// sort by date to construct cumulative demand graph
	sort(credit_demand.begin(), credit_demand.end(), compare_credit_event);
	return;
}

void print_credit_data(const vector<credit_event> & credit_demand, 
					   const vector<landing> & season_data, const int year, 
					   const seasonEnum season)
{
	char filename[40];
	
	if(season == A)
		sprintf(filename, "credit_supply_demand.%d.A.csv", year);
	else
		sprintf(filename, "credit_supply_demand.%d.B.csv", year);
	
	ofstream out;
	out.open(filename);
	
	// header row
	out << "Date,Pollock Fished,Bycatch,Credits Available,Credits Needed (Estimated),Credits Needed(Actual)\n";
	
	int date_index;
	double credits_needed_est_std;
	double credits_needed_act_std;
	
	date_index = 0;
	credits_needed_est_std = 0;
	credits_needed_act_std = 0;	
	
	int working_date;
	double pollock;
	int bycatch;
	int credits;
	if(season == A)
	{
		credits = 68392 * 0.7 * 0.498;
	}
	else if(season == B)
	{
		credits = 68392 * 0.3 * 0.693;
	}
	
	// initial values
	working_date = season_data[0].year * 10000 + season_data[0].month * 100 + season_data[0].day;
	pollock = season_data[0].pollock;
	bycatch = int(season_data[0].chinook + 0.5);
	credits -= int(season_data[0].chinook + 0.5);
	if(credit_demand.size() > 0)
	{
		while(working_date >= credit_demand[date_index].date)
		{
			credits_needed_est_std += credit_demand[date_index].credits_needed_est;
			credits_needed_act_std += credit_demand[date_index].credits_needed_act;
			date_index++;
		}
	}
	int num_data = season_data.size();
	int current_date;
	
	for(int i = 1; i < num_data; i++)
	{
		current_date = season_data[i].year * 10000 + season_data[i].month * 100 + season_data[i].day;
		
		if(current_date != working_date)
		{
			out << working_date << ",";
			out << pollock << ",";
			out << bycatch << ",";
			out << credits << ",";
			out << credits_needed_est_std << ",";
			out << credits_needed_act_std << "\n";
			
			// update credits_needed variables
			if(credit_demand.size() > 0)
			{
				while((current_date >= credit_demand[date_index].date) && 
					  (date_index < credit_demand.size()))
				{
					credits_needed_est_std += credit_demand[date_index].credits_needed_est;
					credits_needed_act_std += credit_demand[date_index].credits_needed_act;
					date_index++;
				}
			}
			
			working_date = current_date;
			pollock = 0;
			bycatch = 0;
		}
		
		pollock += season_data[i].pollock;
		bycatch += int(season_data[i].chinook + 0.5);
		credits -= int(season_data[i].chinook + 0.5);
		if(credits < 0)
			credits = 0;
		
	}
	
	out.close();
	return;
}

/*
 void process(const vector<landing> & raw_data, const int year, const char* outfile_name)
 {
 bool date_setup = false;
 
 int temp_daycount;
 weeks.clear();
 
 int num_data = raw_data.size();
 for(int i = 0; i < num_data; i++)
 {
 if(raw_data[i].year == year)
 {
 if (!date_setup)
 {
 temp_daycount = day_count(year, raw_data[i].month, raw_data[i].day);
 weeks.push_back(temp_daycount);
 while(temp_daycount < 365)
 {
 temp_daycount += 7;
 weeks.push_back(temp_daycount);
 }
 num_weeks = weeks.size();
 date_setup = true;
 }
 add_entry(vessel_data, raw_data[i]);
 }
 else if (raw_data[i].year > year)
 {
 break;
 }
 }
 
 int num_vessels = vessel_data.size();
 
 vector<bool> active_weeks;
 active_weeks.assign(num_weeks, false);
 for(int i = 0; i < num_weeks; i++)
 {
 for(int j = 0; j < num_vessels; j++)
 {
 if(vessel_data[j].pollock[i] > 0)
 {
 active_weeks[i] = true;
 break;
 }
 }
 }
 
 ofstream out;
 out.open(outfile_name);
 if(!out.good())
 {
 cerr << "could not use " << outfile_name << " for output";
 return;
 }
 
 int label_columns = 2;
 
 // output dates row
 for(int i = 0; i < label_columns; i++)
 {
 out << ",";
 }
 for(int i = 0; i < num_weeks; i++)
 {
 if(active_weeks[i])
 {
 out << day_name(weeks[i], year) << ",,,,";
 // out << ",,";
 }
 }
 out << "YR\n";
 
 // output header row
 out << "Vessel,Coop,";
 for(int i = 0; i < num_weeks; i++)
 {
 if(active_weeks[i])
 {
 out << "Pollock,Bycatch,BycatchSTD,CreditsSTD,";
 // out << ",,";
 }
 }
 out << "Pollock_A,%Pollock_A,Allocation_A,%Credits_A,Credits_A,Bycatch_A,Balance_A,Stop Date,Lost Pollock,";
 out << "Pollock_B,%Pollock_B,Allocation_B,%Credits_B,Credits_B,Bycatch_B,Balance_B,Stop Date,Lost Pollock,";
 out << "Pollock_YR,Bycatch_YR,Credits_YR\n";
 
 
 for(int j = 0; j < num_weeks; j++)
 {
 if(weeks[j] > 160)
 {
 season_end_index = j;
 break;
 }
 }
 
 do_calculations(vessel_data);
 
 // output vessel data
 for(int i = 0; i < num_vessels; i++)
 {
 out << vessel_data[i].name << ",";
 out << vessel_data[i].coop << ",";
 for(int j = 0; j < num_weeks; j++)
 {
 if(active_weeks[j])
 {
 out << vessel_data[i].pollock[j] << ",";
 out << vessel_data[i].chinook[j] << ",";
 out << vessel_data[i].chinook_std[j] << ",";
 out << vessel_data[i].credits_std[j] << ",";
 //out << ",,";
 }
 }
 out << vessel_data[i].pollock_a << ",";
 out << vessel_data[i].pollock_a / pollock_a * 100 << "%,";
 out << vessel_data[i].allocation_a << ",";
 out << vessel_data[i].credits_frac_a * 100 << "%,";
 out << vessel_data[i].credits_a << ",";
 out << vessel_data[i].chinook_a << ",";
 out << vessel_data[i].credits_a - vessel_data[i].chinook_a << ",";
 if(vessel_data[i].stop_a > num_weeks)
 out << "NA,";
 else
 out << day_name(weeks[vessel_data[i].stop_a], year) << ",";
 out << vessel_data[i].lost_a << ",";
 
 out << vessel_data[i].pollock_b << ",";
 out << vessel_data[i].pollock_b / pollock_b * 100 << "%,";
 out << vessel_data[i].allocation_b << ",";
 out << vessel_data[i].credits_frac_b * 100 << "%,";
 out << vessel_data[i].credits_b << ",";
 out << vessel_data[i].chinook_b << ",";
 out << vessel_data[i].credits_b - vessel_data[i].chinook_b << ",";
 if(vessel_data[i].stop_b > num_weeks)
 out << "NA,";
 else
 out << day_name(weeks[vessel_data[i].stop_b], year) << ",";
 out << vessel_data[i].lost_b << ",";
 
 out << vessel_data[i].pollock_a + vessel_data[i].pollock_b << ",";
 out << vessel_data[i].chinook_a + vessel_data[i].chinook_b << ",";
 out << vessel_data[i].credits_a + vessel_data[i].credits_b;
 out << "\n";
 }
 
 double pollock_total;
 int chinook_total;
 int chinook_std;
 int credits_std;
 double credits_needed;
 
 // output total for sector
 out << "\n";
 out << "TOTAL" << ",";
 out << ",";
 for(int j = 0; j < num_weeks; j++)
 {
 if(active_weeks[j])
 {
 pollock_total = 0;
 chinook_total = 0;
 chinook_std = 0;
 credits_std = 0;
 for(int i = 0; i < num_vessels; i++)
 {
 pollock_total += vessel_data[i].pollock[j];
 chinook_total += vessel_data[i].chinook[j];
 chinook_std += vessel_data[i].chinook_std[j];
 credits_std += vessel_data[i].credits_std[j];
 }
 out << pollock_total << ",";
 out << chinook_total << ",";
 out << chinook_std << ",";
 out << credits_std << ",";
 //out << ",,";
 }
 }
 
 out << pollock_a << ",100.00%," << "," << credits_frac_a * 100 << "%,";
 out << actual_credits_a << "," << chinook_a << ",";
 out << ",," << lost_a << ",";
 out << pollock_b << ",100.00%," << "," << credits_frac_b * 100 << "%,";
 out << actual_credits_b << "," << chinook_b << ",";
 out << ",," << lost_b << ",";
 out << pollock_a + pollock_b << "," << chinook_a + chinook_b;
 out << "\n";
 
 out << "\n";
 out << ",,";
 for(int i = 0; i < num_weeks; i++)
 {
 if(active_weeks[i])
 out << day_name(weeks[i], year) << ",";
 }
 out << "\n";
 
 // output credit supply
 out << "Credits Available,,";
 for(int j = 0; j < num_weeks; j++)
 {
 if(active_weeks[j])
 {
 credits_std = 0;
 for(int i = 0; i < num_vessels; i++)
 {
 credits_std += vessel_data[i].credits_std[j];
 }
 out << credits_std << ",";
 }
 }
 out << "\n";
 
 // output credit demand
 out << "Credits Needed,,";
 for(int j = 0; j < num_weeks; j++)
 {
 if(active_weeks[j])
 {
 credits_needed = 0;
 for(int i = 0; i < num_vessels; i++)
 {
 if (j >= vessel_data[i].stop_a && j < season_end_index)
 credits_needed += vessel_data[i].credits_needed_a;
 if (j >= vessel_data[i].stop_b)
 credits_needed += vessel_data[i].credits_needed_b;
 }
 out << credits_needed << ",";
 }
 }
 out << "\n";
 
 out.close();
 
 return;
 }
 */

/*
 void do_calculations(vector<vessel>& vessel_data)
 {
 pollock_a = 0;
 chinook_a = 0;
 pollock_b = 0;
 chinook_b = 0;
 credits_frac_a = 0;
 credits_frac_b = 0;
 credits_a = 0.498 * 0.70 * 68392; // from C-2 salmon bycatch motion
 credits_b = 0.693 * 0.30 * 68392; // from C-2 salmon bycatch motion
 actual_credits_a = 0;
 actual_credits_b = 0;
 lost_a = 0;
 lost_b = 0;
 
 int num_vessels = vessel_data.size();
 
 // count total numbers and season to date bycatch
 for(int i = 0; i < num_vessels; i++)
 {
 pollock_a += vessel_data[i].pollock_a;
 chinook_a += vessel_data[i].chinook_a;
 pollock_b += vessel_data[i].pollock_b;
 chinook_b += vessel_data[i].chinook_b;
 vessel_data[i].chinook_std[0] = vessel_data[i].chinook[0];
 vessel_data[i].pollock_std[0] = vessel_data[i].pollock[0];
 for(int j = 1; j < season_end_index; j++)
 {
 vessel_data[i].chinook_std[j] = vessel_data[i].chinook_std[j-1] + vessel_data[i].chinook[j];
 vessel_data[i].pollock_std[j] = vessel_data[i].pollock_std[j-1] + vessel_data[i].pollock[j];
 
 }
 for(int j = season_end_index + 1; j < num_weeks; j++)
 {
 vessel_data[i].chinook_std[j] = vessel_data[i].chinook_std[j-1] + vessel_data[i].chinook[j];
 vessel_data[i].pollock_std[j] = vessel_data[i].pollock_std[j-1] + vessel_data[i].pollock[j];
 }
 }
 
 // count credit fractional allocation
 for(int i = 0; i < num_vessels; i++)
 {
 vessel_data[i].credits_frac_a = vessel_data[i].pollock_a / pollock_a * vessel_data[i].allocation_a;
 credits_frac_a += vessel_data[i].credits_frac_a;
 
 vessel_data[i].credits_frac_b = vessel_data[i].pollock_b / pollock_b * vessel_data[i].allocation_b;
 credits_frac_b += vessel_data[i].credits_frac_b;
 }
 
 // count credits given
 for(int i = 0; i < num_vessels; i++)
 {
 vessel_data[i].credits_a =  int(vessel_data[i].credits_frac_a / credits_frac_a * credits_a);
 actual_credits_a += vessel_data[i].credits_a;
 
 vessel_data[i].credits_b =  int(vessel_data[i].credits_frac_b / credits_frac_b * credits_b);
 actual_credits_b += vessel_data[i].credits_b;
 }
 
 // count credits available
 for(int i = 0; i < num_vessels; i++)
 {
 vessel_data[i].credits_std[0] = vessel_data[i].credits_a - vessel_data[i].chinook[0];
 for(int j = 1; j < season_end_index; j++)
 {
 vessel_data[i].credits_std[j] = vessel_data[i].credits_std[j-1] - vessel_data[i].chinook[j];
 }
 vessel_data[i].credits_std[season_end_index] = vessel_data[i].credits_b - vessel_data[i].chinook[season_end_index];
 if(vessel_data[i].credits_std[season_end_index-1] > 0)
 vessel_data[i].credits_std[season_end_index] += vessel_data[i].credits_std[season_end_index-1];
 for(int j = season_end_index+1; j < num_weeks; j++)
 {
 vessel_data[i].credits_std[j] = vessel_data[i].credits_std[j-1] - vessel_data[i].chinook[j];
 }
 
 for(int j = 0; j < num_weeks; j++)
 {
 if(vessel_data[i].credits_std[j] < 0)
 vessel_data[i].credits_std[j] = 0;
 }
 }
 
 // compute loss due to stranding
 for(int i = 0; i < num_vessels; i++)
 {
 if(vessel_data[i].pollock_a != 0 && vessel_data[i].pollock_b == 0) // vessel fishes in A season only
 {
 for(int j = 0; j < season_end_index; j++)
 {
 if (vessel_data[i].pollock_std[j] == vessel_data[i].pollock_a)
 {
 vessel_data[i].credits_std[j] = (1 - STRANDING) * vessel_data[i].credits_std[j];
 }
 }
 for(int j = season_end_index; j < num_weeks; j++)
 {
 vessel_data[i].credits_std[j] = (1 - STRANDING) * vessel_data[i].credits_std[j];
 }
 }
 else // vessel fishes in B season
 {
 for(int j = season_end_index; j < num_weeks; j++)
 {
 if (vessel_data[i].pollock_std[j] == vessel_data[i].pollock_b)
 {
 vessel_data[i].credits_std[j] = (1 - STRANDING) * vessel_data[i].credits_std[j];
 }
 }
 }
 }
 
 // compute when vessels run out of credits
 for(int i = 0; i < num_vessels; i++)
 {
 if (vessel_data[i].credits_a != 0)
 {
 for(int j = 0; j < season_end_index; j++)
 {
 if (vessel_data[i].credits_std[j] <= 0)
 {
 vessel_data[i].stop_a = j;
 break;
 }
 }
 }
 if (vessel_data[i].credits_b != 0)	
 {
 for(int j = season_end_index; j < num_weeks; j++)
 {
 if (vessel_data[i].credits_std[j] <= 0)
 {
 vessel_data[i].stop_b = j;
 break;
 }
 }
 }
 }
 
 // compute amount of lost pollock
 for(int i = 0; i < num_vessels; i++)
 {
 for(int j = vessel_data[i].stop_a+1; j < season_end_index; j++)
 {
 vessel_data[i].lost_a += vessel_data[i].pollock[j];
 }
 for(int j = vessel_data[i].stop_b+1; j < num_weeks; j++)
 {
 vessel_data[i].lost_b += vessel_data[i].pollock[j];
 }
 lost_a += vessel_data[i].lost_a;
 lost_b += vessel_data[i].lost_b;
 }
 
 
 int stop_a;
 int stop_b;
 double bycatch_rate;
 // compute amount of needed credits to fish lost pollock
 for(int i = 0; i < num_vessels; i++)
 {
 stop_a = vessel_data[i].stop_a;
 stop_b = vessel_data[i].stop_b;
 if(stop_a < num_weeks)
 {
 bycatch_rate = vessel_data[i].chinook_std[stop_a] / vessel_data[i].pollock_std[stop_a];
 vessel_data[i].credits_needed_a = bycatch_rate * (vessel_data[i].lost_a);
 }
 if(stop_b < num_weeks)
 {
 bycatch_rate = vessel_data[i].chinook_std[stop_b] / vessel_data[i].pollock_std[stop_b];
 vessel_data[i].credits_needed_b = bycatch_rate * (vessel_data[i].lost_b);
 }
 }
 return;
 }
 */

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

/*
 int week_index(int day_count)
 {
 for(int i = 1; i < num_weeks; i++)
 {
 if(day_count < weeks[i])
 return i-1;
 }
 }
 
 string day_name(int day_count, int year)
 {
 string months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
 int month_index = 0;
 if(day_count > 31) // january
 {
 day_count -= 31;
 month_index ++;
 }
 if(year == 2004) // february
 {
 if(day_count > 29)
 {
 day_count -= 29;
 month_index ++;
 }
 }
 else
 {
 if(day_count > 28)
 {
 day_count -= 28;
 month_index ++;
 }
 }
 if(day_count > 31) // march
 {
 day_count -= 31;
 month_index ++;
 }
 if(day_count > 30) // april
 {
 day_count -= 30;
 month_index ++;
 }
 if(day_count > 31) // may
 {
 day_count -= 31;
 month_index ++;
 }
 if(day_count > 30) // june
 {
 day_count -= 30;
 month_index ++;
 }
 if(day_count > 31) // july
 {
 day_count -= 31;
 month_index ++;
 }
 if(day_count > 31) // august
 {
 day_count -= 31;
 month_index ++;
 }
 if(day_count > 30) // september
 {
 day_count -= 30;
 month_index ++;
 }
 if(day_count > 31) // october
 {
 day_count -= 31;
 month_index ++;
 }
 if(day_count > 30) // november
 {
 day_count -= 30;
 month_index ++;
 }
 if(day_count > 31)
 {
 return "Dec-31";
 }
 
 char date[3];
 char yr[5];
 sprintf(date, "%d", day_count);
 sprintf(yr, "%d", year);
 return months[month_index] + "-" + date + "-" + yr;
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
 */

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

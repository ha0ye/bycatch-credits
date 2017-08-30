#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>

using namespace std;

// data structures
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
	double pollock;
	int chinook;
};

// function prototypes
void read_in_landings(istream & in, vector<landing> & raw_data, 
					  vector<string> & column_names);
void process(vector<landing> & raw_data);
void process_year(const vector<landing> & raw_data, const int year,
				  vector<double> & rates, vector<int> & days, vector<double> & worst_rates);
void filter_data(const vector<landing> & raw_data, const int year, 
				 vector<landing> & year_data);
void process_data(const vector<landing> & year_data, const int year, 
				  vector<double> & rates, vector<int> & days, vector<double> & worst_rates);
void add_entry(vector<vessel> & vessel_db, landing data);
double worst_rate(vector<vessel> & vessel_db);

string parse_line(string line_buffer, int& position);
int parse_month(string value_buffer);
int parse_day(string value_buffer);
int day_count(int year, int month, int day);
string day_name(int day_count, int year);

// global variables

const int floor_price_chinook = 5000;

double value_A = 0.20;
double value_B = 0.12;

int main(int argc, char** argv)
{
	ifstream datafile;
	datafile.open("cv_sector_data.csv");
	
	// read in raw landings data
	vector<landing> raw_data;
	vector<string> column_names;
	read_in_landings(datafile, raw_data, column_names);
	datafile.close();
	
	// process data
	process(raw_data);
	
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

void process(vector<landing> & raw_data)
{
	vector<double> rates;
	vector<double> worst_rates;
	vector<int> days;
	
	rates.clear();
	worst_rates.clear();
	days.clear();
	
	process_year(raw_data, 2000, rates, days, worst_rates);
	process_year(raw_data, 2001, rates, days, worst_rates);
	process_year(raw_data, 2002, rates, days, worst_rates);
	process_year(raw_data, 2003, rates, days, worst_rates);
	process_year(raw_data, 2004, rates, days, worst_rates);
	process_year(raw_data, 2005, rates, days, worst_rates);
	process_year(raw_data, 2006, rates, days, worst_rates);
	process_year(raw_data, 2007, rates, days, worst_rates);
	
	ofstream out;
	out.open("rates.csv");
	
	int num_rates = rates.size();
	int year = 2000;
	double value;
	
	out << ",";
	out << "A season,,,,,";
	out << ",";
	out << "B season\n";
	out << "year,";
	out << "date,sector rate,sector implied value,worst vessel rate,worst vessel implied value,";
	out << ",";
	out << "date,sector rate,sector implied value,worst vessel rate,worst vessel implied value\n";
	
	for(int i = 0; i < num_rates; i++)
	{
		if(i%2 == 0)
		{
			out << year << ",";
		}
		
		if(rates[i] > 0)
		{
			out << day_name(days[i], year) << ",";
			out << rates[i] << ",";

			if(i%2 == 0)
			{
				value = 2204.6 * value_A / rates[i];
			}
			else
			{
				value = 2204.6 * value_B / rates[i];
			}
			out << value << ",";
			
			out << worst_rates[i] << ",";
			
			if(i%2 == 0)
			{
				value = 2204.6 * value_A / worst_rates[i];
			}
			else
			{
				value = 2204.6 * value_B / worst_rates[i];
			}
			out << value << ",";
		}
		else
		{
			out << "NA,NA,inf,NA,inf";
		}
		
		if(i%2 == 0)
		{
			out << ",";
		}
		else
		{
			out << "\n";
		}
		if(i%2 == 1)
		{
			year++;
		}
	}
	out.close();
	
	return;
}

void process_year(const vector<landing> & raw_data, const int year,
				  vector<double> & rates, vector<int> & days, vector<double> & worst_rates)
{
	// filter out the desired season
	vector<landing> year_data;
	filter_data(raw_data, year, year_data);
	
	// process
	process_data(year_data, year, rates, days, worst_rates);
	
	return;
}

void filter_data(const vector<landing> & raw_data, const int year, 
				 vector<landing> & year_data)
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
	
	return;
}

void process_data(const vector<landing> & year_data, const int year, 
				  vector<double> & rates, vector<int> & days, vector<double> & worst_rates)
{
	vector<vessel> vessel_db;
	int num_data = year_data.size();
	
	double pollock_std;
	int chinook_std;
	int day;
	int b_season_start = day_count(year, 6, 11);
	
	// A season
	pollock_std = 0;
	chinook_std = 0;
	vessel_db.clear();
	for(int i = 0; i < num_data; i++)
	{
		day = day_count(year, year_data[i].month, year_data[i].day);
		if(day >= b_season_start)
			break;
		pollock_std += year_data[i].pollock;
		chinook_std += int(year_data[i].chinook + 0.5);
		add_entry(vessel_db, year_data[i]);
		if(chinook_std >= floor_price_chinook)
		{
			break;
		}
	}
	if(chinook_std >= floor_price_chinook)
	{
		rates.push_back(double(chinook_std) / pollock_std);
		days.push_back(day);
		worst_rates.push_back(worst_rate(vessel_db));
	}
	else
	{
		rates.push_back(0.0);
		days.push_back(-1);
		worst_rates.push_back(0.0);
	}
	
	// B season
	pollock_std = 0;
	chinook_std = 0;
	vessel_db.clear();
	for(int i = 0; i < num_data; i++)
	{
		day = day_count(year, year_data[i].month, year_data[i].day);
		while(day < b_season_start)
		{
			i++;
			day = day_count(year, year_data[i].month, year_data[i].day);
		}
		pollock_std += year_data[i].pollock;
		chinook_std += int(year_data[i].chinook + 0.5);
		add_entry(vessel_db, year_data[i]);
		if(chinook_std >= floor_price_chinook)
		{
			break;
		}
	}
	if(chinook_std >= floor_price_chinook)
	{
		rates.push_back(double(chinook_std) / pollock_std);
		days.push_back(day);
		worst_rates.push_back(worst_rate(vessel_db));
	}
	else
	{
		rates.push_back(0.0);
		days.push_back(-1);
		worst_rates.push_back(0.0);
	}
	return;
}

void add_entry(vector<vessel> & vessel_db, landing data)
{
	int num_vessels = vessel_db.size();
	for(int i = 0; i < num_vessels; i++)
	{
		if((vessel_db[i].name.compare(data.name) == 0) &&
		   (vessel_db[i].coop.compare(data.coop) == 0))
		{
			vessel_db[i].pollock += data.pollock;
			vessel_db[i].chinook += data.chinook;
			return;
		}
	}
	
	// not found
	vessel new_vessel;
	new_vessel.name = data.name;
	new_vessel.coop = data.coop;
	new_vessel.pollock = data.pollock;
	new_vessel.chinook = data.chinook;
	vessel_db.push_back(new_vessel);
	
	return;
}

double worst_rate(vector<vessel> & vessel_db)
{
	int num_vessels = vessel_db.size();
	if(num_vessels == 0)
		return 0.0;
	
	double worst_rate = vessel_db[0].chinook / vessel_db[0].pollock;
	double rate;
	for(int i = 1; i < num_vessels; i++)
	{
		rate = vessel_db[i].chinook / vessel_db[i].pollock;
		if(rate > worst_rate)
			worst_rate = rate;
	}
	return worst_rate;
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
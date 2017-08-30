/*
 *  simulator_tools.cpp
 *  processor
 *
 *  Created by Hao Ye on 12/2/08.
 *  Copyright 2008. All rights reserved.
 *
 */

#include "simulator_tools.h"

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

double linear(const double z_score)
{
	if(z_score <= -2)
	{
		return 0;
	}
	else if(z_score >= 2)
	{
		return 1;
	}
	else
	{
		return (z_score + 2) / 4;
	}
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
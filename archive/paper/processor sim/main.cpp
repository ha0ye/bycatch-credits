
#include <fstream>

#include "vessel.h"
#include "simulator.h"

using namespace std;

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
	
	simulator my_simulator;
	
	// read in raw landings data
	vector<landing> raw_data;
	vector<string> column_names;
	my_simulator.read_in_landings(datafile);
	datafile.close();
	
	// process data
	my_simulator.process();
	
	return 0;
}
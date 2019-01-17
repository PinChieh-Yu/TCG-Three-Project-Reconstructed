#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <chrono>
#include "board.h"
#include "agent.h"
using namespace std;

void InitNewEpisode(size_t& step, board& current, string& movement, uint32_t& hint);
time_t millisec();

struct episode {
	time_t start, end;
	vector<string> actions;
};

int main(int argc, const char* argv[]) {
	cout << "Three-Demo: ";
	copy(argv, argv + argc, ostream_iterator<const char*>(cout, " "));
	cout << endl << endl;

	size_t total = 1000, block = 0, limit = 0;
	string play_args, evil_args;
	string load, save;
	bool summary = false;
	for (int i = 1; i < argc; i++) {
		string para(argv[i]);
		if (para.find("--total=") == 0) {
			total = stoull(para.substr(para.find("=") + 1));
			cout << "TOTAL:" << total << endl;
		} else if (para.find("--block=") == 0) {
			block = stoull(para.substr(para.find("=") + 1));
			cout << "BLOCK:" << block << endl;
		} else if (para.find("--limit=") == 0) {
			limit = stoull(para.substr(para.find("=") + 1));
			cout << "LIMIT:" << limit << endl;
		} else if (para.find("--play=") == 0) {
			play_args = para.substr(para.find("=") + 1);
			cout << "PLAY:" << play_args << endl;
		} else if (para.find("--evil=") == 0) {
			evil_args = para.substr(para.find("=") + 1);
		} else if (para.find("--load=") == 0) {
			load = para.substr(para.find("=") + 1);
			cout << "LOAD:" << load << endl;
		} else if (para.find("--save=") == 0) {
			save = para.substr(para.find("=") + 1);
			cout << "SAVE:" << save << endl;
		} else if (para.find("--summary") == 0) {
			summary = true;
			cout << "SUMMARY:" << summary << endl;
		}
	}

	srand(time(NULL));

	player play(play_args);
	environment envi;

	size_t step;
	board current;
	string movement;
	uint32_t hint;
	vector<episode> record;

	for (size_t turn = 1; turn <= total; turn++) {
		episode epi;
		InitNewEpisode(step, current, movement, hint);
		epi.start = millisec();
		while(true){
			step++;
			if (max(step, size_t(9)) % 2) {
				envi.take_action(current, movement, hint);
			} else {
				play.take_action(current, movement, hint);
			}
			
			if (movement == "~") {
				break;
			} else {
				epi.actions.push_back(movement);
			}
		}
		epi.end = millisec();
		play.backward_train();
		envi.reset_bag();
		record.push_back(epi);
	}
	cout << current << endl;

	if (save.size()) {
		ofstream out(save, ios::out | ios::trunc);
		for (unsigned i = 0; i < record.size(); i++) {
			out << "player:enviro@" << dec << record[i].start << "|";
			for (vector<string>::iterator ite = record[i].actions.begin(); ite != record[i].actions.end(); ite++){
				out << *ite;
			}
			out << "|enviro@" << dec << record[i].end << "\n";	
		}
		out.close();
	}

	return 0;
}

void InitNewEpisode(size_t& step, board& current, string& movement, uint32_t& hint) {
	step = 0;
	current.Reset();
	movement = "00";
	hint = 0;
}

time_t millisec() {
	auto now = chrono::system_clock::now().time_since_epoch();
	return chrono::duration_cast<chrono::milliseconds>(now).count();
}

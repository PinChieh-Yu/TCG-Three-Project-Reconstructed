#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <regex>
#include <memory>
#include "board.h"
#include "agent.h"
#include "io.h"
using namespace std;

void InitNewEpisode();
time_t millisec();
void ApplyAction(string action);

struct episode {
	time_t start, end;
	vector<string> actions;
};

size_t step;
board current;
string movement;
uint32_t hint;
vector<episode> record;

int shell(int argc, const char* argv[]) {
	//arena host("anonymous");

	string login;

	string play_args, evil_args;
	uint8_t mode = 2; //0=>player 1=>envi 2=>not playing

	for (int i = 1; i < argc; i++) {
		string para(argv[i]);
		if (para.find("--name=") == 0 || para.find("--account=") == 0) {
			//host.set_account(para.substr(para.find("=") + 1));
		} else if (para.find("--login=") == 0) {
			login = para.substr(para.find("=") + 1);
			cout << "LOGIN:" << login << endl;
		} else if (para.find("--save=") == 0 || para.find("--dump=") == 0) {
			//host.set_dump_file(para.substr(para.find("=") + 1));
		} else if (para.find("--play") == 0) {
			play_args = para.substr(para.find("=") + 1);
			cout << "PLAY:" << play_args << endl;
		} else if (para.find("--evil") == 0) {
			evil_args = para.substr(para.find("=") + 1);
			cout << "ENVI:" << evil_args << endl;
		}
	}

	player play(play_args);
	environment envi;

	regex match_move("^#\\S+ \\S+$"); // e.g. "#M0001 ?", "#M0001 #U"
	regex match_ctrl("^#\\S+ \\S+ \\S+$"); // e.g. "#M0001 open Slider:Placer", "#M0001 close score=15424"
	regex arena_ctrl("^[@$].+$"); // e.g. "@ login", "@ error the account "Name" has already been taken"
	regex arena_info("^[?%].+$"); // e.g. "? message from anonymous: 2048!!!"

	for (string command; input() >> command; ) {
		try {
			if (regex_match(command, match_move)) {
				string id, move;
				stringstream(command) >> id >> move;
				cout << id << move << endl;

				if (move == "?") {
					// your agent need to take an action
					step++;
					if (mode == 0 || mode == 1) {
						if (max(step, size_t(9)) % 2) {
							envi.take_action(current, movement, hint);
							output() << id << ' ' << movement << "+" << hint << endl;
						} else {
							play.take_action(current, movement, hint);
							output() << id << ' ' << movement << endl;
						}
					} else {
						output() << id << ' ' << movement << endl;
					}
				} else {
					// perform your opponent's action
				    ApplyAction(move);
				}

			} else if (regex_match(command, match_ctrl)) {
				string id, ctrl, tag;
				stringstream(command) >> id >> ctrl >> tag;

				if (ctrl == "open") {
					// a new match is pending
					InitNewEpisode();
					if (mode != 0 && mode != 1) {
						if (tag.substr(0, tag.find(':')) == "pcyu_play") {
							mode = 0;
							output() << id << " open accept" << endl;
						} else if (tag.substr(tag.find(':') + 1) == "pcyu_envi") {
							mode = 1;
							output() << id << " open accept" << endl;
						} else {
							mode = 2;
							output() << id << " open reject" << endl;
						}
					} else {
						output() << id << " open reject" << endl;
					}
				} else if (ctrl == "close") {
					// a match is finished
					mode = 2;
					//host.close(id, tag);
				}

			} else if (regex_match(command, arena_ctrl)) {
				string ctrl;
				stringstream(command).ignore(1) >> ctrl;

				if (ctrl == "login") {
					output("@ ") << "login " << login << " pcyu_play(p) pcyu_envi(e)" << endl;
				} else if (ctrl == "status") {
					// display current local status
					info() << "+++++ status +++++" << endl;
					info() << "login " << login << " pcyu_play(p) pcyu_envi(e)" << endl;
					info() << "----- status -----" << endl;

				} else if (ctrl == "error" || ctrl == "exit") {
					// some error messages or exit command
					string message = command.substr(command.find_first_not_of("@$ "));
					info() << message << endl;
					break;
				}

			} else if (regex_match(command, arena_info)) {
				// message from arena server
			}
		} catch (std::exception& ex) {
			string message = string(typeid(ex).name()) + ": " + ex.what();
			message = message.substr(0, message.find_first_of("\r\n"));
			output("? ") << "exception " << message << " at \"" << command << "\"" << endl;
		}
	}

	return 0;
}


int main(int argc, const char* argv[]) {
	cout << "Three-Demo: ";
	copy(argv, argv + argc, ostream_iterator<const char*>(cout, " "));
	cout << endl << endl;

	srand(time(NULL));
	size_t total = 1000, block = 0, limit = 0;
	string play_args, evil_args;
	string load, save;
	bool summary = false;
	for (int i = 1; i < argc; i++) {
		string para(argv[i]);
		if (para.find("--shell") == 0) {
			return shell(argc, argv);
		} else if (para.find("--total=") == 0) {
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

	player play(play_args);
	rnd_environment envi;

	for (size_t turn = 1; turn <= total; turn++) {
		episode epi;
		InitNewEpisode();
		epi.start = millisec();
		while(true){
			step++;
			if (max(step, size_t(9)) % 2) {
				envi.take_action(current, movement, hint);
			} else {
				play.take_action(current, movement, hint);
			}
			
			if (movement == "??") {
				break;
			} else {
				epi.actions.push_back(movement);
			}
		}
		epi.end = millisec();
		play.backward_train();
		envi.reset_bag();
		record.push_back(epi);

		if (block != 0 && turn % block == 0) {
			cout << "Finish turn " << turn << endl;
		}
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

void InitNewEpisode() {
	step = 0;
	current.Reset();
	movement = "00";
	hint = 0;
}

time_t millisec() {
	auto now = chrono::system_clock::now().time_since_epoch();
	return chrono::duration_cast<chrono::milliseconds>(now).count();
}

void ApplyAction(string action) {
	step++;
	if (action == "#U") {
		current.slide(0);
	} else if (action == "#R") {
		current.slide(1);
	} else if (action == "#D") {
		current.slide(2);
	} else if (action == "#L") {
		current.slide(3);
	} else if (action.size() == 4) {
		uint8_t place, tile;
		place = action[0] < 'A' ? action[0] - '0' : action[0] - 55;
		tile = action[1] < 'A' ? action[1] - '0' : action[1] - 55;
		current(place) = tile;
		hint = action[3] - '0';
	}
}

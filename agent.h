#pragma once
#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <unordered_map>
#include <type_traits>
#include <algorithm>
#include <cmath>
#include <fstream>
#include "board.h"
using namespace std;

class player {
public:
	player(const string args = "") : opcode({ "#U", "#R", "#D", "#L" }), tuples({{
		{{{0,1,2,3,4,5},{4,5,6,7,8,9},{7,6,5,11,10,9},{15,14,13,11,10,9}}},
		{{{3,7,11,15,2,6},{2,6,10,14,1,5},{14,10,6,13,9,5},{12,8,4,13,9,5}}},
		{{{15,14,13,12,11,10},{11,10,9,8,7,6},{8,9,10,4,5,6},{0,1,2,4,5,6}}},
		{{{12,8,4,0,13,9},{13,9,5,1,14,10},{1,5,9,2,6,10},{3,7,11,2,6,10}}},
		{{{3,2,1,0,7,6},{7,6,5,4,11,10},{4,5,6,8,9,10},{12,13,14,8,9,10}}},
		{{{15,11,7,3,14,10},{14,10,6,2,13,9},{2,6,10,1,5,9},{0,4,8,1,5,9}}},
		{{{12,13,14,15,8,9},{8,9,10,11,4,5},{11,10,9,7,6,5},{3,2,1,7,6,5}}},
		{{{0,4,8,12,1,5},{1,5,9,13,2,6},{13,9,5,14,10,6},{15,11,7,14,10,6}}}}}), alpha(0.1)
	{
		stringstream ss(args);
		for (string pair; ss >> pair;) {
			string key = pair.substr(0, pair.find('='));
			string value = pair.substr(pair.find('=') + 1);
			if (key == "init") {
				InitWeightTable();
			} else if(key == "load") {
				LoadWeightTable(value);
			} else if(key == "save") {
				s_path = value;
			} else if(key == "alpha") {
				alpha = atof(value.c_str());
			}
		}
	}

	~player() {
		if (s_path.length())
			SaveWeightTable(s_path);
	}

	void take_action(board& before, string& movement, uint16_t& hint) {
		board after;
		unsigned int hash;
		int reward, final_reward = 0;
		int final_op = -1; 
		double value, highest_value = -2147483648;

		hint -= 1;
		for (int op = 0; op < 3; op++) { // four direction
			after = before;
			reward = after.slide(op);
			if (reward != -1) {
				// hash the tuple
				value = reward;
				for(int i = 0; i < 8; i++){
					for(int j = 0; j < 4; j++){
						hash = 0;
						for(int k = 0; k < 6; k++){
							hash = (hash << 4) + after(tuples[i][j][k]);
						}
						hash = (hash << 4) + (op << 2) + hint;
						value += table[j][hash];
					}
				}
				if (highest_value < value) {
					final_op = op;
					final_reward = reward;
					highest_value = value;
				}
			}
		}

		if (final_op != -1) {
			before.slide(final_op);
			board_records.push_back(before);
			reward_records.push_back(final_reward);
			hint_records.push_back(hint);
			move_records.push_back(final_op);
			movement = opcode[final_op];
		} else {
			movement = "??";
		}
	}

	void backward_train() {
		uint64_t hash;
		int pre_move, cur_move, pre_hint, cur_hint;
		board pre_board, cur_board;
		double pre_value, cur_value, result;
		//update end board
		pre_board = board_records.back();
		pre_move = move_records.back();
		pre_hint = hint_records.back();
		board_records.pop_back();
		move_records.pop_back();
		hint_records.pop_back();
		pre_value = 0;
		for(int i = 0; i < 8; i++){
			for(int j = 0; j < 4; j++){
				hash = 0;
				for(int k = 0; k < 6; k++){
					hash = (hash << 4) + pre_board(tuples[i][j][k]);
				}
				hash = (hash << 4) + (pre_move << 2) + pre_hint;
				pre_value += table[j][hash];
			}
		}
		result = (0 - pre_value) * alpha / 192;
		for(int i = 0; i < 8; i++){
			for(int j = 0; j < 4; j++){
				hash = 0;
				for(int k = 0; k < 6; k++){
					hash = (hash << 4) + pre_board(tuples[i][j][k]);
				}
				hash = (hash << 4) + (pre_move << 2) + pre_hint;
				table[j][hash] += result;
			}
		}

		//start backward train
		while (board_records.size() > 1) {
			cur_board = pre_board;
			cur_move = pre_move;
			cur_hint = pre_hint;
			pre_board = board_records.back();
			pre_move = move_records.back();
			pre_hint = hint_records.back();
			board_records.pop_back();
			move_records.pop_back();
			hint_records.pop_back();

			cur_value = 0;
			pre_value = 0;
			for(int i = 0; i < 8; i++){
				for(int j = 0; j < 4; j++){
					hash = 0;
					for(int k = 0; k < 6; k++){
						hash = (hash << 4) + cur_board(tuples[i][j][k]);
					}
					hash = (hash << 4) + (cur_move << 2) + cur_hint;
					cur_value += table[j][hash];
				}
				for(int j = 0; j < 4; j++){
					hash = 0;
					for(int k = 0; k < 6; k++){
						hash = (hash << 4) + pre_board(tuples[i][j][k]);
					}
					hash = (hash << 4) + (pre_move << 2) + pre_hint;
					pre_value += table[j][hash];
				}
			}

			cur_value += reward_records.back();
			reward_records.pop_back();

			result = (cur_value - pre_value) * alpha / 192;

			for(int i = 0; i < 8; i++){
				for(int j = 0; j < 4; j++){
					hash = 0;
					for(int k = 0; k < 6; k++){
						hash = (hash << 4) + pre_board(tuples[i][j][k]);
					}
					hash = (hash << 4) + (pre_move << 2) + pre_hint;
					table[j][hash] += result;
				}
			}
		}

		reward_records.clear();
		board_records.clear();
		hint_records.clear();
		move_records.clear();
	}

private:
	void InitWeightTable() {
		table.clear();
		table.resize(4);
	}

	void LoadWeightTable(string path) {
		table.clear();
		ifstream in(path, ios::in | ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t block;
		uint64_t size, key;
		float value;
		
		in.read(reinterpret_cast<char*>(&block), sizeof(uint32_t));
		table.resize(block);
		cout << "load block:" << block << endl;
		for (unsigned int b = 0; b < block; b++) {
			in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
			cout << "-load size:" << size << endl;
			while (size--) {
				in.read(reinterpret_cast<char*>(&key), sizeof(uint64_t));
				in.read(reinterpret_cast<char*>(&value), sizeof(float));
				table[b][key] = value;
			}
		}
		in.close();
	}

	void SaveWeightTable(string path) {
		ofstream out(path, ios::out | ios::binary | ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t block = table.size();
		uint64_t size;
		out.write(reinterpret_cast<const char*>(&block), sizeof(uint32_t));
		for (auto const& w : table) {
			size = w.size();
			out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
			for(auto const& pair : w) {
				//cout << pair.first << ":" << pair.second << endl;
    			out.write(reinterpret_cast<const char*>(&pair.first), sizeof(uint64_t));
    			out.write(reinterpret_cast<const char*>(&pair.second), sizeof(float));
			}
		}
		out.close();
	}

private:
	array<string, 4> opcode;
	vector<int> reward_records;
	vector<board> board_records;
	vector<int> hint_records;
	vector<int> move_records;
	array<array<array<int, 6>, 4>, 8> tuples;
	
	vector<unordered_map<uint64_t, float>> table;
	float alpha;
	string s_path;
};

class environment {
public:
	environment(const string args = "") : policy({{{12, 13, 14, 15}, {0, 4, 8, 12}, {0, 1, 2, 3}, {3, 7, 11, 15}}}), 
	point({{"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"}}), bonus_count(0), total_count(0)
	{ 
		reset_bag();
		stringstream ss(args);
		for (string pair; ss >> pair;) {
			string key = pair.substr(0, pair.find('='));
			string value = pair.substr(pair.find('=') + 1);
			if(key == "load") {
				LoadWeightTable(value);
			}
		} 
	}

	void take_action(board& after, string& movement, uint16_t& hint) {
		int place, dir = -1;

		//setup position
		if (movement == "#U") {
			dir = 0;
		} else if (movement == "#R") {
			dir = 1;
		} else if (movement == "#D") {
			dir = 2;
		} else if (movement == "#L") {
			dir = 3;
		}
		if (dir != -1){
			do {
				place = policy[dir][rand() % 4];
			} while (after(place) != 0);
		} else {
			do {
				place = rand() % 16;
			} while (after(place) != 0);
		}

		tile = next;
		max = after.max();
		total_count++;
		if (max >= 7 && ((double)rand() / RAND_MAX) <= (1.0f / 21.0f)) {
			next = round(4 + ((double)rand() / RAND_MAX) * (max - 7));
			hint = 4;
			bonus_count++;
		} else {
			next = get_tile_from_bag();
			hint = next;
		}
		
		after(place) = tile;
		movement = point[place] + point[tile];
	}

	void reset_bag(){
		bag.clear();
		next = get_tile_from_bag();
	}

private:
	uint16_t get_tile_from_bag() {
		uint16_t tile;
		if (bag.empty()) {
			for(int i = 0; i < 4; i++){
				bag.push_back(1);
				bag.push_back(2);
				bag.push_back(3);
			}
			random_shuffle(bag.begin(), bag.end());
		}
		tile = bag.back();
		bag.pop_back();
	
		return tile;
	}

	void LoadWeightTable(string path) {
		table.clear();
		ifstream in(path, ios::in | ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t block;
		uint64_t size, key;
		float value;
		
		in.read(reinterpret_cast<char*>(&block), sizeof(uint32_t));
		table.resize(block);
		cout << "load block:" << block << endl;
		for (unsigned int b = 0; b < block; b++) {
			in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
			cout << "-load size:" << size << endl;
			while (size--) {
				in.read(reinterpret_cast<char*>(&key), sizeof(uint64_t));
				in.read(reinterpret_cast<char*>(&value), sizeof(float));
				table[b][key] = value;
			}
		}
		in.close();
	}

private:
	vector<unordered_map<uint64_t, float>> table;

	array<array<int, 4>, 4> policy;
	array<string, 16> point;
	vector<int> bag;
	uint16_t next;

	uint16_t tile;
	uint16_t max;

	uint16_t bonus_count;
	uint16_t total_count;
};

class rnd_environment {
public:
	rnd_environment() : policy({{{12, 13, 14, 15}, {0, 4, 8, 12}, {0, 1, 2, 3}, {3, 7, 11, 15}}}),
		point({{"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"}}) { reset_bag(); }

	void take_action(board& after, string& movement, uint16_t& hint) {
		int place, dir = -1;

		if (movement == "#U") {
			dir = 0;
		} else if (movement == "#R") {
			dir = 1;
		} else if (movement == "#D") {
			dir = 2;
		} else if (movement == "#L") {
			dir = 3;
		}
		if (dir != -1){
			do {
				place = policy[dir][rand() % 4];
			} while (after(place) != 0);
		} else {
			do {
				place = rand() % 16;
			} while (after(place) != 0);
		}

		tile = next;
		max = after.max();
		if (max >= 7 && ((double)rand() / RAND_MAX) <= (1.0f / 21.0f)) {
			next = round(4 + ((double)rand() / RAND_MAX) * (max - 7));
			hint = 4;
		} else {
			next = get_tile_from_bag();
			hint = next;
		}
		
		after(place) = tile;
		movement = point[place] + point[tile];
	}

	void reset_bag(){
		bag.clear();
		next = get_tile_from_bag();
	}

private:
	uint32_t get_tile_from_bag() {
		uint32_t tile;
		if (bag.empty()) {
			for(int i = 0; i < 4; i++){
				bag.push_back(1);
				bag.push_back(2);
				bag.push_back(3);
			}
			random_shuffle(bag.begin(), bag.end());
		}
		tile = bag.back();
		bag.pop_back();
	
		return tile;
	}

private:
	array<array<int, 4>, 4> policy;
	array<string, 16> point;
	vector<int> bag;
	uint32_t next;

	uint32_t tile;
	uint32_t max;
};

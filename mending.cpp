#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <ctime>
#include "board.h"
using namespace std;

void LoadWeightTable(string path, vector<unordered_map<uint32_t, float>>& table) {
	table.clear();
	ifstream in(path, ios::in | ios::binary);
	if (!in.is_open()) std::exit(-1);
	uint32_t block;
	uint32_t size, key;
	float value;
		
	in.read(reinterpret_cast<char*>(&block), sizeof(uint32_t));
	table.resize(block);
	cout << "load block:" << block << endl;
	for (unsigned int b = 0; b < block; b++) {
		in.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
		cout << "-load size:" << size << endl;
		while (size--) {
			in.read(reinterpret_cast<char*>(&key), sizeof(uint32_t));
			in.read(reinterpret_cast<char*>(&value), sizeof(float));
			table[b][key] = value;
		}
	}
	in.close();
}

void SaveWeightTable(string path, vector<unordered_map<uint32_t, float>>& table) {
	ofstream out(path, ios::out | ios::binary | ios::trunc);
	if (!out.is_open()) std::exit(-1);
	uint32_t block = table.size();
	uint32_t size;
	out.write(reinterpret_cast<const char*>(&block), sizeof(uint32_t));
	for (auto const& w : table) {
		size = w.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
		for(auto const& pair : w) {
			//cout << pair.first << ":" << pair.second << endl;
    		out.write(reinterpret_cast<const char*>(&pair.first), sizeof(uint32_t));
    		out.write(reinterpret_cast<const char*>(&pair.second), sizeof(float));
		}
	}
	out.close();
}

int main(int argc, const char* argv[]) {
	string path_1(argv[1]);
	string path_2(argv[2]);
	string result(argv[3]);
	cout << "First:" << path_1 << endl << "Second:" << path_2 << endl << "Result:" << result << endl;

	vector<unordered_map<uint32_t, float>> first, second;
	LoadWeightTable(path_1, first);
	LoadWeightTable(path_2, second);

	srand (time(NULL));

	cout << "Start mending" << endl;
	unordered_map<uint32_t, float>::iterator it;
	for(int i = 0; i < 4; i++) {
		it = second[i].begin();
		while (it != second[i].end()) {
			if (first[i].find(it->first) == first[i].end()) {
				first[i][it->first] = it->second;
			} else {
				first[i][it->first] = first[i][it->first] / 2 + it->second / 2;
			}
			it = second[i].erase(it);
		}
	}
	cout << "Start saving" << endl;
	SaveWeightTable(result, first);
}
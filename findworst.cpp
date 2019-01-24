#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <memory>
#include <vector>
#include <array>
#include "board.h"
#include "agent.h"
using namespace std;

worst_player play("load=weights.bin");
double min_value = 2147483647, value;
board result;
int hint, result_hint;
void dfs(board& b, array<int, 3>& bag, int last, int count) {
	if (count == 9) {
		value = play.take_action(b, bag, hint);
		if (value < min_value) {
			min_value = value;
			result = b;
			result_hint = hint;
		}
		return;
	}
	for (int i = last+1; i < 16; i++) {
		for (int t = 0; t < 3; t++) {
			if (bag[t] == 0) continue;
			bag[t]--;
			b(i) = (t+1);
			dfs(b, bag, i, count+1);
			bag[t]++;
			b(i) = 0;
		}
	}
	return;
}
int main(int argc, const char* argv[]) {
	board tmp;
	array<int, 3> bag({{4, 4, 4}});
	cout << "Start finding" << endl;
	dfs(tmp, bag, -1, 0);
	cout << "Worst Board:" << endl;
	cout << result << endl;
	cout << "Next Hint:" << endl;
	cout << result_hint << endl;
	return 0;
}
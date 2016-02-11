#include "js_connection.h"

#include <iostream>
#include <string>

#include "test.h"

using namespace std;

void OnEvent() {
	cout << "Lost Connection!\n";
}

int Test::test_js() {
	JSConnection jsconn;
	jsconn.AddLostConnListener(&OnEvent);
	jsconn.Start();

	cin.ignore();
	return 1;
}

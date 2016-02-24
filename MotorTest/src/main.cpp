#include <ncurses.h>
#include <pigpio.h>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

static const int kEscRate = 400; // hz

int g_motorSpeed[4] = { 1000, 1000, 1000, 1000 };
int g_motorGpio[4] = { 23, 18, 15, 14 };
bool g_selected[4] { false };
WINDOW* g_statWin;
WINDOW* g_helpWin;

void PrintMotors() {
	stringstream ss;
	for (int i = 0; i < 4; i++) {
		if (g_selected[i])
			ss << "*";
		ss << "Motor " << (i + 1) << ": " << g_motorSpeed[i] << "\n";
	}
	mvwaddstr(g_statWin, 0, 0, ss.str().c_str());
	wrefresh(g_statWin);
}

void UpdateMotors() {
	for (int i = 0; i < 4; i++) {
		if (g_motorSpeed[i] > 2000)
			g_motorSpeed[i] = 2000;
		if (g_motorSpeed[i] < 1000)
			g_motorSpeed[i] = 1000;
	}
	gpioPWM(g_motorGpio[0], g_motorSpeed[0]);
	gpioPWM(g_motorGpio[1], g_motorSpeed[1]);
	gpioPWM(g_motorGpio[2], g_motorSpeed[2]);
	gpioPWM(g_motorGpio[3], g_motorSpeed[3]);
}

void IncreaseMotors(int amount) {
	for (int i = 0; i < 4; i++) {
		if (g_selected[i]) {
			g_motorSpeed[i] += amount;
		}
	}
}

void DecreaseMotors(int amount) {
	for (int i = 0; i < 4; i++) {
		if (g_selected[i]) {
			g_motorSpeed[i] -= amount;
		}
	}
}

void resetMotors() {
	for (int i = 0; i < 4; i++) {
		g_motorSpeed[i] = 1000;
	}
}

void onexit() {
	endwin();
	gpioPWM(g_motorGpio[0], 1000);
	gpioPWM(g_motorGpio[1], 1000);
	gpioPWM(g_motorGpio[2], 1000);
	gpioPWM(g_motorGpio[3], 1000);
	gpioTerminate();
}

void signal_handler(int sig) {
	onexit();
	exit(1);
}

void CalibrateESC() {
	cout << "Make sure ESC is in standby mode (beeping every few sec)\n";
	cout << "Press <ENTER> to start ESC calibration";
	cin.ignore();
	gpioPWM(g_motorGpio[0], 2000);
	gpioPWM(g_motorGpio[1], 2000);
	gpioPWM(g_motorGpio[2], 2000);
	gpioPWM(g_motorGpio[3], 2000);
	cout << "Press <ENTER> after beep-beep tone is heard";
	cin.ignore();
	gpioPWM(g_motorGpio[0], 1000);
	gpioPWM(g_motorGpio[1], 1000);
	gpioPWM(g_motorGpio[2], 1000);
	gpioPWM(g_motorGpio[3], 1000);
	cout << "Calibration done\n";
	cout << "Press <ENTER> to exit";
	cin.ignore();
}

void Motor_test() {
	initscr();
	curs_set(0);
	noecho();
	g_statWin = newwin(5, COLS, LINES - 5, 0);
	g_helpWin = newwin(getbegy(g_statWin), COLS, 0, 0);
	refresh();
	waddstr(g_helpWin, "1, 2, 3, 4: Select/unselect motor\n");
	waddstr(g_helpWin, "q, a: Increase/decrease by 10\n");
	waddstr(g_helpWin, "w, s: Increase/decrease by 50\n");
	waddstr(g_helpWin, "e, d: Increase/decrease by 100\n");
	waddstr(g_helpWin, "r: Reset all to 1000\n");
	waddstr(g_helpWin, "z: Quit\n");
	wrefresh(g_helpWin);

	UpdateMotors();
	PrintMotors();

	char input;
	do {
		input = getch();
		switch (input) {
		case '1':
			g_selected[0] = !g_selected[0];
			break;
		case '2':
			g_selected[1] = !g_selected[1];
			break;
		case '3':
			g_selected[2] = !g_selected[2];
			break;
		case '4':
			g_selected[3] = !g_selected[3];
			break;
		case 'q':
			IncreaseMotors(10);
			break;
		case 'a':
			DecreaseMotors(10);
			break;
		case 'w':
			IncreaseMotors(50);
			break;
		case 's':
			DecreaseMotors(50);
			break;
		case 'e':
			IncreaseMotors(100);
			break;
		case 'd':
			DecreaseMotors(100);
			break;
		case 'r':
			resetMotors();
			break;
		default:
			break;
		}
		UpdateMotors();
		PrintMotors();
	} while (input != 'z');
}

int main(int argc, char** argv) {
	if (gpioInitialise() < 0) {
		return (-1);
	}
	atexit(onexit);
	signal(SIGINT, signal_handler);

	gpioSetPWMfrequency(g_motorGpio[0], kEscRate);
	gpioSetPWMrange(g_motorGpio[0], 1000000 / kEscRate);
	gpioSetPWMfrequency(g_motorGpio[1], kEscRate);
	gpioSetPWMrange(g_motorGpio[1], 1000000 / kEscRate);
	gpioSetPWMfrequency(g_motorGpio[2], kEscRate);
	gpioSetPWMrange(g_motorGpio[2], 1000000 / kEscRate);
	gpioSetPWMfrequency(g_motorGpio[3], kEscRate);
	gpioSetPWMrange(g_motorGpio[3], 1000000 / kEscRate);

	if (argc > 1 && strcmp(argv[1], "-c") == 0) {
		CalibrateESC();
	} else {
		Motor_test();
	}
}

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

int motor_speed_[4] = { 1000, 1000, 1000, 1000 };
int motor_gpio_[4] = { 23, 18, 15, 14 };
bool selected[4] { false };
WINDOW* stat_win;
WINDOW* help_win;

void PrintMotors() {
	stringstream ss;
	for (int i = 0; i < 4; i++) {
		if (selected[i])
			ss << "*";
		ss << "Motor " << (i + 1) << ": " << motor_speed_[i] << "\n";
	}
	mvwaddstr(stat_win, 0, 0, ss.str().c_str());
	wrefresh(stat_win);
}

void UpdateMotors() {
	for (int i = 0; i < 4; i++) {
		if (motor_speed_[i] > 2000)
			motor_speed_[i] = 2000;
		if (motor_speed_[i] < 1000)
			motor_speed_[i] = 1000;
	}
	gpioPWM(motor_gpio_[0], motor_speed_[0]);
	gpioPWM(motor_gpio_[1], motor_speed_[1]);
	gpioPWM(motor_gpio_[2], motor_speed_[2]);
	gpioPWM(motor_gpio_[3], motor_speed_[3]);
}

void IncreaseMotors(int amount) {
	for (int i = 0; i < 4; i++) {
		if (selected[i]) {
			motor_speed_[i] += amount;
		}
	}
}

void DecreaseMotors(int amount) {
	for (int i = 0; i < 4; i++) {
		if (selected[i]) {
			motor_speed_[i] -= amount;
		}
	}
}

void ResetMotors() {
	for (int i = 0; i < 4; i++) {
		motor_speed_[i] = 1000;
	}
}

void onexit() {
	endwin();
	gpioPWM(motor_gpio_[0], 1000);
	gpioPWM(motor_gpio_[1], 1000);
	gpioPWM(motor_gpio_[2], 1000);
	gpioPWM(motor_gpio_[3], 1000);
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
	gpioPWM(motor_gpio_[0], 2000);
	gpioPWM(motor_gpio_[1], 2000);
	gpioPWM(motor_gpio_[2], 2000);
	gpioPWM(motor_gpio_[3], 2000);
	cout << "Press <ENTER> after beep-beep tone is heard";
	cin.ignore();
	gpioPWM(motor_gpio_[0], 1000);
	gpioPWM(motor_gpio_[1], 1000);
	gpioPWM(motor_gpio_[2], 1000);
	gpioPWM(motor_gpio_[3], 1000);
	cout << "Calibration done\n";
	cout << "Press <ENTER> to exit";
	cin.ignore();
}

void Motor_test() {
	initscr();
	curs_set(0);
	noecho();
	stat_win = newwin(5, COLS, LINES - 5, 0);
	help_win = newwin(getbegy(stat_win), COLS, 0, 0);
	refresh();
	waddstr(help_win, "1, 2, 3, 4: Select/unselect motor\n");
	waddstr(help_win, "q, a: Increase/decrease by 10\n");
	waddstr(help_win, "w, s: Increase/decrease by 50\n");
	waddstr(help_win, "e, d: Increase/decrease by 100\n");
	waddstr(help_win, "r: Reset all to 1000\n");
	waddstr(help_win, "z: Quit\n");
	wrefresh(help_win);

	UpdateMotors();
	PrintMotors();

	char input;
	do {
		input = getch();
		switch (input) {
		case '1':
			selected[0] = !selected[0];
			break;
		case '2':
			selected[1] = !selected[1];
			break;
		case '3':
			selected[2] = !selected[2];
			break;
		case '4':
			selected[3] = !selected[3];
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
			ResetMotors();
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

	gpioSetPWMfrequency(motor_gpio_[0], kEscRate);
	gpioSetPWMrange(motor_gpio_[0], 1000000 / kEscRate);
	gpioSetPWMfrequency(motor_gpio_[1], kEscRate);
	gpioSetPWMrange(motor_gpio_[1], 1000000 / kEscRate);
	gpioSetPWMfrequency(motor_gpio_[2], kEscRate);
	gpioSetPWMrange(motor_gpio_[2], 1000000 / kEscRate);
	gpioSetPWMfrequency(motor_gpio_[3], kEscRate);
	gpioSetPWMrange(motor_gpio_[3], 1000000 / kEscRate);

	if (argc > 1 && strcmp(argv[1], "-c") == 0) {
		CalibrateESC();
	} else {
		Motor_test();
	}
}

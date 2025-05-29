#include <iostream>
#include "memory.hpp"
#include "gui.hpp"
#include "app.hpp"

int main()
{
	if (!memory::initialize())
		return -1;
	gui::initialize();
	while (!app::exit) {
		Sleep(100);
	}
	memory::cleanup();
}

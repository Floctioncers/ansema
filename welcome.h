#ifndef	WELCOME_H
#define WELCOME_H

#include <string>

namespace Welcome
{
	inline std::string const WelcomeText{
		"ANSEMA ANother\n"
		"    SEcret MAnager is a simple application written in C++ which should help you to protect your secrets and passwords.\n\n"
		"HELP\n"
		"    Program consist of two functions : Password generator\n"
		"    and Secret editor.\n\n"
		"    PASSWORD GENERATOR\n"
		"        Simple password generator.Takes a formula that\n"
		"        consists of :\n"
		"            n - generates a digit,\n"
		"            a - generates lower english alphabet char,\n"
		"            A - same as a but uppercase,\n"
		"            -. * _ - generate - . * _ respectivelly,\n"
		"            x - generates one of these ,.!? ; : chars,\n"
		"            X - generates one of these @#$%^&* chars.\n"
		"        You can generate them with button Generate!, double\n"
		"        click on generated input will copy it in your clipboard.\n\n"
		"    SECRET EDITOR\n"
		"        Simple text editor with a little enhancement, when you\n"
		"        put  anything between[and], it will be only visible in\n"
		"        edit mode.In view mode you can double click on the\n"
		"        generated ****** and the hidden word will be copied\n"
		"        in your clipboard.\n"
		"        Intented usage is to put username and password\n"
		"        between bracket and when needed just to savely copy\n"
		"        them without worrying that  someone is looking.\n\n"
		"LICENSES\n"
		"	...\n\n"
		"DEPENDENCIES\n"
		"	...\n\n"
	};
}

#endif // !

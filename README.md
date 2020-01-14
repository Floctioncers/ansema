# Ansema

Another secret manager is a simple gui executable, that will protect your
secrets, mainly passwords and account names.

## Getting Started

Building and running the program should be straightforward, you should be able
to run it on Linux, Windows, MacOS and maybe others - depending on the Nana and
Cryptopp projects.

### Prerequisites

You'll need Visual Studio 2019, that should be all.

### Application Manual

Program consists of two modules: Password generator and Secret editor.

#### Password generator
Simple password generator. Takes a formula that consists of:
* n - generates a digit,
* a - generates lower english alphabet char,
* A - same as a but uppercase,
* -. * _ - generate - . * _ respectivelly,
* x - generates one of these ,.!? ; : chars,
* X - generates one of these @#$%^&* chars.
	
You can generate them with button Generate!, double click on generated input
will copy it in your clipboard.

#### Secret editor
Simple text editor with a little enhancement, when you put  anything between
[ and ], it will be only visible in edit mode.In view mode you can double click
on the generated [ secret ] and the hidden word will be copied in your
clipboard. Intented usage is to put username and password between bracket and
when needed just to savely copy them without worrying that someone is looking.

## Built With

* [nana](http://nanapro.org/en-us/) - Used for GUI
* [crypto++](https://cryptopp.com) - Dependency Management

## Authors

* **Floctioncers** - floctioncers@tuta.io

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details


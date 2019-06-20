# Ansema

Another secret manager is a simple gui executable, that will protect your
secrets, mainly passwords and account names.

## Getting Started

Building and running the program should be straightforward, you should be able
to run it on Linux, Windows, MacOS and maybe others - depending on the Nana and
Cryptopp projects.

### Prerequisites

You'll need CMake version 3.14 (older version may function as well) and C++17
compiler - tested with MingW and Microsoft C++ compiler.

### Application Manual

Program consists of two modules: Password generator and Secret editor.

#### Password generator
Simple password generator.Takes a formula that consists of:
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


## Running the tests

Explain how to run the automated tests for this system

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone whose code was used
* Inspiration
* etc


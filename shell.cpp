#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>

#include <vector>

struct Cmd {
    std::vector<std::string> parts = {}; //assumindo que o comando possua partes (argumentos, instrucoes etc)
};

struct Expr {  //expressão interpretanda
    std::vector<Cmd> cmds; //conjunto de comandos esmiuçados
    std::string inputFromFile;
    std::string outputToFile;
    bool background = false; //sem background
};

std::vector<std::string> splitString(const std::string &str, char delmit = ' ') {
    std::vector<std::string> retval;
    for (size_t pos = 0; pos < str.length();) {
        size_t fnd = str.find(delmit, pos);
        if (fnd == std::string::npos) {
            retval.push_back(str.substr(pos));
            break;
        }
        if (fnd != pos)
            retval.push_back(str.substr(pos, fnd - pos));
        pos = fnd + 1;
    }
    return retval;
}

int execvp(const std::vector<std::string> &args) {
    const char **c_args = new const char *[args.size() + 1];
    for (size_t i =0; i < args.size(); i++) {
        c_args[i] = args[i].c_str();
    }
    c_args[args.size()]= nullptr;
    ::execvp(c_args[0], const_cast<char **>(c_args));
    int retval = errno;
    delete[] c_args;
    return retval;
}

int execmd(const Cmd &cmd)
{
    auto &parts = cmd.parts;
    if (parts.size() == 0)
        return EINVAL;

    int retval = execvp(parts);
    return retval;
}

void disprmp() {
    char bffr[512];
    char *dir = getcwd(bffr, sizeof(bffr));
    if (dir) {
        std::cout << R"(e[32m)" << dir << R"(e[39m)";
    }
    std::cout << "$";
    flush(std::cout);
}

std::string reqstcmdl(bool showPrompt) {
        if (showPrompt) {
            disprmp();
        }
    std::string retval;
    getline(std::cin, retval);
    return retval;
}

Expr parsecmdl(std::string cmdl) {
    Expr expression;
    std::vector<std::string> cmds = splitString(cmdl,'|');
    for (size_t i = 0; i <cmds.size(); ++i) {
        std::string &line = cmds[i];
        std::vector<std::string> args = splitString(line, '');
        if (i == cmds.size() -1 && args.size() > 1 && args[args.size() - 1] == "&&") {
            expression.background = true;
            args.resize(args.size() - 1);
        }
        if (i == cmds.size() - 1 && args.size() > 2 && args[args.size() -2] == ">") {
            expression.outputToFile = args[args.size() - 1];
            args.resize(args.size() - 2);
        }
        if (i == 0 && args.size() > 2 && args[args.size() - 2] == "<") {
            expression.inputFromFile = args[args.size() - 1];
            args.resize(args.size() - 2);
        }
        expression.cmds.push_back({args});

    }
    return expression;
}

int execCmds(Expr &expression)
{
	int pipes[expression.cmds.size()-1][2] = {};
	int READ_END = 0;
	int WRITE_END = 1;
	int outputFileid = -1;
	int inputFileid = -1;
	if (expression.outputToFile != "")
	{
		outputFileid = open(expression.outputToFile.c_str(), O_CREAT | O_WRONLY, 0777);
		if (outputFileid < 0)
		{
			std::cout << "Opening file failed." << '\n';
		}
	}
		if (expression.inputFromFile != "")
		{
			inputFileid = open(expression.inputFromFile.c_str(), O_RDONLY, S_IRUSR);
			if (inputFileid < 0)
			{
				std::cout << "Opening file failed/does not exist" << '\n';
			}
		}
		for (int i = 0; i < expression.cmds.size(); i++)
		{
			bool isFirst = i == 0;
			bool isLast = i == expression.cmds.size() - 1;
			if (expression.cmds[i].parts[0] == "exit")
			{
				std::cout << "Exited sucessfully" << '\n';
				exit(EXIT_SUCESS);
			}
			else if (expression.cmds[i].parts[0] == "cd")
			{
				int ch = chdir(expression.cmds[i].parts[1].c_str());
				if (ch < 0)
				{
					std::cout << "Changing dir not succ" << '\n';
				}
				break;
			}
			else 
			{
				if(pipe(pipes[i]))
				{
					std::cout << "Creating pipe failed" << '\n';
					break;
				}
				pid_t child1 = fork();
			}
		}
}

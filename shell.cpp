#include <cstdio>
#include <cstring>
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
				if (child1 == -1){
					std::cout << "Creating child process failed.";
					break;
				}
				if (child1 == 0)
				{
					bool fileWasHandled = false;
					if (isFirst && inputFileid >=0){
						dup2(inputFileid, STDIN_FILENO);
						if(expression.cmds.size() > 1)
						{
							dup2(pipes[i][WRITE_END], STDOUT_FILENO);
						}
						close(pipes[i][READ_END]);
						close(pipes[i][WRITE_END]);
						close(inputFileid);
						fileWasHandled = true;
					}
					if(isLast && outputFileid >=0)
					{
						if(!isFirst){
						close(pipes[i - 1][WRITE_END]);
						dup2(pipes[i - 1][READ_END], STDIN_FILENO);
						close(pipes[i - 1][READ_END]);

						}
						else {
							close(pipes[i][READ_END]);
							close(pipes[i][WRITE_END]);
						}
						dup2(outputFileid, STDOUT_FILENO);
						close(outputFileid);
						fileWasHandled = true;
					}
					if (!fileWasHandled)						{
						if(!isFirst)
						{
							close(pipe[i - 1][WRITE_END]);
							dup2(pipes[i - 1][READ_END], STDIN_FILENO);
							close(pipe[i - 1][READ_END]);
						}
						if (!isLast)

						{
							close(pipe[i][READ_END]);
							dup2(pipes[i][WRITE_END, STDOUT_FILENO]);
							close(pipes[i][WRITE_END]);
						}
			}
					execmd(expression.cmds[i]);
					abort();
		}
				if (!isFirst)
				{
					close(pipes[i - 1][READ_END]);
					close(pipes[i - 1][WRITE_END]);
				}
				waitpid(child1, nullptr, 0);
				}
		}

	return 0;

}
int executeExpression(Expr &expression)
{
	if (expression.cmds.size() == 0)
		return EINVAL;
	if (expression.background)
	{
		pid_t pid = fork();
		if (pid == -1)
		{
			std::cout << "Cmd exec in bg not possible";
		}
		else if (pid == 0)
		{
			return execCmds(expression);
		}
	}
	else {
		return execCmds(expression);
	}
	return 0;
}
int normal(bool showPrompt)
{
	while(std::cin.good())
	{
		std::string cmdl = reqstcmdl(showPrompt);
		Expr expression = parsecmdl(cmdl);

		int rc = executeExpression(expression);
		if (rc!=0)
			std::cer << std::strerror(rc) << '\n';
	}
	return 0;
}

int step1(bool showPrompt)
{
	int mypipe[2];
	if (pipe(mypipe))
	{
		std::printf(std::error, "Pipe failed.\n");
		return EXIT_FAILURE;
	}
	pid_t child2 = fork();
	if (child2 == 0)
	{
		close(mypipe[0]);
		dup2(mypipe[1], STDOUT_FILENO);
		Cmd cmd = {{std::string("date")}};
		execmd(cmd);
		abort();
	}
	close(mypipe[0]);
	close(mypipe[1]);
	waitpid(child1, nullptr, 0);
	waitpid(child2, nullptr, 0);
	return 0;
}
int shell (bool showPrompt)
{
	return normal(showPrompt);
}

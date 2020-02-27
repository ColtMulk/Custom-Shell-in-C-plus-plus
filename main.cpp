#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <vector>
#include <cstring>


using namespace std;

vector<string> *split(string line, string splitChars) {
    vector<string> *chopped = new vector<string>;
    int beg = 0;
    for (int i = 0; i < line.length(); i++) {
        for (int j = 0; j < splitChars.length(); j++) {
            if (line.at(i) == splitChars.at(j)) {
                if (beg != i) {
                    chopped->push_back(line.substr(beg, (i - beg)));
                }
                beg = i + 1;
                break;
            }
        }
    }
    if (beg < line.length()) {
        chopped->push_back(line.substr(beg));
    }
    return chopped;
}

char **getArguments(string line) {
    vector<string> *seperated = split(line, " ");
    char **args = new char *[seperated->size() + 1];
    for (int i = 0; i < seperated->size(); i++) {
        args[i] = (char *) seperated->at(i).c_str();
    }
    args[seperated->size()] = NULL;
    return args;


}

void execute(string function) {
    char **args = getArguments(function);
    execvp(args[0], args);
    cout << "Error " << args[0] << " not found" << endl;
    exit(0);
}

int main() {
    while (true) {
        cout << "My Shell$ ";
        string inputline;
        getline(cin, inputline);
        cout << "Input Line: " << inputline << endl;

        vector<string> *pipes = split(inputline, "|");
        cout << pipes->size() << endl;
        if (pipes->size() > 1) {
            for (int i = 0; i < pipes->size(); i++) {
                int fd[2];
                pipe(fd);
                if (!fork()) {
                    if (i < pipes->size() - 1) {
                        //cout << "PIPES " << pipes->at(i) << endl;
                        dup2(fd[1], 1);
                        close(fd[1]);
                    }
                    execute(pipes->at(i));
                } else {
                    dup2(fd[0], 0);
                    string moreInput;
                    getline(cin, inputline);
                    cout << "after dup " << moreInput << endl;
                    close(fd[1]);
                }
            }
        } else {

            if (inputline == string("exit")) {
                cout << "Bye!! End of shell" << endl;
                break;
            }

            int pid = fork();
            if (pid == 0) {
                execute(inputline);
            } else {
                wait(0);
            }
        }
        cout << "Pipes done" << endl;
        pipes->clear();

    }
    return 0;
}

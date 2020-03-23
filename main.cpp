#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <fstream>
#include <sys/fcntl.h>


using namespace std;

string *removeChars(string s, string chars) {
    //removes characters in chars from s that aren't in quotes
    string *removed = new string;
    bool inQuotes = false;
    for (int i = 0; i < s.length(); i++) {
        bool add = true;
        if (s.at(i) == '\"' | s.at(i) == '\'') {
            inQuotes = !inQuotes;
        }
        for (int j = 0; j < chars.length(); j++) {
            if (chars.at(j) == s.at(i) && !inQuotes) {
                add = false;
            }
        }
        if (add) {
            removed->push_back(s.at(i));
        }
    }
    return removed;
}

vector<string> *split(string line, string splitChars) {
    //splits based on split chars but ignores anything in '' or ""
    vector<string> *chopped = new vector<string>;
    int beg = 0;
    bool quotes = false;
    for (int i = 0; i < line.length(); i++) {
        //flipes quotes bool if quotes are found
        if (line.at(i) == '\'' | line.at(i) == '\"') {
            quotes = !quotes;
        }
        for (int j = 0; j < splitChars.length(); j++) {
            if (line.at(i) == splitChars.at(j) && !quotes) {
                //checks if a split character followed a previous split character
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
    //cerr << "start" << endl;
    vector<string> *seperated = split(line, " ");
    char **args = new char *[seperated->size() + 1];
    for (int i = 0; i < seperated->size(); i++) {
        string *trimed = removeChars(seperated->at(i), " \"\'"); //removes spaces and quotes
        //cerr << *trimed << endl;
        args[i] = (char *) trimed->c_str();
    }
    args[seperated->size()] = NULL;
    return args;


}


void execute(string function) {
    //cerr << "execute" << endl;
    char **args = getArguments(function);
    execvp(args[0], args);
    cout << "Error " << args[0] << " not found" << endl;
    exit(0);
}

void outputRedirection(string command, string file) {
    int *status;
    int id = fork();
    if (!id) {
        int output = open(file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        dup2(output, 1);
        close(output);
        execute(command);
        cerr << "ERROR execute failed" << endl;
        exit(0);
    }
    waitpid(id, status, 0);
}

void inputRedirection(string command, string file) {
    int id = fork();
    int *status;
    if (!id) {
        int input = open((char *) file.c_str(), O_RDONLY);
        dup2(input, 0);
        close(input);
        execute(command);
        cerr << "ERROR execute failed" << endl;
        exit(0);
    }
    waitpid(id, status, 0);
}

void checkRedirectionA() {
    //checks for redirection and for & to determine if to run in background or not
}

int main() {
    vector<int> *pids = new vector<int>;
    int *status = new int;
    while (true) {
        for (int i = 0; i < pids->size(); i++) {
            cerr << "pid" << i << endl;
            waitpid(pids->at(i), status, WNOHANG);
            if(WIFSTOPPED(*status)){
                pids->erase(pids->begin()+i);
                cerr << "Sleep Exited" << endl;
            }
        }
        //cerr << "After pids" << endl;
        cout << "My Shell$ ";
        //fflush(stdout);
        string inputline;
        getline(cin, inputline);
        //cout << "Input Line: " << inputline << endl;

        if (inputline == string("exit")) {
            cout << "Bye!! End of shell" << endl;
            break;
        }

        vector<string> *pipes = split(inputline, "|");
        if (pipes->size() > 1) {
            int standIn = dup(0);
            for (int i = 0; i < pipes->size(); i++) {
                int fd[2];
                pipe(fd);
                int id = fork();
                if (!id) {
                    if (i < pipes->size() - 1) {
                        //cout << "PIPES " << pipes->at(i) << endl;
                        dup2(fd[1], 1);
                        close(fd[1]);
                    }
                    execute(pipes->at(i));
                } else {
                    waitpid(id, status, 0);
                    dup2(fd[0], 0);
                    close(fd[1]);
                }
            }
            dup2(standIn, 0);
            close(standIn);
        } else {
            //cerr << "after input" << endl;
            vector<string> *redirect = split(inputline, "<");
            if (redirect->size() == 2) {
                inputRedirection(redirect->at(0), *removeChars(redirect->at(1), " ")); //removes spaces from file name
                delete redirect;
                continue;
            }
            redirect = split(inputline, ">");
            if (redirect->size() == 2) {
                outputRedirection(redirect->at(0), *removeChars(redirect->at(1), " "));
                delete redirect;
                continue;
            }
            delete redirect;


            int pid = fork();
            if (pid == 0) {
                execute(inputline);
            } else {
                waitpid(pid, status, 0);
            }
        }

        int id = fork();
        if(!id){
            execute("sleep 5");
        }
        else{
            pids->push_back(id);
        }
        //cout << "Pipes done" << endl;
        pipes->clear();

    }
    return 0;
}

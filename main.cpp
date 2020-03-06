#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <fstream>
#include <sys/fcntl.h>


using namespace std;

string* removeChars(string s, string chars) {
    //removes characters in chars from s that aren't in quotes
    string* removed = new string;
    bool inQuotes = false;
    for (int i = 0; i < s.length(); i++) {
        bool add = true;
        if(s.at(i) == '\"' | s.at(i) == '\''){
            inQuotes = !inQuotes;
        }
        for (int j = 0; j < chars.length(); j++) {
            if(chars.at(j) == s.at(i) && !inQuotes){
                add = false;
            }
        }
        if(add){
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
    vector<string> *seperated = split(line, " ");
    char **args = new char *[seperated->size() + 1];
    for (int i = 0; i < seperated->size(); i++) {
        string* trimed = removeChars(seperated->at(i), " \"\'"); //removes spaces and quotes
        args[i] = (char *) trimed->c_str();
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

void outputRedirection(string command, string file) {

    if(!fork()) {
        int output = open(file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        dup2(output, 1);
        close(output);
        execute(command);
        cerr << "ERROR execute failed" << endl;
        exit(0);
    }
    wait(0);
}

void inputRedirection(string command, string file) {
    if(!fork()) {
        int input = open((char *) file.c_str(), O_RDONLY);
        dup2(input, 0);
        close(input);
        execute(command);
        cerr << "ERROR execute failed" << endl;
        exit(0);
    }
    wait(0);
}

int main() {
    vector<string>* test = split("ls -a", " ");
    while (true) {
        cout << "My Shell$ ";
        //fflush(stdout);
        string inputline;
        getline(cin, inputline);
        //cout << "Input Line: " << inputline << endl;

        vector<string> *pipes = split(inputline, "|");
        if (pipes->size() > 1) {
            int standIn = dup(0);
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
                    wait(0);
                    dup2(fd[0], 0);
                    close(fd[1]);
                }
            }
            dup2(standIn, 0);
            close(standIn);
        } else {
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
        //cout << "Pipes done" << endl;
        pipes->clear();

    }
    return 0;
}

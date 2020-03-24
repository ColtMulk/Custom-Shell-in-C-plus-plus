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
            if (chars.at(j) == s.at(i) && (!inQuotes || chars.at(j) == '\'' || chars.at(j) == '\"')) {
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

int executeWRedirectAnd(string line, vector<int> *pids, bool pipeStart = false, bool pipeEnd = false) {
    //checks for redirection and for & to determine if to run in background or not
    vector<string> *seperated = split(line, " ");
    bool inputR = false, outputR = false, a = false;
    string input, output;
    //gets special arguments from line
    //doesn't allow certain arguments for pipe start or pipe end
    //creates command string that doesn't include the special arguments
    string command;
    for(int i = 0; i < seperated->size(); i++){

        if(seperated->at(i) == "<" && !pipeEnd){
            // input redirection
            inputR = true;

            //goes to argument for input
            i++;

            //checks if out of range of vector and exits if so
            if(seperated->size() <= i){
                cerr << "< needs an argument" << endl;
                return -1;
            }

            //gets input file
            input = seperated->at(i);
        }
        else if(seperated->at(i) == ">" && !pipeStart){
            //output redirection
            outputR = true;

            //goes to argument for output
            i++;

            //checks if out of range of vector and exits if so
            if(seperated->size() <= i){
                cerr << "> needs an argument" << endl;
                return -1;
            }

            //gets output file
            output = seperated->at(i);
        }
        else if(seperated->at(i) == "&" && !pipeStart && !pipeEnd){
            //Background Proccess
            a = true;
            seperated->erase(seperated->begin()+i);
            i--;
        }
        else{
            command += seperated->at(i) + " ";
        }
    }
    //removes extra space
    command = command.substr(0,command.length()-1);

    int id = fork();
    if(!id){
        //does input redirection
        if(inputR){
            int in = open((char*) input.c_str(), O_RDONLY);
            dup2(in, 0);
            close(in);
        }
        //does output redirection
        if(outputR){
            int out = open((char*) output.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(out, 1);
            close(out);
        }

        //executes command
        execute(command);
        cerr << "Command failed to execute" << endl;
        exit(1);
    }
    else{
        //handles & option
        if(!a){
            waitpid(id, nullptr, 0);
        }
        else{
            pids->push_back(id);
        }
    }
}

string changeDirectory(string command, string previousPath){
    vector<string>* seperated = split(command, " ");
    if(seperated->size() >= 2){
        string* temp = removeChars(seperated->at(1), "\"\'");
        if(*temp == "-"){
            return previousPath;
        }
        else if(chdir(temp->c_str()) != 0){
            cerr << "chdir() failed" << endl;
        }
    }
    else{
        cout << "Command cd needs arguments" << endl;
    }

    char* s = new char[1000];

    return getcwd(s,1000);
}

int main() {
    char s[1000];
    string path = getcwd(s,1000);
    string previousP = path;

    vector<int> *pids = new vector<int>;
    int *status = new int;
    while (true) {
        for (int i = 0; i < pids->size(); i++) {
            int id = waitpid(pids->at(i), status, WNOHANG);
            if(id == -1){
                cerr << "error occured" << endl;
            }
            else if(id == 0){
                //cerr << "Child " << i << " is still running" << endl;
            }
            else{
                //cerr << "Child " << i << " has exited" << endl;
                pids->erase(pids->begin() + i);
                i--;
            }
        }
        //cerr << "After pids" << endl;
        cout << "My Shell" << path << "$ ";
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

                    if(i == 0){
                        executeWRedirectAnd(pipes->at(i), pids, true);
                        exit(0);
                    }
                    else if(i == pipes->size()-1){
                        executeWRedirectAnd(pipes->at(i), pids, false, true);
                        exit(0);
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
            //cout << inputline.substr(0,2) << endl;
            if(inputline.substr(0,2) == "cd"){
                string temp = path;
                path = changeDirectory(inputline, previousP);
                //changes previousPath if path was changed
                if(temp != path) {
                    previousP = temp;
                }
            }
            else if(inputline.substr(0,4) == "jobs"){
                for (int i = 0; i < pids->size(); i++) {
                    cout << "PID: " << pids->at(i) << " is currently running" << endl;
                }
            }
            else{
                //executes line with special arguments
                executeWRedirectAnd(inputline, pids);
            }
        }

        /*int id = fork();
        if(!id){
            execute("sleep 5");
        }
        else{
            pids->push_back(id);
        }*/
        //cout << "Pipes done" << endl;
        pipes->clear();

    }
    return 0;
}

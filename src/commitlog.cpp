/*
    Copyright (C) 2009 Andrew Caudwell (acaudwell@gmail.com)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "commitlog.h"

//RCommitLog

RCommitLog::RCommitLog(std::string logfile, int firstChar) {

    logf     = 0;
    seekable = false;
    success  = false;
    is_dir   = false;
    buffered = false;

    if(logfile == "-") {

        //check first char
        if(checkFirstChar(firstChar, std::cin)) {
            logf     = new StreamLog();
            is_dir   = false;
            seekable = false;
            success  = true;
        }

        return;
    }

    struct stat fileinfo;
    int rc = stat(logfile.c_str(), &fileinfo);

    if(rc==0) {
        is_dir = (fileinfo.st_mode & S_IFDIR) ? true : false;

        if(!is_dir) {

            //check first char
            std::ifstream testf(logfile.c_str());

            bool firstOK = checkFirstChar(firstChar, testf);

            testf.close();

            if(firstOK) {
                logf = new SeekLog(logfile);
                seekable = true;
                success = true;
            }
        }
    }
}

RCommitLog::~RCommitLog() {
    if(logf!=0) delete logf;

    if(temp_file.size()) {
        remove(temp_file.c_str());
    }
}

//check firstChar of stream is as expected. if no firstChar defined just returns true.
bool RCommitLog::checkFirstChar(int firstChar, std::istream& stream) {

    //cant check this
    if(firstChar == -1) return true;

    int c = stream.peek();

    if(firstChar == c) return true;

    return false;
}

bool RCommitLog::checkFormat() {

    if(!success) return false;

    //read a commit to see if the log is in the correct format
    if(nextCommit(lastCommit)) {

        if(seekable) {
            //if the log is seekable, go back to the start
            ((SeekLog*)logf)->seekTo(0.0);
        } else {
            //otherwise set the buffered flag as we have bufferd one commit
            buffered = true;
        }

        return true;
    }

    return false;
}

std::string RCommitLog::getLogCommand() {
    return log_command;
}

bool RCommitLog::isSeekable() {
    return seekable;
}

bool RCommitLog::getCommitAt(float percent, RCommit& commit) {
    if(!seekable) return false;

    SeekLog* seeklog = ((SeekLog*)logf);

    //get the current pointer
    long currpointer = seeklog->getPointer();

    seekTo(percent);

    bool success = findNextCommit(commit,500);

    //set the pointer back
    seeklog->setPointer(currpointer);

    return success;
}

void RCommitLog::seekTo(float percent) {
    if(!seekable) return;

    ((SeekLog*)logf)->seekTo(percent);
}

float RCommitLog::getPercent() {
    if(seekable) return ((SeekLog*)logf)->getPercent();

    return 0.0;
}

bool RCommitLog::findNextCommit(RCommit& commit, int attempts) {

    for(int i=0;i<attempts;i++) {
        RCommit c;

        if(nextCommit(c)) {
            commit = c;
            return true;
        }
    }

    return false;
}

bool RCommitLog::nextCommit(RCommit& commit) {

    if(buffered) {
        commit = lastCommit;
        buffered = false;
        return true;
    }

    bool success = parseCommit(commit);

    if(!success) return false;

    return commit.isValid();
}

bool RCommitLog::isFinished() {
    if(logf->isFinished()) return true;

    return false;
}

// RCommitFile

RCommitFile::RCommitFile(std::string filename, std::string action, vec3f colour) {
    //prepend a root slash
    if(filename[0] != '/') filename = std::string("/") + filename;

    //replace non utf8 characters with ?
    std::string temp;
    utf8::replace_invalid(filename.begin(), filename.end(), back_inserter(temp), '?');
    filename = temp;

    this->filename = filename;
    this->action   = action;
    this->colour   = colour;
}

RCommit::RCommit() {
}

vec3f RCommit::fileColour(std::string filename) {

    size_t pos = filename.rfind('.');

    if(pos != std::string::npos && pos+1<filename.size()) {
        std::string file_ext = filename.substr(pos+1);

        return colourHash(file_ext);
    } else {
        return vec3f(1.0, 1.0, 1.0);
    }
}

void RCommit::addFile(std::string& filename, std::string& action) {
    files.push_back(RCommitFile(filename, action, fileColour(filename)));
}

void RCommit::addFile(std::string& filename, std::string& action, vec3f colour) {
    files.push_back(RCommitFile(filename, action, colour));
}

bool RCommit::isValid() {

    //replace non utf8 characters with ?
    std::string temp;
    utf8::replace_invalid(username.begin(), username.end(), back_inserter(temp), '?');
    username = temp;

    return true;
}


void RCommit::debug() {
    debugLog("files:\n");

    for(std::list<RCommitFile>::iterator it = files.begin(); it != files.end(); it++) {
        RCommitFile f = *it;
        debugLog("%s %s\n", f.action.c_str(), f.filename.c_str());
    }
}

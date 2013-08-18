/*
Copyright 2010-2011 Ed Bow <edxbow@gmail.com>

This file is part of Quake Live - Demo Tools (QLDT).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "Task.h"

DtTask::DtTask() {
    numThreads = QThread::idealThreadCount() + 1;
}

void DtTask::initialize( quint32 jCount ) {
    stopTask = false;
    jobCount_ = jCount;
    activeThreadsNum = 0;
    finishedJobsNum_ = 0;
    lastIndex = 0;
}

void DtTask::stop() {
    stopTask = true;
}

void DtTask::addActiveThreadsNum( int add ) {
    taskMutex.lock();
    activeThreadsNum += add;
    taskMutex.unlock();
}

void DtTask::addJob( int index ) {
    addActiveThreadsNum( 1 );
    lastIndex = index + 1;
}

void DtTask::setJobDone() {
    addActiveThreadsNum( -1 );
    ++finishedJobsNum_;
}

bool DtTask::threadLimit() {
    if ( jobCount_ == activeThreadsNum + finishedJobsNum_ ) {
        return true;
    }

    return activeThreadsNum == numThreads;
}

bool DtTask::done() {
    return stopTask || finishedJobsNum_ == jobCount_;
}

quint32 DtTask::finishedJobsNum() {
    return finishedJobsNum_;
}

quint32 DtTask::jobCount() {
    return jobCount_;
}


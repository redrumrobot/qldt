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

#ifndef DTTASK_H
#define DTTASK_H

#include <QtConcurrentRun>

class DtTask : public QObject {
    Q_OBJECT
public:
    DtTask();

    quint32 finishedJobsNum();
    quint32 jobCount();

    template < class Class, class Func >
    void run( quint32 jobs, Class* taskObj, Func taskFunc, void ( Class::*waitFunc )() );

    void setJobDone();

public slots:
    void stop();

protected:
    quint32 finishedJobsNum_;
    quint32 jobCount_;
    quint32 activeThreadsNum;
    quint32 numThreads;
    bool stopTask;
    quint32 lastIndex;
    QMutex taskMutex;

    void initialize( quint32 jCount );
    void addJob( int index );
    void addActiveThreadsNum( int add );
    bool threadLimit();
    bool done();
};

template < class Class, class Func >
void DtTask::run( quint32 jobs, Class* taskObj, Func taskFunc, void ( Class::*waitFunc )() ) {
    initialize( jobs );

    while ( !done() ) {
        if ( !threadLimit() ) {
            for ( quint32 i = lastIndex; i < jobCount_; ++i ) {
                addJob( i );
                QtConcurrent::run( taskObj, taskFunc, i );

                if ( threadLimit() ) {
                    break;
                }
            }
        }

        ( taskObj->*waitFunc )();
    }
}

#endif // DTASK_H

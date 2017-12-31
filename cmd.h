/**********************************************************************
 *  cmd.h
 **********************************************************************
 * Copyright (C) 2017 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://forum.mxlinux.org>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/


#ifndef CMD_H
#define CMD_H

#include <QProcess>
#include <QTimer>
#include <QTextStream>

#include "cmd_global.h"

class CMDSHARED_EXPORT Cmd: public QObject
{
    Q_OBJECT
public:
    explicit Cmd(QObject *parent = 0);
    ~Cmd();

    bool isRunning() const;
    int getExitCode() const;
    int run(const QString &cmd_str, int est_duration = 10); // with optional estimated time of completion (1sec default)
    QString getOutput() const;
    QString getOutput(const QString &cmd_str);

signals:
    void finished(int exit_code, QProcess::ExitStatus exit_status);
    void outputAvailable(const QString &output);
    void runTime(int, int); // runtime counter with estimated time in deciseconds
    void started();

public slots:
    bool kill();
    bool pause();
    bool resume();
    bool terminate();

private slots:
    void onStdoutAvailable();
    void tick(); // slot called by timer that emits a counter

private:
    int elapsed_time; // elapsed running time in deciseconds
    int est_duration; // estimated completion time in deciseconds
    QByteArray line_out;
    QString output;
    QTextStream buffer;
    QProcess *proc;
    QTimer *timer;

};

#endif // CMD_H

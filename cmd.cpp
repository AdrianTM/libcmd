/**********************************************************************
 *  cmd.cpp
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

#include <QEventLoop>
#include <QDebug>

#include "cmd.h"

Cmd::Cmd(QObject *parent) :
    QObject(parent), buffer(&output)
{
    proc = new QProcess(this);
    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &Cmd::tick);
    connect(proc, static_cast<void (QProcess::*)(int)>(&QProcess::finished), timer, &QTimer::stop);
    connect(proc, &QProcess::readyReadStandardOutput, this, &Cmd::onStdoutAvailable);
}

Cmd::~Cmd()
{
    disconnectFifo();
    if (this->isRunning()) {
        if(!this->terminate()) {
            this->kill();
        }
    }
}

// this function is running the command, takes cmd_str and optional estimated completion time
int Cmd::run(const QString &cmd_str, const QStringList &options, int est_duration)
{
    if (this->isRunning()) { // allow only one process at a time
        qDebug() << "process already running";
        return -1;
    }

    // reset variables if function is reused
    this->est_duration = est_duration;
    this->elapsed_time = 0;  // reset time counter
    this->output.clear();
    this->line_out.clear();

    proc->start("/bin/bash", QStringList() << "-c" << cmd_str);

    // start timer when started
    proc->waitForStarted();
    emit started();
    timer->start(100);

    QEventLoop loop;
    connect(proc, static_cast<void (QProcess::*)(int)>(&QProcess::finished), &loop, &QEventLoop::quit);
    QTextStream out(stdout);

    bool quiet = options.contains("quiet");
    if (!quiet) out << proc->arguments().at(1) << endl;

    loop.exec();

    // kill process if still running after loop finished
    if (this->isRunning()) {
        if(!this->terminate()) {
            this->kill();
        }
    }

    emit finished(proc->exitCode(), proc->exitStatus());
    return getExitCode(quiet);
}

// kill process, return true for success
bool Cmd::kill()
{
    if (!this->isRunning()) {
        return true; // returns true because process is not running
    }
    qDebug() << "killing parent process:" << proc->processId();
    proc->kill();
    proc->waitForFinished(1000);
    emit finished(proc->exitCode(), proc->exitStatus());
    return (!this->isRunning());
}

// terminate process, return true for success
bool Cmd::terminate()
{
    if (!this->isRunning()) {
        return true; // returns true because process is not running
    }
    qDebug() << "terminating parent process:" << proc->processId();
    proc->terminate();
    proc->waitForFinished(1000);
    emit finished(proc->exitCode(), proc->exitStatus());
    return (!this->isRunning());
}

void Cmd::writeToProc(const QString &str)
{
  if (!this->isRunning()) {
    return;
  }
  proc->write(str.toUtf8());
}

void Cmd::writeToFifo(const QString &str)
{
    if (!fifo.exists()) {
        qDebug() << "Fifo file" << fifo.fileName() << "could not be found";
        return;
    }
    file_watch.blockSignals(true);
    fifo.write(str.toUtf8() + "\n");
    fifo.flush();
    file_watch.blockSignals(false);
}

// emit a string signal when FIFO file changed
void Cmd::fifoChanged()
{
    QString out;
    fifo.seek(0);
    out = fifo.readAll().trimmed();
    if (!out.isEmpty()) {
        emit fifoChangeAvailable(out);
    }
}

// pause process
bool Cmd::pause()
{
    if (!this->isRunning()) {
        qDebug() << "process not running";
        return false;
    }
    QString id =  QString::number(proc->processId());
    qDebug() << "pausing process: " << id;
    timer->stop();
    return (system("kill -STOP " + id.toUtf8()) == 0);
}

// resume process
bool Cmd::resume()
{
    QString id =  QString::number(proc->processId());
    if (id == "0") {
        qDebug() << "process id not found";
        return false;
    }
    qDebug() << "resuming process:" << id;
    timer->start();
    return (system("kill -CONT " + id.toUtf8()) == 0);
}

// get the output of the command
QString Cmd::getOutput() const
{
    return output.trimmed();
}

// runs the command passed as argument and return output
QString Cmd::getOutput(const QString &cmd_str,  const QStringList &options, int est_duration)
{
    this->run(cmd_str, options, est_duration);
    return output.trimmed();
}

// on std out available emit the output
void Cmd::onStdoutAvailable()
{
    line_out = proc->readAllStandardOutput();
    if (line_out != "") {
        emit outputAvailable(line_out);
    }
    buffer << line_out;
}

// slot called by timer that emits a counter and the estimated duration to be used by progress bar
void Cmd::tick()
{
    emit runTime(++elapsed_time, est_duration);
}

// check if process is starting or running
bool Cmd::isRunning() const
{
    return (proc->state() != QProcess::NotRunning) ? true : false;
}

// set a Fifo file to be used for interprocess communication
bool Cmd::connectFifo(const QString &file_name)
{
    if (fifo.fileName() != file_name) {
        fifo.setFileName(file_name);
    }
    if (fifo.isOpen()) {
        return true;
    }
    if (fifo.open(QFile::ReadWrite)) {
        file_watch.addPath(file_name);
        connect(&file_watch, &QFileSystemWatcher::fileChanged, this, &Cmd::fifoChanged);
        return true;
    }
    return false;
}

void Cmd::disconnectFifo()
{
    if (fifo.isOpen()) {
        file_watch.disconnect();
        fifo.close();
    }
}

// get the exit code of the finished process
int Cmd::getExitCode(bool quiet) const
{
    if (proc->exitStatus() != 0) { // check first if process crashed, it might still return exit code = 0
        if (!quiet) qDebug() << "exit status:" << proc->exitStatus();
        return proc->exitStatus();
    } else {
        if (!quiet) qDebug() << "exit code:" << proc->exitCode();
        return proc->exitCode();
    }
}

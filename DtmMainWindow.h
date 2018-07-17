/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: ExitDTM
**
** Module: DtmMainWindow.h
**
** Notes:
**
** License: This program is free software: you can redistribute it and/or
**          modify it under the terms of the GNU General Public License as
**          published by the Free Software Foundation, version 3.
**
**          This program is distributed in the hope that it will be useful,
**          but WITHOUT ANY WARRANTY; without even the implied warranty of
**          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**          GNU General Public License for more details.
**
**          You should have received a copy of the GNU General Public License
**          along with this program.  If not, see http://www.gnu.org/licenses/
**
*******************************************************************************/
#ifndef DTMMAINWINDOW_H
#define DTMMAINWINDOW_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QRegularExpression>
#include <QMessageBox>
#include <QScrollBar>
#include <QDebug>
#include <QDesktopServices>

/******************************************************************************/
// Constants
/******************************************************************************/
//Constants for version and functions
const QString                  AppVersion                 = "1.0"; //Version string

//Constants for timeouts and streaming
const qint16                   ModuleTimeout              = 14000; //Time (in ms) until a part of the process is considered timed out

//Constants for program state
const quint8                   ProgramStatusIdle          = 0;
const quint8                   ProgramStatusExitDTM       = 1;
const quint8                   ProgramStatusEraseFS       = 2;
const quint8                   ProgramStatusLicenseCheck  = 3;
const quint8                   ProgramStatus              = 4;

//Command codes used to exit DTM mode and DTM options
const quint8                   DTMExitCMDA                = 0x3f;
const quint8                   DTMExitCMDB                = 0xff;
const QSerialPort::BaudRate    DTMBaudRate                = QSerialPort::Baud19200;
const QSerialPort::FlowControl DTMFlowControl             = QSerialPort::NoFlowControl;

//Exit code results
const int                      ExitCodeOK                 = 0;
const int                      ExitCodeInvalidPort        = -1;
const int                      ExitCodeCTSAsserted        = -2;
const int                      ExitCodeLicenseMissing     = -3;
const int                      ExitCodeTimeout            = -4;
const int                      ExitCodeSerialPortError    = -5;

//Server URL
const QString ServerHost = "uwterminalx.lairdtech.com";            //Hostname/IP of online server with help file

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class MainWindow;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
        QWidget *parent = 0
        );
    ~MainWindow(
        );
    void
    WindowSetup(
        );

public slots:
    void
    SerialRead(
        );
    void
    SerialStatusSlot(
        );
    void
    SerialError(
        QSerialPort::SerialPortError speErrorCode
        );
    void
    SerialBytesWritten(
        qint64 intByteCount
        );

private slots:
    void
    on_btn_Connect_clicked(
        );
    void
    on_btn_Refresh_clicked(
        );
    void
    on_combo_COM_currentIndexChanged(
        int intIndex
        );
    void
    on_btn_Licenses_clicked(
        );
    void
    on_btn_TermClear_clicked(
        );
    void
    SystemTimeout(
        );
    void
    ForceClose(
        );
#ifdef TARGET_OS_MAC
    void
    ContinueOperationMacDoesntSupportCTSWorkaroundFunction(
        );
#endif
    void
    on_btn_Cancel_clicked(
        );
    void
    on_btn_Help_clicked(
        );

private:
    Ui::MainWindow *ui;
    void
    RefreshSerialDevices(
        );
    void
    DoLineEnd(
        );
    void
    SerialStatus(
        bool bType
        );
    void
    OpenDevice(
        QSerialPort::BaudRate spbBaud,
        QSerialPort::FlowControl spfFlow
        );
    void
    UpdateImages(
        );
    void
    TermClose(
        );

    //Private variables
    QSerialPort gspSerialPort; //Contains the handle for the serial port
    quint16 gintRXBytes; //Number of RX bytes
    quint16 gintTXBytes; //Number of TX bytes
    unsigned char gchTermBusyLines; //Number of commands recieved
    QString gstrTermBusyData; //Holds the recieved data for checking
    QTimer *gpSignalTimer; //Handle for a timer to update COM port signals
    QTimer *gpSystemTimeout; //Timer used to check if the process has timed out
    bool gbCTSStatus; //True when CTS is asserted
    QByteArray gbaDisplayBuffer; //Buffer of data to display
    quint8 gintProgramState; //Current position of the state machine
    bool gbExitOnFinish; //If the application should exit when complete or continue to run
    QTimer *gpExitTimer; //Timer used to exit appliction in some instnces
#ifdef TARGET_OS_MAC
    QTimer *gpMacDoesntSupportCTSWorkaroundTimer; //A timer used to work around mac not having any working CTS read/update code
#endif
    int gintExitCode; //Exit code when program exists using above timer
    bool gbShowSerialErrors; //Used to supress serial port errors during opening
};

#endif // DTMMAINWINDOW_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/

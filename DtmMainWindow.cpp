/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: ExitDTM
**
** Module: DtmMainWindow.cpp
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

/******************************************************************************/
// Include Files
/******************************************************************************/
#include "DtmMainWindow.h"
#include "ui_DtmMainWindow.h"

/******************************************************************************/
// Local Functions or Private Members
/******************************************************************************/
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    //Setup the GUI
    ui->setupUi(this);

    //Define default variable values
    gintRXBytes = 0;
    gintTXBytes = 0;
    gchTermBusyLines = 0;
    gbCTSStatus = 0;
    gintProgramState = ProgramStatusIdle;

    //Clear display buffer
    gbaDisplayBuffer.clear();

    //Move to 'About' tab
    ui->selector_Tab->setCurrentIndex(ui->selector_Tab->indexOf(ui->tab_Config));

    //Connect quit signals
    connect(ui->btn_Quit, SIGNAL(clicked()), this, SLOT(close()));

    //Populate the list of devices
    RefreshSerialDevices();

    //Set title
    setWindowTitle(QString("ExitDTM (v").append(AppVersion).append(")"));

    //Configure the signal and program advancement timer
    gpSignalTimer = new QTimer(this);
    connect(gpSignalTimer, SIGNAL(timeout()), this, SLOT(SerialStatusSlot()));

    //Configure the program timeout timer
    gpSystemTimeout = new QTimer(this);
    connect(gpSystemTimeout, SIGNAL(timeout()), this, SLOT(SystemTimeout()));

    //Configure the exit timer
    gpExitTimer = new QTimer(this);
    gpExitTimer->setSingleShot(true);
    gpExitTimer->setInterval(10);
    connect(gpExitTimer, SIGNAL(timeout()), this, SLOT(ForceClose()));

#ifdef TARGET_OS_MAC
    //Because mac just shows CTS as always being asserted which is clearly wrong
    gpMacDoesntSupportCTSWorkaroundTimer = new QTimer(this);
    gpMacDoesntSupportCTSWorkaroundTimer->setSingleShot(true);
    gpMacDoesntSupportCTSWorkaroundTimer->setInterval(350);
    connect(gpMacDoesntSupportCTSWorkaroundTimer, SIGNAL(timeout()), this, SLOT(ContinueOperationMacDoesntSupportCTSWorkaroundFunction()));
#endif

    //Connect serial signals
    connect(&gspSerialPort, SIGNAL(readyRead()), this, SLOT(SerialRead()));
    connect(&gspSerialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(SerialError(QSerialPort::SerialPortError)));
    connect(&gspSerialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(SerialBytesWritten(qint64)));

    //Change terminal font to a monospaced font
#pragma warning("TODO: Revert manual font selection when QTBUG-54623 is fixed")
#ifdef _WIN32
    QFont fntTmpFnt2 = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#elif __APPLE__
    QFont fntTmpFnt2 = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#else
    //Fix for qt bug
    QFont fntTmpFnt2 = QFont("monospace");
#endif
    QFontMetrics tmTmpFM(fntTmpFnt2);
    ui->text_TermEditData->setFont(fntTmpFnt2);
    ui->text_TermEditData->setTabStopWidth(tmTmpFM.width(" ")*6);

    //Currently not in a state
    gintProgramState = ProgramStatusIdle;

    //Check command line
    QStringList slArgs = QCoreApplication::arguments();
    unsigned char chi = 1;
    bool bArgCom = false;
    bool bArgNoRecovery = false;
    bool bArgShowWindow = true;
    gbExitOnFinish = false;
    while (chi < slArgs.length())
    {
        if (slArgs[chi].left(4).toUpper() == "COM=")
        {
            //Set com port
            QString strPort = slArgs[chi].right(slArgs[chi].length()-4);
#ifdef _WIN32
            if (strPort.left(3) != "COM")
            {
                //Prepend COM for UwTerminal shortcut compatibility
                strPort.prepend("COM");
            }
#endif
            ui->combo_COM->setCurrentText(strPort);
            bArgCom = true;

            //Update serial port info
            on_combo_COM_currentIndexChanged(0);
        }
/*        else if (slArgs[chi].left(5).toUpper() == "BAUD=")
        {
            //Set baud rate
            ui->combo_Baud->setCurrentText(slArgs[chi].right(slArgs[chi].length()-5));
        }
        else if (slArgs[chi].left(5).toUpper() == "FLOW=")
        {
            //Set flow control
            if (slArgs[chi].right(1).toInt() >= 0 && slArgs[chi].right(1).toInt() < 3)
            {
                //Valid
                ui->combo_Handshake->setCurrentIndex(slArgs[chi].right(1).toInt());
            }
        }*/
        else if (slArgs[chi].toUpper() == "NOLICENSE")
        {
            //Skip license checking
            ui->check_License->setChecked(false);
        }
        else if (slArgs[chi].toUpper() == "NORECOVERY")
        {
            //Connect to device at startup
            bArgNoRecovery = true;
        }
        else if (slArgs[chi].toUpper() == "AUTOEXIT")
        {
            //Automatically close application once complete
            gbExitOnFinish = true;
        }
        else if (slArgs[chi].toUpper() == "NOWINDOW")
        {
            //Hide window
            if (gbExitOnFinish == true && bArgCom == true && bArgNoRecovery == false)
            {
                //Hide the window
                bArgShowWindow = false;
            }
        }
        ++chi;
    }

    if (bArgShowWindow == true)
    {
        //Show the window
        this->show();
    }

    if (bArgCom == true && bArgNoRecovery == false)
    {
        //Enough information to connect!
        OpenDevice(DTMBaudRate, DTMFlowControl);
    }
}

//=============================================================================
//=============================================================================
MainWindow::~MainWindow(){
    //Disconnect all signals
    disconnect(this, SLOT(close()));
    disconnect(this, SLOT(SerialStatusSlot()));
    disconnect(this, SLOT(SerialRead()));
    disconnect(this, SLOT(SerialError(QSerialPort::SerialPortError)));
    disconnect(this, SLOT(SerialBytesWritten(qint64)));
    disconnect(this, SLOT(SystemTimeout()));
    disconnect(this, SLOT(ForceClose()));
#ifdef TARGET_OS_MAC
    disconnect(this, SLOT(ContinueOperationMacDoesntSupportCTSWorkaroundFunction()));
#endif

    if (gspSerialPort.isOpen() == true)
    {
        //Close serial connection before quitting
        gspSerialPort.close();
        gpSignalTimer->stop();
    }

    if (gpSystemTimeout->isActive())
    {
        //Stop the timeout timer
        gpSystemTimeout->stop();
    }

    //Delete variables
    delete gpSignalTimer;
    delete gpSystemTimeout;
    delete gpExitTimer;
#ifdef TARGET_OS_MAC
    delete gpMacDoesntSupportCTSWorkaroundTimer;
#endif
    delete ui;
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Connect_clicked(
    )
{
    //Connect to COM port button clicked.
    if (gintProgramState == ProgramStatusIdle)
    {
        //Not currently busy
        OpenDevice(DTMBaudRate, DTMFlowControl);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::TermClose(
    )
{
    //Close, but first clear up from download/streaming
    gintProgramState = ProgramStatusIdle;
    gpSystemTimeout->stop();

    //Close the serial port
    while (gspSerialPort.isOpen() == true)
    {
        gspSerialPort.clear();
        gspSerialPort.close();
    }
    gpSignalTimer->stop();

    //Change status message
    ui->statusBar->showMessage("");
    ui->label_TermConn->setText("[Port not open]");

    //Update images
    UpdateImages();

    //Disable button
    ui->btn_Cancel->setEnabled(false);

    //Enable button
    ui->btn_Connect->setEnabled(true);
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Refresh_clicked(
    )
{
    //Refresh the list of serial ports
    RefreshSerialDevices();
}

//=============================================================================
//=============================================================================
void
MainWindow::RefreshSerialDevices(
    )
{
    //Clears and refreshes the list of serial devices
    QString strPrev = "";
    QRegularExpression reTempRE("^(\\D*?)(\\d+)$");
    QList<int> lstEntries;
    lstEntries.clear();

    if (ui->combo_COM->count() > 0)
    {
        //Remember previous option
        strPrev = ui->combo_COM->currentText();
    }
    ui->combo_COM->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QRegularExpressionMatch remTempREM = reTempRE.match(info.portName());
        if (remTempREM.hasMatch() == true)
        {
            //Can sort this item
            int i = lstEntries.count()-1;
            while (i >= 0)
            {
                if (remTempREM.captured(2).toInt() > lstEntries[i])
                {
                    //Found correct order position, add here
                    ui->combo_COM->insertItem(i+1, info.portName());
                    lstEntries.insert(i+1, remTempREM.captured(2).toInt());
                    i = -1;
                }
                --i;
            }

            if (i == -1)
            {
                //Position not found, add to beginning
                ui->combo_COM->insertItem(0, info.portName());
                lstEntries.insert(0, remTempREM.captured(2).toInt());
            }
        }
        else
        {
            //Cannot sort this item
            ui->combo_COM->insertItem(ui->combo_COM->count(), info.portName());
        }
    }

    //Search for previous item if one was selected
    if (strPrev == "")
    {
        //Select first item
        ui->combo_COM->setCurrentIndex(0);
    }
    else
    {
        //Search for previous
        int i = 0;
        while (i < ui->combo_COM->count())
        {
            if (ui->combo_COM->itemText(i) == strPrev)
            {
                //Found previous item
                ui->combo_COM->setCurrentIndex(i);
                break;
            }
            ++i;
        }
    }

    //Update serial port info
    on_combo_COM_currentIndexChanged(0);
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialRead(
    )
{
    //Read the data into a buffer and copy it to edit for the display data
    QByteArray baOrigData = gspSerialPort.readAll();

    //Update the display with the data
    QByteArray baDispData = baOrigData;

    //Replace unprintable characters
    baDispData.replace('\0', "\\00").replace("\x01", "\\01").replace("\x02", "\\02").replace("\x03", "\\03").replace("\x04", "\\04").replace("\x05", "\\05").replace("\x06", "\\06").replace("\x07", "\\07").replace("\x08", "\\08").replace("\x0b", "\\0B").replace("\x0c", "\\0C").replace("\x0e", "\\0E").replace("\x0f", "\\0F").replace("\x10", "\\10").replace("\x11", "\\11").replace("\x12", "\\12").replace("\x13", "\\13").replace("\x14", "\\14").replace("\x15", "\\15").replace("\x16", "\\16").replace("\x17", "\\17").replace("\x18", "\\18").replace("\x19", "\\19").replace("\x1a", "\\1a").replace("\x1b", "\\1b").replace("\x1c", "\\1c").replace("\x1d", "\\1d").replace("\x1e", "\\1e").replace("\x1f", "\\1f");

    //Update display buffer
    gbaDisplayBuffer.append("> ");
    gbaDisplayBuffer.append(QString(baDispData).replace("\r", "").replace("\n", ""));
    gbaDisplayBuffer.append("\r\n");
    ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
    ui->text_TermEditData->verticalScrollBar()->setValue(ui->text_TermEditData->verticalScrollBar()->maximum());

    //Update number of recieved bytes
    gintRXBytes = gintRXBytes + baOrigData.length();
    ui->label_TermRx->setText(QString::number(gintRXBytes));

    if (gintProgramState != ProgramStatusIdle)
    {
        //Currently waiting for a response
        gstrTermBusyData = gstrTermBusyData.append(baOrigData);
        gchTermBusyLines = gchTermBusyLines + baOrigData.count("\n");

        if (gintProgramState == ProgramStatusEraseFS && gchTermBusyLines >= 2)
        {
            //Check that module filesystem has been erased
            if (gstrTermBusyData.indexOf("\nFFS Erased, Rebooting...") != -1 && gstrTermBusyData.indexOf("\n00\r", gstrTermBusyData.indexOf("\nFFS Erased, Rebooting...") + 1) != -1)
            {
                //Module has been erased - no longer in DTM mode
                gstrTermBusyData.clear();
                gchTermBusyLines = 0;
                if (ui->check_License->isChecked())
                {
                    //Optional license check
                    gintProgramState = ProgramStatusLicenseCheck;

                    //Check the license and BT address
                    gspSerialPort.write("at i 4");
                    DoLineEnd();
                    gbaDisplayBuffer.append("< at i 4\n");
                    gspSerialPort.write("at i 14");
                    DoLineEnd();
                    gbaDisplayBuffer.append("< at i 14\n");
                    ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
                    ui->text_TermEditData->verticalScrollBar()->setValue(ui->text_TermEditData->verticalScrollBar()->maximum());
                }
                else
                {
                    //No license check, finished
                    ui->text_TermEditData->appendPlainText("License check not performed.");

                    //Clean up
                    gstrTermBusyData.clear();
                    gchTermBusyLines = 0;
                    gintProgramState = ProgramStatusIdle;
                    gpSystemTimeout->stop();
                    gbaDisplayBuffer.append("\r\n\r\n ~ DTM escape complete ~ \r\n");
                    ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
                    ui->text_TermEditData->verticalScrollBar()->setValue(ui->text_TermEditData->verticalScrollBar()->maximum());

                    //Close port
                    TermClose();

                    //Show result
                    if (gbExitOnFinish == true)
                    {
                        //License code is present and module is out of DTM
                        QApplication::exit(ExitCodeOK);
                    }
                    else
                    {
                        //Show result on message box
                        QMessageBox::information(this, "Exit DTM mode result", "Escape from DTM mode complete, you can now communicate with the module as required.\r\n\r\nYour module's license has been unchecked, and is ready for use.\r\n", QMessageBox::Close);
                    }
                }
            }
        }
        else if (gintProgramState == ProgramStatusLicenseCheck && gchTermBusyLines == 4)
        {
            QRegularExpression reTempLicRE("\n10\t4\t00 ([a-zA-Z0-9]{12})\r\n00\r");
            QRegularExpressionMatch remTempLicREM = reTempLicRE.match(gstrTermBusyData);
            QString strResultData = "Escape from DTM mode complete, you can now communicate with the module as required.\r\n\r\n";
            bool bLicenseValid = false;
            if (remTempLicREM.hasMatch() == true && remTempLicREM.captured(1).toUpper() == "0016A4C0FFEE")
            {
                //Invalid license detected
                QRegularExpression reTempAddrRE("\n10\t14\t(00|01|02|03|04) ([a-zA-Z0-9]{12})\r\n00\r");
                QRegularExpressionMatch remTempAddrREM = reTempAddrRE.match(gstrTermBusyData);
                strResultData.append("Your module does not have a valid license, you will need to send the response to the command 'at i 14' to Laird support for them to generate you a license.\r\n");
                if (remTempAddrREM.hasMatch() == true)
                {
                    //We have an address to return
                    strResultData.append("\r\nAT I 14 response from this module: ").append(remTempAddrREM.captured(2)).append(".\r\n");
                }
                ui->text_TermEditData->appendPlainText("License check: bad key.");
            }
            else if (remTempLicREM.hasMatch() == true && remTempLicREM.captured(1).toUpper() == "0016A4C0FFEE")
            {
                //Valid license detected
                strResultData.append("Your module has a valid license (").append(remTempLicREM.captured(1).toUpper()).append(") and is ready for use.\r\n");
                ui->text_TermEditData->appendPlainText("License check: good key.");
                bLicenseValid = true;
            }

            //Clean up
            gstrTermBusyData.clear();
            gchTermBusyLines = 0;
            gintProgramState = ProgramStatusIdle;
            gpSystemTimeout->stop();
            gbaDisplayBuffer.append("\r\n\r\n ~ DTM escape complete ~ \r\n");
            ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
            ui->text_TermEditData->verticalScrollBar()->setValue(ui->text_TermEditData->verticalScrollBar()->maximum());

            //Close port
            TermClose();

            //Show result
            if (gbExitOnFinish == true)
            {
                //Exit application
                if (bLicenseValid == true)
                {
                    //License code is present and module is out of DTM
                    QApplication::exit(ExitCodeOK);
                }
                else
                {
                    //License code is missing but module is out of DTM
                    QApplication::exit(ExitCodeLicenseMissing);
                }
            }
            else
            {
                //Show result on message box
                QMessageBox::information(this, "Exit DTM mode result", strResultData, QMessageBox::Close);
            }
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateImages(
    )
{
    //Updates images to reflect status
    if (gspSerialPort.isOpen() == true)
    {
        //Port open
        SerialStatus(true);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::DoLineEnd(
    )
{
    //Outputs a line ending - CR
    gspSerialPort.write("\r");
    return;
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialStatus(
    bool bType
    )
{
    if (gspSerialPort.isOpen() == true)
    {
        unsigned int intSignals = gspSerialPort.pinoutSignals();
        if ((((intSignals & QSerialPort::ClearToSendSignal) == QSerialPort::ClearToSendSignal ? 1 : 0) != gbCTSStatus || bType == true))
        {
            //CTS changed
            gbCTSStatus = ((intSignals & QSerialPort::ClearToSendSignal) == QSerialPort::ClearToSendSignal ? 1 : 0);
        }

        if (gintProgramState > ProgramStatusIdle)
        {
            //We are awaiting for something to happen
            if (gintProgramState == ProgramStatusExitDTM)
            {
                //Waiting for module to exit DTM and return to interactive mode
                if (gbCTSStatus == 1)
                {
                    //Module has reset in normal mode, let's re-open the UART at the normal settings
                    gintProgramState = ProgramStatusEraseFS;
                    OpenDevice((QSerialPort::BaudRate)ui->combo_Baud->currentText().toUInt(), (ui->combo_Handshake->currentIndex() == 2 ? QSerialPort::SoftwareControl : (ui->combo_Handshake->currentIndex() == 1 ? QSerialPort::HardwareControl : QSerialPort::NoFlowControl)));

                    //Send the clear configuration command
                    DoLineEnd(); //In case module was not in DTM and has received garbage command
                    gspSerialPort.write("at&f*");
                    DoLineEnd();
                    gbaDisplayBuffer.append("< at&f*\n");
                    ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
                    ui->text_TermEditData->verticalScrollBar()->setValue(ui->text_TermEditData->verticalScrollBar()->maximum());
                }
            }
        }
    }
    else
    {
        //Port isn't open, disable timer
        gpSignalTimer->stop();
    }
    return;
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialStatusSlot(
    )
{
    //Slot function to update serial pinout status
    SerialStatus(0);
}

//=============================================================================
//=============================================================================
void
MainWindow::OpenDevice(
    QSerialPort::BaudRate spbBaud,
    QSerialPort::FlowControl spfFlow
    )
{
    //Function to open serial port
    if (gspSerialPort.isOpen() == true)
    {
        //Close serial port
        while (gspSerialPort.isOpen() == true)
        {
            gspSerialPort.clear();
            gspSerialPort.close();
        }
        gpSignalTimer->stop();

        //Change status message
        ui->statusBar->showMessage("");

        //Update images
        UpdateImages();
    }

    if (ui->combo_COM->currentText().length() > 0)
    {
        //Port selected: setup serial port
        gspSerialPort.setPortName(ui->combo_COM->currentText());
        gspSerialPort.setBaudRate(spbBaud);
        gspSerialPort.setDataBits(QSerialPort::Data8);
        gspSerialPort.setStopBits(QSerialPort::OneStop);
        gspSerialPort.setParity(QSerialPort::NoParity);
        gspSerialPort.setFlowControl(spfFlow);

        //Disable showing errors until open was successful
        gbShowSerialErrors = false;

        if (gspSerialPort.open(QIODevice::ReadWrite))
        {
            //Successful
            ui->statusBar->showMessage(QString("[").append(ui->combo_COM->currentText()).append(":").append(ui->combo_Baud->currentText()).append(",").append((ui->combo_Handshake->currentIndex() == 0 ? "N" : ui->combo_Handshake->currentIndex() == 1 ? "H" : ui->combo_Handshake->currentIndex() == 2 ? "S" : "")).append("]{").append("cr").append("}"));
            ui->label_TermConn->setText(ui->statusBar->currentMessage());

            //Show serial errors
            gbShowSerialErrors = true;

            //Disable button
            ui->btn_Connect->setEnabled(false);

            //Enable button
            ui->btn_Cancel->setEnabled(true);

            //Signal checking
            SerialStatus(1);

            if (spbBaud == DTMBaudRate && spfFlow == DTMFlowControl)
            {
                //First stage of program
                gintProgramState = ProgramStatusExitDTM;
                gpSystemTimeout->start(ModuleTimeout);
            }

#ifndef TARGET_OS_MAC
            //2018 and mac doesn't support CTS/RTS, why even be surprised...
            if (gintProgramState == ProgramStatusExitDTM && gbCTSStatus == 1)
            {
                //CTS not deasserted as expected. Close port
                TermClose();

                if (gbExitOnFinish == true)
                {
                    //Exit with error code
                    gintExitCode = ExitCodeCTSAsserted;
                    QApplication::exit(ExitCodeCTSAsserted);
                    gpExitTimer->start();
                    return;
                }
                else
                {
                    //Show error
                    QMessageBox::warning(this, "Error: CTS is asserted", "CTS should not be asserted whilst in DTM mode, aborting...", QMessageBox::Ok);
                    return;
                }
            }
#endif

            //Start signal timer
            gpSignalTimer->start(100);

            //Change to terminal tab
            ui->selector_Tab->setCurrentIndex(ui->selector_Tab->indexOf(ui->tab_Disp));

            //Set focus to input text edit
            ui->text_TermEditData->setFocus();

            if (gintProgramState == ProgramStatusExitDTM)
            {
                //Generate the exit DTM command
                QByteArray baExitDTM;
                baExitDTM.append(DTMExitCMDA);
                baExitDTM.append(DTMExitCMDB);

                //Send the exit DTM command
                gspSerialPort.write(baExitDTM);
                if (gbaDisplayBuffer.count() > 0)
                {
                    //Line break
                    gbaDisplayBuffer.append("-------------------------\r\n\r\n");
                }
                gbaDisplayBuffer.append("< \\3F\\FF\n");
                ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
                ui->text_TermEditData->verticalScrollBar()->setValue(ui->text_TermEditData->verticalScrollBar()->maximum());

#ifdef TARGET_OS_MAC
                //Workaround for mac
                gpMacDoesntSupportCTSWorkaroundTimer->start();
#endif
            }
        }
        else
        {
            //Error whilst opening
            ui->statusBar->showMessage("Error: ");
            ui->statusBar->showMessage(ui->statusBar->currentMessage().append(gspSerialPort.errorString()));
            QString strMessage = QString("Error whilst attempting to open the serial device: ").append(gspSerialPort.errorString()).append("\n\nIf the serial port is open in another application, please close the other application")
#if !defined(_WIN32) && !defined( __APPLE__)
            .append(", please also ensure you have been granted permission to the serial device in /dev/")
#endif
            .append((ui->combo_Baud->currentText().toULong() > 115200 ? ", please also ensure that your serial device supports baud rates greater than 115200 (normal COM ports do not have support for these baud rates)" : ""))
            .append(" and try again.");

            if (gbExitOnFinish == true)
            {
                //Close application with error
                QApplication::exit(ExitCodeInvalidPort);
            }
            else
            {
                //Show error message
                QMessageBox::warning(this, "Error opening port", strMessage, QMessageBox::Ok);
            }
        }
    }
    else
    {
        //No serial port selected
        if (gbExitOnFinish == true)
        {
            //Close application with error
            QApplication::exit(ExitCodeInvalidPort);
        }
        else
        {
            //Show error message
            QMessageBox::warning(this, "Error opening port", "No serial port was selected, please select a serial port and try again.\r\nIf you see no serial ports listed, ensure your device is connected to your computer and you have the appropriate drivers installed.", QMessageBox::Ok);
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialError(
    QSerialPort::SerialPortError speErrorCode
    )
{
    if (speErrorCode == QSerialPort::NoError)
    {
        //No error. Why this is ever emitted is a mystery to me.
        return;
    }
    else if ((speErrorCode == QSerialPort::ResourceError || speErrorCode == QSerialPort::PermissionError) && gbShowSerialErrors == true)
    {
        //Resource error or permission error (device unplugged?)
        if (gspSerialPort.isOpen() == true)
        {
            //Close active connection
            gspSerialPort.close();
        }

        //Change status message
        ui->statusBar->showMessage("");
        ui->label_TermConn->setText("[Port not open]");

        //Update images
        UpdateImages();

        if (gbExitOnFinish == true)
        {
            //Exit application
            QApplication::exit(ExitCodeSerialPortError);
        }
        else
        {
            //Set back to idle
            gintProgramState = ProgramStatusIdle;
            gpSystemTimeout->stop();
            gpSignalTimer->stop();

#ifdef TARGET_OS_MAC
        gpMacDoesntSupportCTSWorkaroundTimer->stop();
#endif

            //Change status message
            ui->statusBar->showMessage("Fatal error with serial connection");
            ui->label_TermConn->setText("[Port not open]");

            //Update images
            UpdateImages();

            //Disable button
            ui->btn_Cancel->setEnabled(false);

            //Enable button
            ui->btn_Connect->setEnabled(true);

            //Show error
            QMessageBox::warning(this, "Error during DTM escape", "Fatal error with serial connection.\nPlease reconnect to the device to continue.", QMessageBox::Ok);
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::SerialBytesWritten(
    qint64 intByteCount
    )
{
    //Updates the display with the number of bytes written
    gintTXBytes += intByteCount;
    ui->label_TermTx->setText(QString::number(gintTXBytes));
}

//=============================================================================
//=============================================================================
void
MainWindow::on_combo_COM_currentIndexChanged(
    int
    )
{
    //Serial port selection has been changed, update text
    if (ui->combo_COM->currentText().length() > 0)
    {
        QSerialPortInfo spiSerialInfo(ui->combo_COM->currentText());
        if (spiSerialInfo.isValid())
        {
            //Port exists
            QString strDisplayText(spiSerialInfo.description());
            if (spiSerialInfo.manufacturer().length() > 1)
            {
                //Add manufacturer
                strDisplayText.append(" (").append(spiSerialInfo.manufacturer()).append(")");
            }
            if (spiSerialInfo.serialNumber().length() > 1)
            {
                //Add serial
                strDisplayText.append(" [").append(spiSerialInfo.serialNumber()).append("]");
            }
            ui->label_SerialInfo->setText(strDisplayText);
        }
        else
        {
            //No such port
            ui->label_SerialInfo->setText("Invalid serial port selected");
        }
    }
    else
    {
        //Clear text as no port is selected
        ui->label_SerialInfo->clear();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Licenses_clicked(
    )
{
    //Show license text
    QString strMessage = QString("ExitDTM uses the Qt framework version 5, which is licensed under the GPLv3 (not including later versions).\nExitDTM uses and may be linked statically to various other libraries including Xau, XCB, expat, fontconfig, zlib, bz2, harfbuzz, freetype, udev, dbus, icu, unicode, UPX The licenses for these libraries are provided below:\n\n\n\nLib Xau:\n\nCopyright 1988, 1993, 1994, 1998  The Open Group\n\nPermission to use, copy, modify, distribute, and sell this software and its\ndocumentation for any purpose is hereby granted without fee, provided that\nthe above copyright notice appear in all copies and that both that\ncopyright notice and this permission notice appear in supporting\ndocumentation.\nThe above copyright notice and this permission notice shall be included in\nall copies or substantial portions of the Software.\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\nIMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\nFITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE\nOPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN\nAN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\nCONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\nExcept as contained in this notice, the name of The Open Group shall not be\nused in advertising or otherwise to promote the sale, use or other dealings\nin this Software without prior written authorization from The Open Group.\n\n\n\nxcb:\n\nCopyright (C) 2001-2006 Bart Massey, Jamey Sharp, and Josh Triplett.\nAll Rights Reserved.\n\nPermission is hereby granted, free of charge, to any person\nobtaining a copy of this software and associated\ndocumentation files (the 'Software'), to deal in the\nSoftware without restriction, including without limitation\nthe rights to use, copy, modify, merge, publish, distribute,\nsublicense, and/or sell copies of the Software, and to\npermit persons to whom the Software is furnished to do so,\nsubject to the following conditions:\n\nThe above copyright notice and this permission notice shall\nbe included in all copies or substantial portions of the\nSoftware.\n\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY\nKIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE\nWARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR\nPURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS\nBE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER\nIN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\nOUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\nOTHER DEALINGS IN THE SOFTWARE.\n\nExcept as contained in this notice, the names of the authors\nor their institutions shall not be used in advertising or\notherwise to promote the sale, use or other dealings in this\nSoftware without prior written authorization from the\nauthors.\n\n\n\nexpat:\n\nCopyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd\n   and Clark Cooper\nCopyright (c) 2001, 2002, 2003, 2004, 2005, 2006 Expat maintainers.\nPermission is hereby granted, free of charge, to any person obtaining\na copy of this software and associated documentation files (the\n'Software'), to deal in the Software without restriction, including\nwithout limitation the rights to use, copy, modify, merge, publish,\ndistribute, sublicense, and/or sell copies of the Software, and to\npermit persons to whom the Software is furnished to do so, subject to\nthe following conditions:\n\nThe above copyright notice and this permission notice shall be included\nin all copies or substantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,\nEXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\nMERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\nIN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY\nCLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\nTORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE\nSOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\n\n\nfontconfig:\n\nCopyright © 2001,2003 Keith Packard\n\nPermission to use, copy, modify, distribute, and sell this software and its\ndocumentation for any purpose is hereby granted without fee, provided that\nthe above copyright notice appear in all copies and that both that\ncopyright notice and this permission notice appear in supporting\ndocumentation, and that the name of Keith Packard not be used in\nadvertising or publicity pertaining to distribution of the software without\nspecific, written prior permission.  Keith Packard makes no\nrepresentations about the suitability of this software for any purpose.  It\nis provided 'as is' without express or implied warranty.\n\nKEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,\nINCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO\nEVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR\nCONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,\nDATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\nTORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\nPERFORMANCE OF THIS SOFTWARE.\n\nz:\n\n (C) 1995-2013 Jean-loup Gailly and Mark Adler\n\n  This software is provided 'as-is', without any express or implied\n  warranty.  In no event will the authors be held liable for any damages\n  arising from the use of this software.\n\n  Permission is granted to anyone to use this software for any purpose,\n  including commercial applications, and to alter it and redistribute it\n  freely, subject to the following restrictions:\n\n  1. The origin of this software must not be misrepresented; you must not\n     claim that you wrote the original software. If you use this software\n     in a product, an acknowledgment in the product documentation would be\n     appreciated but is not required.\n  2. Altered source versions must be plainly marked as such, and must not be\nmisrepresented as being the original software.\n  3. This notice may not be removed or altered from any source distribution.\n\n  Jean-loup Gailly        Mark Adler\n  jloup@gzip.org          madler@alumni.caltech.edu\n\n\n\nbz2:\n\n\nThis program, 'bzip2', the associated library 'libbzip2', and all\ndocumentation, are copyright (C) 1996-2010 Julian R Seward.  All\nrights reserved.\n\nRedistribution and use in source and binary forms, with or without\nmodification, are permitted provided that the following conditions\nare met:\n\n1. Redistributions of source code must retain the above copyright\n   notice, this list of conditions and the following disclaimer.\n\n2. The origin of this software must not be misrepresented; you must\n   not claim that you wrote the original software.  If you use this\n   software in a product, an acknowledgment in the product\n   documentation would be appreciated but is not required.\n\n3. Altered source versions must be plainly marked as such, and must\n   not be misrepresented as being the original software.\n\n4. The name of the author may not be used to endorse or promote\n   products derived from this software without specific prior written\n   permission.\n\nTHIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS\nOR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\nWARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\nARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY\nDIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\nDAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE\nGOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\nINTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\nWHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\nNEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\nSOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\nJulian Seward, jseward@bzip.org\nbzip2/libbzip2 version 1.0.6 of 6 September 2010\n\n\n\nharfbuzz:\n\nHarfBuzz is licensed under the so-called 'Old MIT' license.  Details follow.\n\nCopyright © 2010,2011,2012  Google, Inc.\nCopyright © 2012  Mozilla Foundation\nCopyright © 2011  Codethink Limited\nCopyright © 2008,2010  Nokia Corporation and/or its subsidiary(-ies)\nCopyright © 2009  Keith Stribley\nCopyright © 2009  Martin Hosken and SIL International\nCopyright © 2007  Chris Wilson\nCopyright © 2006  Behdad Esfahbod\nCopyright © 2005  David Turner\nCopyright © 2004,2007,2008,2009,2010  Red Hat, Inc.\nCopyright © 1998-2004  David Turner and Werner Lemberg\n\nFor full copyright notices consult the individual files in the package.\n\nPermission is hereby granted, without written agreement and without\nlicense or royalty fees, to use, copy, modify, and distribute this\nsoftware and its documentation for any purpose, provided that the\nabove copyright notice and the").append(" following two paragraphs appear in\nall copies of this software.\n\nIN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR\nDIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES\nARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN\nIF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH\nDAMAGE.\n\nTHE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,\nBUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND\nFITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS\nON AN 'AS IS' BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO\nPROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.\n\n\n\nfreetype:\n\nThe  FreeType 2  font  engine is  copyrighted  work and  cannot be  used\nlegally  without a  software license.   In  order to  make this  project\nusable  to a vast  majority of  developers, we  distribute it  under two\nmutually exclusive open-source licenses.\n\nThis means  that *you* must choose  *one* of the  two licenses described\nbelow, then obey  all its terms and conditions when  using FreeType 2 in\nany of your projects or products.\n\n  - The FreeType License, found in  the file `FTL.TXT', which is similar\n    to the original BSD license *with* an advertising clause that forces\n    you  to  explicitly cite  the  FreeType  project  in your  product's\n    documentation.  All  details are in the license  file.  This license\n    is  suited  to products  which  don't  use  the GNU  General  Public\n    License.\n\n    Note that  this license  is  compatible  to the  GNU General  Public\n    License version 3, but not version 2.\n\n  - The GNU General Public License version 2, found in  `GPLv2.TXT' (any\n    later version can be used  also), for programs which already use the\n    GPL.  Note  that the  FTL is  incompatible  with  GPLv2 due  to  its\n    advertisement clause.\n\nThe contributed BDF and PCF drivers come with a license similar  to that\nof the X Window System.  It is compatible to the above two licenses (see\nfile src/bdf/README and src/pcf/README).\n\nThe gzip module uses the zlib license (see src/gzip/zlib.h) which too is\ncompatible to the above two licenses.\n\nThe MD5 checksum support (only used for debugging in development builds)\nis in the public domain.\n\n\n\nudev:\n\nCopyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>\nCopyright (C) 2003-2010 Kay Sievers <kay@vrfy.org>\n\nThis program is free software: you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 2 of the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program.  If not, see <http://www.gnu.org/licenses/>.\n\n\n\ndbus:\n\nD-Bus is licensed to you under your choice of the Academic Free\nLicense version 2.1, or the GNU General Public License version 2\n(or, at your option any later version).\n\n\n\nicu:\n\nICU License - ICU 1.8.1 and later\nCOPYRIGHT AND PERMISSION NOTICE\nCopyright (c) 1995-2015 International Business Machines Corporation and others\nAll rights reserved.\nPermission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the 'Software'), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, provided that the above copyright notice(s) and this permission notice appear in all copies of the Software and that both the above copyright notice(s) and this permission notice appear in supporting documentation.\nTHE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\nExcept as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder.\n\n\n\nUnicode:\n\nCOPYRIGHT AND PERMISSION NOTICE\n\nCopyright © 1991-2015 Unicode, Inc. All rights reserved.\nDistributed under the Terms of Use in\nhttp://www.unicode.org/copyright.html.\n\nPermission is hereby granted, free of charge, to any person obtaining\na copy of the Unicode data files and any associated documentation\n(the 'Data Files') or Unicode software and any associated documentation\n(the 'Software') to deal in the Data Files or Software\nwithout restriction, including without limitation the rights to use,\ncopy, modify, merge, publish, distribute, and/or sell copies of\nthe Data Files or Software, and to permit persons to whom the Data Files\nor Software are furnished to do so, provided that\n(a) this copyright and permission notice appear with all copies\nof the Data Files or Software,\n(b) this copyright and permission notice appear in associated\ndocumentation, and\n(c) there is clear notice in each modified Data File or in the Software\nas well as in the documentation associated with the Data File(s) or\nSoftware that the data or software has been modified.\n\nTHE DATA FILES AND SOFTWARE ARE PROVIDED 'AS IS', WITHOUT WARRANTY OF\nANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE\nWARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\nNONINFRINGEMENT OF THIRD PARTY RIGHTS.\nIN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS\nNOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL\nDAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,\nDATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER\nTORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR\nPERFORMANCE OF THE DATA FILES OR SOFTWARE.\n\nExcept as contained in this notice, the name of a copyright holder\nshall not be used in advertising or otherwise to promote the sale,\nuse or other dealings in these Data Files or Software without prior\nwritten authorization of the copyright holder.\n\n\nUPX:\n\nCopyright (C) 1996-2013 Markus Franz Xaver Johannes Oberhumer\nCopyright (C) 1996-2013 László Molnár\nCopyright (C) 2000-2013 John F. Reiser\n\nAll Rights Reserved. This program may be used freely, and you are welcome to redistribute and/or modify it under certain conditions.\n\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the UPX License Agreement for more details: http://upx.sourceforge.net/upx-license.html");
    QMessageBox::information(this, "ExitDTM Licenses", strMessage, QMessageBox::Ok);
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_TermClear_clicked(
    )
{
    //Clears display buffer
    gbaDisplayBuffer.clear();
    ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
}

//=============================================================================
//=============================================================================
void
MainWindow::SystemTimeout(
    )
{
    //Occurs when there is a timeout waiting for a response
    QString strMessage = QString("Unfortunately, an error has occured whilst attempting to exit DTM mode on the attached module. Are you sure this module is a valid BL654 device and has the UART pins (and nRESET) wired correctly? Are you sure ").append(ui->combo_COM->currentText()).append(" is the correct serial port for this device? Are you sure there is a valid firmware image loaded to the module? Are you sure the provided serial settings (Baud rate: ").append(ui->combo_Baud->currentText()).append(", Handshaking: ").append(ui->combo_Handshake->currentText()).append(") is correct?\r\n\r\nPlease detail your setup and attach this message as a screenshot when you contact support for further assistance.\r\n\r\nProcess ID: ").append(QString::number(gintProgramState)).append(" CTS: ").append(QString::number(gbCTSStatus)).append(", Lines: ").append(QString::number(gchTermBusyLines)).append(", BufferA: ").append(gstrTermBusyData).append(", BufferB: ").append(gbaDisplayBuffer);
    gpSystemTimeout->stop();
    gintProgramState = ProgramStatusIdle;
    gchTermBusyLines = 0;
    gstrTermBusyData.clear();
    gbaDisplayBuffer.clear();
#ifdef TARGET_OS_MAC
    gpMacDoesntSupportCTSWorkaroundTimer->stop();
#endif

    //Close serial port
    TermClose();

    //Check which mode to run
    if (gbExitOnFinish == true)
    {
        //Exit application
        QApplication::exit(ExitCodeTimeout);
    }
    else
    {
        //Show message
        QMessageBox::warning(this, "Error during DTM escape", strMessage, QMessageBox::Ok);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::ForceClose(
    )
{
    //Forces the application to exit
    QApplication::exit(gintExitCode);
}

#ifdef TARGET_OS_MAC
//=============================================================================
//=============================================================================
void
MainWindow::ContinueOperationMacDoesntSupportCTSWorkaroundFunction(
    )
{
    //Workaround for mac
    if (gintProgramState == ProgramStatusExitDTM)
    {
        //We will just assume that the module has reset in normal mode, let's re-open the UART at the normal settings
        gintProgramState = ProgramStatusEraseFS;
        OpenDevice((QSerialPort::BaudRate)ui->combo_Baud->currentText().toUInt(), (ui->combo_Handshake->currentIndex() == 2 ? QSerialPort::SoftwareControl : (ui->combo_Handshake->currentIndex() == 1 ? QSerialPort::HardwareControl : QSerialPort::NoFlowControl)));

        //Send the clear configuration command
        DoLineEnd(); //In case module was not in DTM and has received garbage command
        gspSerialPort.write("at&f*");
        DoLineEnd();
        gbaDisplayBuffer.append("< at&f*\n");
        ui->text_TermEditData->setPlainText(gbaDisplayBuffer);
        ui->text_TermEditData->verticalScrollBar()->setValue(ui->text_TermEditData->verticalScrollBar()->maximum());
    }
}
#endif

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Cancel_clicked(
    )
{
    //Cancel operation
    TermClose();
    ui->statusBar->showMessage("Operation cancelled!");
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Help_clicked(
    )
{
    //Open help file
    if (QDesktopServices::openUrl(QUrl(QString("http://").append(ServerHost).append("/exitdtm_help.pdf"))) == false)
    {
        //Failed to open
        QMessageBox::critical(this, "Error opening help file", QString("An error occured whilst attempting to open the online help file: http://").append(ServerHost).append("/exitdtm_help.pdf"), QMessageBox::Ok, QMessageBox::NoButton);
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/

/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: ExitDTM
**
** Module: main.cpp
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
#include <QApplication>
#include <QCommandLineParser>
#if TARGET_OS_MAC
#include <QStyleFactory>
#endif

//=============================================================================
//=============================================================================
int
main(
    int argc,
    char *argv[]
    )
{
    QApplication a(argc, argv);
#if TARGET_OS_MAC
    //Fix for Mac to stop bad styling
    QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif
    MainWindow w;

    return a.exec();
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/

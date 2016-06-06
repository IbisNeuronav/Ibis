/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Dante De Nigris for writing this class

#include <QApplication>
#include <QObject>
#include <QThread>

#include <iostream>

#include "gpu_volumereconstructionwidget.h"

int main(int argc, char*argv[])
{
  QApplication app(argc, argv);

  GPU_VolumeReconstructionWidget reconstructionWidget;

  reconstructionWidget.show();

  return app.exec();
}

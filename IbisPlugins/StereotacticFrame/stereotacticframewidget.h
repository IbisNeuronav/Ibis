/*=========================================================================
Ibis Neuronav
Copyright (c) Simon Drouin, Anna Kochanowska, Louis Collins.
All rights reserved.
See Copyright.txt or http://ibisneuronav.org/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// Thanks to Simon Drouin for writing this class

#ifndef __StereotacticFrameWidget_h_
#define __StereotacticFrameWidget_h_

#include <QWidget>

class Application;
class StereotacticFramePluginInterface;

namespace Ui
{
    class StereotacticFrameWidget;
}


class StereotacticFrameWidget : public QWidget
{

    Q_OBJECT

public:

    explicit StereotacticFrameWidget(QWidget *parent = 0);
    ~StereotacticFrameWidget();

    void SetPluginInterface( StereotacticFramePluginInterface * pi );

private slots:

    void OnCursorMoved();
    void on_show3DFrameCheckBox_toggled(bool checked);
    void on_showManipulatorsCheckBox_toggled(bool checked);

    void on_lineEditFrameX_editingFinished();
    void on_lineEditFrameY_editingFinished();
    void on_lineEditFrameZ_editingFinished();

private:

    void UpdateUi();
    void UpdateCursor();

    void ParseFramePosition(double pos[3]);

    StereotacticFramePluginInterface * m_pluginInterface;
    Ui::StereotacticFrameWidget * ui;

};

#endif

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

#ifndef __KeyframeEditorWidget_h_
#define __KeyframeEditorWidget_h_

#include <QWidget>
#include <vector>

class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;
class QColor;

class AnimatePluginInterface;

class KeyframeEditorWidget : public QWidget
{

    Q_OBJECT

public:

    KeyframeEditorWidget( QWidget* parent );
    ~KeyframeEditorWidget();
    void SetAnim( AnimatePluginInterface * anim );

private:

    void paintEvent( QPaintEvent * event );

    // Handling of mouse events
    void mouseReleaseEvent( QMouseEvent * event );
    void mouseMoveEvent( QMouseEvent * event );
    void mousePressEvent( QMouseEvent * event );

    void MoveCurrentKey( int x, int y );
    void FrameFromPixCoord( int x, int y, int & paramIndex, int & frame );
    void PixCoordFromFrame( int paramIndex, int frame, double & x, double & y );

    static const int m_keySize;
    static const int m_rowSize;

    int m_movingRowIndex;
    int m_movingKeyIndex;

    AnimatePluginInterface * m_anim;
};

#endif

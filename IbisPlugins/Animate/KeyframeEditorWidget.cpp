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

#include "KeyframeEditorWidget.h"
#include "animateplugininterface.h"

#include <QPainter>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QMouseEvent>

#include <limits>

const int KeyframeEditorWidget::m_keySize = 6;
const int KeyframeEditorWidget::m_rowSize = 16;

KeyframeEditorWidget::KeyframeEditorWidget( QWidget * parent )
    : QWidget( parent )
{
    m_anim = 0;
    m_movingRowIndex = -1;
    m_movingKeyIndex = -1;
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setMinimumSize( QSize( 200, 10 ) );
}

KeyframeEditorWidget::~KeyframeEditorWidget()
{
}

void KeyframeEditorWidget::SetAnim( AnimatePluginInterface * anim )
{
    m_anim = anim;
    setMinimumHeight( m_anim->GetNumberOfAnimatedParams() * m_rowSize );
}

void KeyframeEditorWidget::mousePressEvent( QMouseEvent * event )
{
    Q_UNUSED(event)

    int frame = -1;
    int row = -1;
    this->FrameFromPixCoord( event->x(), event->y(), row, frame );
    int closestKey = m_anim->FindClosestKey( row, frame );
    if( closestKey != -1 )
    {
        double keyX, keyY;
        this->PixCoordFromFrame( row, closestKey, keyX, keyY );
        if( abs( keyX - event->x() ) <= m_keySize )
        {
            m_movingRowIndex = row;
            m_movingKeyIndex = m_anim->GetKeyForFrame( row, closestKey );
        }
    }

    update();
}

void KeyframeEditorWidget::mouseReleaseEvent( QMouseEvent * event )
{
    if( m_movingRowIndex == -1 || m_movingKeyIndex == -1 )
        return;

    MoveCurrentKey( event->x(), event->y() );

    m_movingRowIndex = -1;
    m_movingKeyIndex = -1;

    update();
}


void KeyframeEditorWidget::mouseMoveEvent(QMouseEvent* event)
{
    if( m_movingRowIndex == -1 || m_movingKeyIndex == -1 )
        return;

    MoveCurrentKey( event->x(), event->y() );

    update();
}

void KeyframeEditorWidget::MoveCurrentKey( int x, int y )
{
    int row = -1;
    int frame = -1;
    FrameFromPixCoord( x, y, row, frame );
    if( frame < 0 )
        frame = 0;
    if( frame > m_anim->GetNumberOfFrames() - 1 )
        frame = m_anim->GetNumberOfFrames() - 1;
    int oldFrame = m_anim->GetFrameForKey( m_movingRowIndex, m_movingKeyIndex );
    m_anim->MoveKey( m_movingRowIndex, oldFrame, frame );
}

void KeyframeEditorWidget::paintEvent( QPaintEvent * event )
{
    QPainter p(this);
    p.setRenderHint( QPainter::HighQualityAntialiasing );

    // Draw background
    QRect backRect( 0, 0, width() - 1, height() - 1 );
    p.setBrush( palette().brush( QPalette::Base ) );
    p.setPen( palette().color( QPalette::Dark ) );
    p.drawRect( backRect );

    // draw lines between params
    for( int i = 0; i < m_anim->GetNumberOfAnimatedParams(); ++i )
        p.drawLine( 0, (i+1) * m_rowSize, width() - 1, (i+1) * m_rowSize );

    // Draw all keyframes
    p.setBrush( Qt::cyan );
    p.setPen( Qt::black );
    if( m_anim )
    {
        for( int row = 0; row < m_anim->GetNumberOfAnimatedParams(); ++ row )
        {
            for( int key = 0; key < m_anim->GetNumberOfKeys( row ); ++key )
            {
                int frame = m_anim->GetFrameForKey( row, key );
                double x, y;
                PixCoordFromFrame( row, frame, x, y );
                p.drawEllipse( QPointF( x, y ), m_keySize, m_keySize );
            }
        }
    }

    p.setPen( Qt::red );
    double xl,yl;
    PixCoordFromFrame( 0, m_anim->GetCurrentFrame(), xl, yl );
    p.drawLine( QPointF( xl, 0 ), QPointF( xl, height() - 1 ) );

    p.end();
}

void KeyframeEditorWidget::FrameFromPixCoord( int x, int y, int & paramIndex, int & frame )
{
    paramIndex = y / m_rowSize;
    double frameLength = ((double)(width() - 2 * m_keySize )) / (m_anim->GetNumberOfFrames() - 1);
    frame = (int)round( ( x - m_keySize ) / frameLength );
}

void KeyframeEditorWidget::PixCoordFromFrame( int paramIndex, int frame, double & x, double & y )
{
    double frameLength = ((double)( width() - 2 * m_keySize )) / (m_anim->GetNumberOfFrames() - 1);
    x = m_keySize + frame * frameLength;
    y = 0.5 * m_rowSize + paramIndex * m_rowSize;
}

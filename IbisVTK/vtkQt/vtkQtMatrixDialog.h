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

#ifndef MATRIXDIALOG_H
#define MATRIXDIALOG_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout; 
class QHBoxLayout; 
class QGridLayout; 
class QLineEdit;
class QPushButton;
class QFile;
class vtkMatrix4x4;
class vtkHomogeneousTransform;
class vtkEventQtSlotConnect;

class vtkQtMatrixDialog : public QDialog
{ 
    Q_OBJECT

public:

    vtkQtMatrixDialog( bool readOnly, QWidget * parent = 0 );
    ~vtkQtMatrixDialog();

    void SetMatrix( vtkMatrix4x4 * mat );
    void SetDirectory(const QString &dir);
    void UpdateUI();

    QLineEdit   * m_matEdit[4][4];
    QPushButton * m_invertButton;
    QPushButton * m_identityButton;
    QPushButton * m_revertButton;
    QPushButton * m_loadButton;
    QPushButton * m_saveButton;
    QPushButton * m_copyButton;
    QPushButton * m_pasteButton;

signals:

    void MatrixModified();

public slots:
    
    void InvertButtonClicked();
    void IdentityButtonClicked();
    void RevertButtonClicked();
    void MatrixChanged();
    void LoadButtonClicked();
    void SaveButtonClicked();
    void CopyButtonClicked();
    void PasteButtonClicked();
    void SetMatrixElements( );
    void LineEditChanged();

protected:

    bool m_readOnly;
    vtkMatrix4x4 * m_matrix;
    vtkMatrix4x4 * m_copy_matrix;
    QString m_directory;
    
    vtkEventQtSlotConnect * m_eventSlotConnect;  // used to connect vtkEvents to Slots of this class
    
    QVBoxLayout * MatrixDialogLayout;
    QGridLayout * gridBox;
    QHBoxLayout * Layout3;

    bool  LoadFromXFMFile(QFile *f, vtkMatrix4x4 * mat);
};


#endif

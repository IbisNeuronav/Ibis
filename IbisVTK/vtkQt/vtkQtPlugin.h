#ifndef VTKQTPLUGIN_H
#define VTKQTPLUGIN_H

#include <QDesignerCustomWidgetCollectionInterface>
#include <QDesignerCustomWidgetInterface>
#include <QObject>
#include <QWidget>
#include <QtPlugin>

class vtkQtHistogramWidgetPlugin : public QDesignerCustomWidgetInterface
{
public:
    vtkQtHistogramWidgetPlugin();
    ~vtkQtHistogramWidgetPlugin();

    QString name() const;
    QString domXml() const;
    QWidget * createWidget( QWidget * parent = 0 );
    QString group() const;
    QIcon icon() const;
    QString includeFile() const;
    QString toolTip() const;
    QString whatsThis() const;
    bool isContainer() const;
};

class vtkQtPiecewiseFunctionWidgetPlugin : public QDesignerCustomWidgetInterface
{
public:
    vtkQtPiecewiseFunctionWidgetPlugin();
    ~vtkQtPiecewiseFunctionWidgetPlugin();

    QString name() const;
    QString domXml() const;
    QWidget * createWidget( QWidget * parent = 0 );
    QString group() const;
    QIcon icon() const;
    QString includeFile() const;
    QString toolTip() const;
    QString whatsThis() const;
    bool isContainer() const;
};

class vtkQtColorTransferFunctionWidgetPlugin : public QDesignerCustomWidgetInterface
{
public:
    vtkQtColorTransferFunctionWidgetPlugin();
    ~vtkQtColorTransferFunctionWidgetPlugin();

    QString name() const;
    QString domXml() const;
    QWidget * createWidget( QWidget * parent = 0 );
    QString group() const;
    QIcon icon() const;
    QString includeFile() const;
    QString toolTip() const;
    QString whatsThis() const;
    bool isContainer() const;
};

// implement designer widget collection interface
class vtkQtPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_INTERFACES( QDesignerCustomWidgetCollectionInterface )

public:
    vtkQtPlugin();
    ~vtkQtPlugin();

    virtual QList<QDesignerCustomWidgetInterface *> customWidgets() const;

private:
    vtkQtHistogramWidgetPlugin * m_histogramWidgetPlugin;
    vtkQtPiecewiseFunctionWidgetPlugin * m_piecewiseFunctionWidgetPlugin;
    vtkQtColorTransferFunctionWidgetPlugin * m_colorTransferFunctionWidgetPlugin;
};

#endif

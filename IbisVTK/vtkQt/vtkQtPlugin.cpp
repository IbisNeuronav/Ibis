#include "vtkQtPlugin.h"
#include "vtkQtPlugin.xpm"
#include "vtkQtHistogramWidget.h"
#include "vtkQtPiecewiseFunctionWidget.h"
#include "vtkQtColorTransferFunctionWidget.h"

//==================================================================
// Histogram widget wrapper
//==================================================================

vtkQtHistogramWidgetPlugin::vtkQtHistogramWidgetPlugin()
{
}

vtkQtHistogramWidgetPlugin::~vtkQtHistogramWidgetPlugin()
{
}

QString vtkQtHistogramWidgetPlugin::name() const
{
    return "vtkQtHistogramWidget";
}

QString vtkQtHistogramWidgetPlugin::domXml() const
{
    return QLatin1String("<widget class=\"vtkQtHistogramWidget\" name=\"vtkqthistogramwidget\">\n"
                         " <property name=\"geometry\">\n"
                         "  <rect>\n"
                         "   <x>0</x>\n"
                         "   <y>0</y>\n"
                         "   <width>100</width>\n"
                         "   <height>100</height>\n"
                         "  </rect>\n"
                         " </property>\n"
                         "</widget>\n");
}

QWidget * vtkQtHistogramWidgetPlugin::createWidget(QWidget* parent)
{
    vtkQtHistogramWidget * widget = new vtkQtHistogramWidget( parent );

    // return the widget
    return widget;
}

QString vtkQtHistogramWidgetPlugin::group() const
{
    return "vtkQt";
}

QIcon vtkQtHistogramWidgetPlugin::icon() const
{
    return QIcon( QPixmap( vtkQtPlugin_xpm ) );
}

QString vtkQtHistogramWidgetPlugin::includeFile() const
{
    return "vtkQtHistogramWidget.h";
}

QString vtkQtHistogramWidgetPlugin::toolTip() const
{
    return "Qt vtk histogram widget";
}

QString vtkQtHistogramWidgetPlugin::whatsThis() const
{
    return "A Qt widget to display vtk histograms";
}

bool vtkQtHistogramWidgetPlugin::isContainer() const
{
    return false;
}

//==================================================================
// PiecewiseFunction widget wrapper
//==================================================================

vtkQtPiecewiseFunctionWidgetPlugin::vtkQtPiecewiseFunctionWidgetPlugin()
{
}

vtkQtPiecewiseFunctionWidgetPlugin::~vtkQtPiecewiseFunctionWidgetPlugin()
{
}

QString vtkQtPiecewiseFunctionWidgetPlugin::name() const
{
    return "vtkQtPiecewiseFunctionWidget";
}

QString vtkQtPiecewiseFunctionWidgetPlugin::domXml() const
{
    return QLatin1String("<widget class=\"vtkQtPiecewiseFunctionWidget\" name=\"vtkQtPiecewiseFunctionWidget\">\n"
                         " <property name=\"geometry\">\n"
                         "  <rect>\n"
                         "   <x>0</x>\n"
                         "   <y>0</y>\n"
                         "   <width>200</width>\n"
                         "   <height>200</height>\n"
                         "  </rect>\n"
                         " </property>\n"
                         "</widget>\n");
}

QWidget* vtkQtPiecewiseFunctionWidgetPlugin::createWidget(QWidget* parent)
{
    vtkQtPiecewiseFunctionWidget * widget = new vtkQtPiecewiseFunctionWidget( parent );

    // return the widget
    return widget;
}

QString vtkQtPiecewiseFunctionWidgetPlugin::group() const
{
    return "vtkQt";
}

QIcon vtkQtPiecewiseFunctionWidgetPlugin::icon() const
{
    return QIcon( QPixmap( vtkQtPlugin_xpm ) );
}

QString vtkQtPiecewiseFunctionWidgetPlugin::includeFile() const
{
    return "vtkQtPiecewiseFunctionWidget.h";
}

QString vtkQtPiecewiseFunctionWidgetPlugin::toolTip() const
{
    return "Qt vtkPiecewiseFunction edition widget";
}

QString vtkQtPiecewiseFunctionWidgetPlugin::whatsThis() const
{
    return "A Qt widget display and edit vtkPiecewiseFunction";
}

bool vtkQtPiecewiseFunctionWidgetPlugin::isContainer() const
{
    return false;
}

//==================================================================
// ColorTransferFunction widget wrapper
//==================================================================

vtkQtColorTransferFunctionWidgetPlugin::vtkQtColorTransferFunctionWidgetPlugin()
{
}

vtkQtColorTransferFunctionWidgetPlugin::~vtkQtColorTransferFunctionWidgetPlugin()
{
}

QString vtkQtColorTransferFunctionWidgetPlugin::name() const
{
    return "vtkQtColorTransferFunctionWidget";
}

QString vtkQtColorTransferFunctionWidgetPlugin::domXml() const
{
    return QLatin1String("<widget class=\"vtkQtColorTransferFunctionWidget\" name=\"vtkQtColorTransferFunctionWidget\">\n"
                         " <property name=\"geometry\">\n"
                         "  <rect>\n"
                         "   <x>0</x>\n"
                         "   <y>0</y>\n"
                         "   <width>200</width>\n"
                         "   <height>60</height>\n"
                         "  </rect>\n"
                         " </property>\n"
                         "</widget>\n");
}

QWidget* vtkQtColorTransferFunctionWidgetPlugin::createWidget(QWidget* parent)
{
    vtkQtColorTransferFunctionWidget * widget = new vtkQtColorTransferFunctionWidget( parent );

    // return the widget
    return widget;
}

QString vtkQtColorTransferFunctionWidgetPlugin::group() const
{
    return "vtkQt";
}

QIcon vtkQtColorTransferFunctionWidgetPlugin::icon() const
{
    return QIcon( QPixmap( vtkQtPlugin_xpm ) );
}

QString vtkQtColorTransferFunctionWidgetPlugin::includeFile() const
{
    return "vtkQtColorTransferFunctionWidget.h";
}

QString vtkQtColorTransferFunctionWidgetPlugin::toolTip() const
{
    return "Qt vtkColorTransferFunction edition widget";
}

QString vtkQtColorTransferFunctionWidgetPlugin::whatsThis() const
{
    return "A Qt widget display and edit vtkColorTransferFunction";
}

bool vtkQtColorTransferFunctionWidgetPlugin::isContainer() const
{
    return false;
}

//==================================================================
// General plugin implementation
//==================================================================

vtkQtPlugin::vtkQtPlugin()
{
    m_histogramWidgetPlugin = new vtkQtHistogramWidgetPlugin;
    m_piecewiseFunctionWidgetPlugin = new vtkQtPiecewiseFunctionWidgetPlugin;
    m_colorTransferFunctionWidgetPlugin = new vtkQtColorTransferFunctionWidgetPlugin;
}

vtkQtPlugin::~vtkQtPlugin()
{
    delete m_histogramWidgetPlugin;
    delete m_piecewiseFunctionWidgetPlugin;
    delete m_colorTransferFunctionWidgetPlugin;
}

QList<QDesignerCustomWidgetInterface*> vtkQtPlugin::customWidgets() const
{
    QList<QDesignerCustomWidgetInterface*> plugins;
    plugins.append(m_histogramWidgetPlugin);
    plugins.append(m_piecewiseFunctionWidgetPlugin);
    plugins.append(m_colorTransferFunctionWidgetPlugin);
    return plugins;
}

Q_EXPORT_PLUGIN(vtkQtPlugin)


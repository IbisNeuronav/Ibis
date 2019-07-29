#ifndef IBISPREFERENCES_H
#define IBISPREFERENCES_H

#include <QObject>
#include <QWidget>
#include <QMap>
#include <QSettings>

class PreferenceWidget;

class IbisPreferences : public QObject
{
    Q_OBJECT
public:
    explicit IbisPreferences(QObject *parent = nullptr);

    void LoadSettings( QSettings & settings );
    void SaveSettings( QSettings & settings );
    void RegisterCustomVariable( const QString  & varName, const QString & customVariable );
    void UnRegisterCustomVariable(const QString &varName );
    const QString GetCustomVariable(const QString &varName );

    enum VARIABLE_TYPE{ FILE_VARIABLE_TYPE, DIRECTORY_VARIABLE_TYPE, UNKNOWN_VARIABLE_TYPE };
    struct TypedVariable
    {
        QString name;
        VARIABLE_TYPE varType;
    };

    void ShowPreferenceDialog();
    QMap< QString, TypedVariable > GetCustomVariables( ) { return m_customVariables; }
    void SetCustomPaths( QMap< QString, TypedVariable > &map ) { m_customVariables = map; }
signals:

protected:
    typedef QMap< QString, TypedVariable > CustomVariablesMap;
    CustomVariablesMap m_customVariables;

private slots:
    void OnPreferenceWidgetClosed();

private:
    PreferenceWidget *m_prefWidget;
};

#endif // IBISPREFERENCES_H

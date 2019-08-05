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
    void RegisterPath( const QString  & name, const QString & path );
    void UnRegisterPath(const QString &pathName );
    const QString GetPath(const QString &pathName );
    bool IsPathRegistered( const QString &pathName );

    void ShowPreferenceDialog();
    QMap< QString, QString > GetCustomPaths( ) { return m_customPaths; }
    void SetCustomPaths( QMap< QString, QString > &map ) { m_customPaths = map; }
signals:

protected:
    typedef QMap< QString, QString > CustomPathsMap;
    CustomPathsMap m_customPaths;

private slots:
    void OnPreferenceWidgetClosed();

private:
    PreferenceWidget *m_prefWidget;
};

#endif // IBISPREFERENCES_H

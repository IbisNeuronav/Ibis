#ifndef IBISPREFERENCES_H
#define IBISPREFERENCES_H

#include <QMap>
#include <QObject>
#include <QSettings>
#include <QWidget>

/**
 * @class   IbisPreferences
 * @brief   Setting paths to directories or executables needed in Ibis.
 *
 *  Preferences are saved in QSettings together with ApplicationSettings,
 *  loaded automatically on application start and saved automatically on exit.
 *  @sa ApplicationSettings
 *
 */

class PreferenceWidget;

class IbisPreferences : public QObject
{
    Q_OBJECT
public:
    explicit IbisPreferences( QObject * parent = nullptr );

    /** Load saved preferences. */
    void LoadSettings( QSettings & settings );
    /** Save current preferences. */
    void SaveSettings( QSettings & settings );
    /** Map a selected path to a preference name. */
    void RegisterPath( const QString & name, const QString & path );
    /** Remove mapped path. */
    void UnRegisterPath( const QString & pathName );
    /** Find a path using its name. */
    const QString GetPath( const QString & pathName );
    /** Check if the path is in the preferences. */
    bool IsPathRegistered( const QString & pathName );

    /** Show the settings dialog. */
    void ShowPreferenceDialog();
    /** Get all mapped paths. */
    QMap<QString, QString> GetCustomPaths() { return m_customPaths; }
    /** Set mapped paths. */
    void SetCustomPaths( QMap<QString, QString> & map ) { m_customPaths = map; }

protected:
    typedef QMap<QString, QString> CustomPathsMap;
    CustomPathsMap m_customPaths;

private slots:
    void OnPreferenceWidgetClosed();

private:
    PreferenceWidget * m_prefWidget;
};

#endif  // IBISPREFERENCES_H

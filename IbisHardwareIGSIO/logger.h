#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QStringList>

class Logger : public QObject
{

    Q_OBJECT

public:
    explicit Logger(QObject *parent = nullptr);
    void AddLog( const QString & log );
    void ClearLog();
    QString GetAll() const;
    QString GetLast() const;

signals:

    void LogAdded( const QString & last );
    void LogCleared();

protected:

    static const int m_maxNumberOfLogs;
    QStringList m_log;

};

#endif

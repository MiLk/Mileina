#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <QObject>
#include <QMutex>
#include <QDateTime>
#include <QTimer>
#include <QMap>

class Timer
{
public:
    Timer(QString _from, QString _to, QString _message, uint _w, uint _d, uint _h, uint _m, uint _s):
        m_from(_from),
        m_to(_to),
        m_message(_message),
        m_start(QDateTime::currentDateTimeUtc()),
        m_end(QDateTime::currentDateTimeUtc())
    {
        m_end = m_end.addDays(_w * 7 + _d);
        m_end = m_end.addSecs(_h * 3600 + _m * 60 + _s);
    }
    QString getFrom() const { return m_from; }
    QString getTo() const { return m_to; }
    QString getMessage() const { return m_message; }
    QDateTime getStart() const { return m_start; }
    QDateTime getEnd() const { return m_end; }
private:
    QString m_from;
    QString m_to;
    QString m_message;
    QDateTime m_start;
    QDateTime m_end;
};

class TimerManager : public QObject
{
    Q_OBJECT
public:
    static TimerManager* getInstance();
    static void drop();
    const Timer addTimer(QString _from, QString _to, QString _message, uint _w, uint _d, uint _h, uint _m, uint _s);
    const QMap<qint64,Timer>& getTimerList() const { return m_timerList; }
    int removeTimer(qint64 _key);
signals:
    void timerFinished(const Timer& _timer);
private slots:
    void update();
private:
    TimerManager();
    TimerManager(const TimerManager &);
    TimerManager& operator=(const TimerManager &);
    static TimerManager* s_pInstance;
    QTimer* m_pTimer;
    QMap<qint64,Timer> m_timerList;
};

#endif // TIMERMANAGER_H

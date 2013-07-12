#include "timermanager.h"

#include "qirc.h"

TimerManager* TimerManager::s_pInstance = 0;

TimerManager::TimerManager(): QObject() {
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(update()));
    m_pTimer->start(1000);
}

TimerManager* TimerManager::getInstance()
{
    static QMutex mutex;
    if (!s_pInstance)
    {
        mutex.lock();

        if (!s_pInstance)
            s_pInstance = new TimerManager;

        mutex.unlock();
    }

    return s_pInstance;
}

void TimerManager::drop()
{
    static QMutex mutex;
    mutex.lock();
    delete s_pInstance;
    s_pInstance = 0;
    mutex.unlock();
}

const Timer TimerManager::addTimer(QString _from, QString _to, QString _message, uint _w, uint _d, uint _h, uint _m, uint _s) {
    Timer timer(_from, _to, _message,_w,_d,_h,_m,_s);
    m_timerList.insert(timer.getEnd().toMSecsSinceEpoch(),timer);
    return timer;
}

void TimerManager::update() {
    if(m_timerList.empty())
        return;
    QMap<qint64,Timer>::Iterator itr = m_timerList.begin();
    while((!m_timerList.empty()) && (itr.key() <= QDateTime::currentDateTimeUtc().toMSecsSinceEpoch())) {
        emit timerFinished(*itr);
        itr = m_timerList.erase(itr);
    }
}

int TimerManager::removeTimer(qint64 _key) {
    return m_timerList.remove(_key);
}

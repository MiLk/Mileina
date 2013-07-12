#ifndef QIRC_H
#define QIRC_H

#include <QObject>
#include <QtNetwork/QAbstractSocket>
#include <QString>
#include <QJsonObject>

class QTcpSocket;
class TimerManager;
class Timer;
class QIrc : public QObject
{
    Q_OBJECT
public:
    explicit QIrc(QObject *parent = 0);
    ~QIrc();
    
signals:
    
public slots:

private slots:
    void readData();
    void onConnected();
    void onDisconnected();
    void displayError(QAbstractSocket::SocketError socketError);
    void onTimerFinished(const Timer& _timer);

private:
    void parseCommand(QString _s);
    void receiveCommand(QString _from, QString _command, QString _to, QString _msg);
    void send(QString _command, QString _to, QString _msg);
    void sendRaw(QString _s);
    void identify();
    void joinChannels();
    void join(QString _channel, QString _password = NULL);

    QString m_oReceivedMsg;
    QTcpSocket *m_pSocket;
    QJsonObject m_config;
};

#endif // QIRC_H

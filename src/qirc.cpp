#include "qirc.h"

#include <QtNetwork/QTcpSocket>
#include <QtDebug>
#include <QtScript/QScriptEngine>
#include <QStringList>
#include <QFile>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <math.h>

#include "timermanager.h"

QIrc::QIrc(QObject *_parent) :
    QObject(_parent)
{
    QFile file("config.json");
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Unable to open config file.";
        exit(1);
    }
    QJsonParseError error;
    QJsonDocument config = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if(error.error != QJsonParseError::NoError) {
        qDebug() << error.errorString();
        exit(1);
    }
    if(!config.isObject()) {
        qDebug() << "Bad config file format.";
        exit(1);
    }
    m_config = config.object();

    m_pSocket = new QTcpSocket(this);

    connect(m_pSocket, SIGNAL(readyRead()),
            this, SLOT(readData()));
    connect(m_pSocket, SIGNAL(connected()),
            this, SLOT(onConnected()));
    connect(m_pSocket, SIGNAL(disconnected()),
            this, SLOT(onDisconnected()));
    connect(m_pSocket, SIGNAL(error( QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    m_pSocket->connectToHost(m_config["address"].toString(),m_config["port"].toDouble());

    connect(TimerManager::getInstance(), SIGNAL(timerFinished(Timer)),
            this, SLOT(onTimerFinished(Timer)));
}

QIrc::~QIrc()  {
    TimerManager::drop();
}

void QIrc::displayError(QAbstractSocket::SocketError _socketError) {
    switch (_socketError) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            qDebug() << "The host was not found. "
                        "Please check the host name and port settings.";
            break;
        case QAbstractSocket::ConnectionRefusedError:
            qDebug() << "The connection was refused by the peer. "
                        "Make sure the fortune server is running, "
                        "and check that the host name and port "
                        "settings are correct.";
            break;
        default:
            qDebug() << "The following error occurred:"  << m_pSocket->errorString();
    }
}

void QIrc::readData() {
    m_oReceivedMsg += m_pSocket->readAll();
    bool notFinished = false;
    if(m_oReceivedMsg.right(2) != "\r\n")
        notFinished = true;

    QStringList list = m_oReceivedMsg.split("\r\n", QString::SkipEmptyParts);
    if(notFinished) {
        m_oReceivedMsg = list.takeLast();
    }
    else
        m_oReceivedMsg = QString();

    for(QStringList::Iterator itr = list.begin(); itr != list.end(); ++itr) {
        parseCommand(*itr);
    }
}

void QIrc::onConnected() {
    qDebug() << "Connected to the server !";
    sendRaw("NICK "+ m_config["nickname"].toString() +"\r\n"
            "USER "+ m_config["nickname"].toString() +" "+ m_config["nickname"].toString() +" "
            + m_pSocket->peerName() +
            " :"+ m_config["nickname"].toString() +"");
}

void QIrc::onDisconnected() {
    qDebug() << "Disconnected from the server !";
}

void QIrc::parseCommand(QString _s) {
    if(_s.size()) {
        if(_s[0] == ':') {
            QStringList cmd = _s.split(' ');
            if(cmd[0].indexOf('!') > -1) {
                if(cmd[0][0] == '~' || cmd[0][0] == ':')
                    cmd[0] = cmd[0].mid(1);
                QString from = cmd.takeFirst();
                QString command = cmd.takeFirst();
                QString to = cmd.takeFirst();
                receiveCommand(from, command, to, cmd.join(" "));
            } else { // no "!" so the user has no mask, messages from the host
                bool isInt;
                int msgCode = cmd[1].toInt(&isInt);
                if(isInt) {
                    switch(msgCode) {
                        case 376:
                            qDebug() << "identification needed";
                            break;
                        case 401:
                            qDebug() << "no such nick/channel";
                            break;
                        case 403:
                            qDebug() << "no such channel";
                            break;
                        case 404:
                            qDebug() << "cannot send text to channel";
                            break;
                        case 405:
                            qDebug() << "too many channels joined";
                            break;
                        case 407:
                            qDebug() << "duplicate entries";
                            break;
                        case 421:
                            qDebug() << "unknown command";
                            break;
                        case 431:
                            qDebug() << "no nick given";
                            break;
                        case 432:
                            qDebug() << "erroneus nickname (invalid character)";
                            break;
                        case 433:
                            qDebug() << "nick already in use";
                            break;
                        case 436:
                            qDebug() << "nickname collision";
                            break;
                        case 442:
                            qDebug() << "you are not on that channel";
                            break;
                        case 451:
                            qDebug() << "you have not registered";
                            break;
                        case 461:
                            qDebug() << "Not enough parameters";
                            break;
                        case 464:
                            qDebug() << "password incorrect";
                            break;
                        case 471:
                            qDebug() << "cannot join this channel (full)";
                            break;
                        case 473:
                            qDebug() << "cannot join this channel (invite only)";
                            break;
                        case 474:
                            qDebug() << "cannot join this channel (you've been ban)";
                            break;
                        case 475:
                            qDebug() << "cannot join this channel (need password)";
                            break;
                        case 481:
                            qDebug() << "you are not an IRC operator";
                            break;
                        case 482:
                            qDebug() << "you are not a channel operator";
                            break;
                        default:
                            qDebug() << "Useless message: " << _s;
                            break;
                    }
                }
            }
        }
        else if(_s.section(' ', 0, 0) == "PING") {
            _s[1] = 'O';
            sendRaw(_s);
        } else {
            qDebug() << "Unparsed message: " << _s;
        }

    }
}

void QIrc::sendRaw(QString _s) {
    m_pSocket->write(_s.toUtf8() + "\r\n");
}

void QIrc::send(QString _command, QString _to, QString _msg) {
    sendRaw(_command + " " + _to + " " + _msg);
}

void QIrc::identify() {
    sendRaw("MODE "+ m_config["nickname"].toString() +" +B");
    sendRaw("PRIVMSG NickServ :IDENTIFY "+ m_config["password"].toString() +"");
    joinChannels();
}

void QIrc::joinChannels() {
    QJsonArray channels = m_config["channels"].toArray();
    for(QJsonArray::ConstIterator citr = channels.begin(); citr != channels.end(); ++citr) {
        join((*citr).toObject()["channel"].toString(), (*citr).toObject()["password"].toString());
    }
}

void QIrc::join(QString _channel, QString _password) {
    if(_password.isEmpty())
        sendRaw("JOIN " + _channel);
    else
        sendRaw("JOIN " + _channel + " " + _password);
}

void QIrc::receiveCommand(QString _from, QString _command, QString _to, QString _msg) {
    if(_msg[0] == '~' || _msg[0] == ':')
        _msg = _msg.mid(1);

    QString realname = "";
    QString hostname = "";
    QRegExp rx("^([a-z0-9]+)!([^a-z0-9]?[a-z0-9]+)@(.+)$", Qt::CaseInsensitive);
    if(rx.exactMatch(_from)) {
        QStringList list = rx.capturedTexts();
        list.removeFirst();
        _from = list.takeFirst();
        realname = list.takeFirst();
        hostname = list.takeFirst();
    }

    if(_from == "NickServ" && _command == "NOTICE") {
        if(_msg.contains("please choose a different nick")) {
            identify();
        }
    }

    QString to = (_to[0] == '#') ? _to : _from;
    if(_command == "PRIVMSG") {
        if(_msg.contains("ACTION") && _msg.contains(m_config["nickname"].toString())) {
            _msg.replace(m_config["nickname"].toString(),_from);
            send(_command, to, _msg);
            return;
        }
        QRegExp re("^\\?.+$", Qt::CaseInsensitive);
        if(re.exactMatch(_msg)) {
            _msg.remove(0,1);
            _msg.replace(",",".");
            _msg.replace(QRegExp("([0-9\\.]+)\\s*\\^\\s*([0-9\\.]+)"),"Math.pow(\\1,\\2)");
            QScriptEngine engine;
            QString function("function myFunction() { return " + _msg + "; }");
            engine.evaluate(function);
            if(engine.hasUncaughtException()) {
                send(_command, to, _from + ": " + engine.uncaughtException().toString());
            } else {
                QScriptValue scriptFunction = engine.globalObject().property("myFunction");
                double result = scriptFunction.call().toNumber();
                if(!isnan(result))
                    send(_command, to, _from + ": " + QString("%1").arg(result));
            }
            return;
        }
        re.setPattern("^\\.t\\s*"
                   "(?:(\\d+)w)?"
                   "(?:(\\d+)d)?"
                   "(?:(\\d+)h)?"
                   "(?:(\\d+)m)?"
                   "(?:(\\d+)s)?"
                   "(?:\\s+([^\\s]+.*))?$");
        if(re.exactMatch(_msg)) {
            QStringList list = re.capturedTexts();
            if(!(
               list[1].toUInt() == 0
            && list[2].toUInt() == 0
            && list[3].toUInt() == 0
            && list[4].toUInt() == 0
            && list[5].toUInt() == 0
            )) {
                Timer timer = TimerManager::getInstance()->addTimer(_from, to, list[6],
                        list[1].toUInt(),
                        list[2].toUInt(),
                        list[3].toUInt(),
                        list[4].toUInt(),
                        list[5].toUInt());
                send(_command, to,
                     QString("[timer added] " + timer.getMessage() + " (expires the " +
                             timer.getEnd().toLocalTime().toString("dd-MM-yy") +" at " +
                             timer.getEnd().toLocalTime().toString("hh:mm:ss") +")")
                     );
                return;
            }
        }
        re.setPattern("^\\.t all$");
        if(re.exactMatch(_msg)) {
            const QMap<qint64, Timer> list = TimerManager::getInstance()->getTimerList();
            if(!list.empty()) {
                QMap<qint64, Timer>::ConstIterator citr;
                for(citr = list.cbegin(); citr != list.cend(); ++citr) {
                    if(citr.value().getTo() == to) {
                        QString expires_str("expires");
                        if(QDateTime::currentDateTime().daysTo(citr.value().getEnd().toLocalTime()) > 0) {
                            expires_str += " the " + citr.value().getEnd().toLocalTime().toString("dd-MM-yy")
                                    + " at " + citr.value().getEnd().toLocalTime().toString("hh:mm");
                        } else if(QDateTime::currentDateTimeUtc().secsTo(citr.value().getEnd()) > 900) {
                            expires_str += " at " + citr.value().getEnd().toLocalTime().toString("hh:mm");
                        } else if(QDateTime::currentDateTimeUtc().secsTo(citr.value().getEnd()) > 60) {
                            expires_str += " in " + QString("%1 min %2 secs")
                                    .arg((int)(QDateTime::currentDateTimeUtc().secsTo(citr.value().getEnd())/60))
                                    .arg((int)(QDateTime::currentDateTimeUtc().secsTo(citr.value().getEnd())%60));
                        } else {
                            expires_str += " in " + QString("%2 secs")
                                    .arg((int)(QDateTime::currentDateTimeUtc().secsTo(citr.value().getEnd())));
                        }
                        send(_command, to,
                             QString("#" + QString("%1").arg(citr.key()) + " - " + citr.value().getMessage() + " " + expires_str)
                             );
                    }
                }
            } else {
                send(_command, to, "No timer for this channel.");
            }
            return;
        }
        re.setPattern("^\\.t cancel #?(\\d+)$");
        if(re.exactMatch(_msg)) {
            QStringList list = re.capturedTexts();
            if(TimerManager::getInstance()->removeTimer(list[1].toLongLong()) > 0) {
                send(_command, to, QString("Timer cancelled."));
            } else {
                send(_command, to, QString("No timer with this key found!"));
            }
            return;
        }
        re.setPattern("^\\.t\\s.*");
        if(re.exactMatch(_msg)) {
            send(_command, to, QString("Invalid syntax: .t <time> <message>"));
            return;
        }
    }
    qDebug() << _from << " to " << _to << " [" << _command << "]: " << _msg;
}

void QIrc::onTimerFinished(const Timer& _timer) {
    send("PRIVMSG", _timer.getTo(),
         QString(_timer.getFrom() +
                 ": ding! " +
                 _timer.getMessage())
         );
}

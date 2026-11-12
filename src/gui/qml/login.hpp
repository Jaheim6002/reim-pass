#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>

class LogInQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool signedIn READ signedIn CONSTANT)

    Q_PROPERTY(QString firstPassphrase WRITE setFirst)
    Q_PROPERTY(QString secondPassphrase WRITE setSecond)

    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

    QML_ELEMENT

public:
    explicit LogInQml(QObject *parent = 0);

    bool signedIn() const {
        return m_signedIn;
    } 

    QString errorMessage() const {
        return m_errorMessage;
    }

public slots:
    void setFirst(const QString &value);
    void setSecond(const QString &value);

    void setError(const QString &value);

signals:
    void errorChanged();

private:
    bool m_signedIn = false;

    QString m_firstPassphrase;
    QString m_secondPassphrase;

    QString m_errorMessage = "";
};
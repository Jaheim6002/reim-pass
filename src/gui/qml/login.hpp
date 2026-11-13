#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include "../headers/common.hpp"

class LogInQml : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool registered READ registered CONSTANT)

    Q_PROPERTY(QString firstPassphrase WRITE setFirst)
    Q_PROPERTY(QString secondPassphrase WRITE setSecond)

    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

    QML_ELEMENT

public:
    explicit LogInQml(QObject *parent = nullptr) {}

    bool registered() const {
        return m_registered;
    } 

    QString errorMessage() const {
        return m_errorMessage;
    }

    // Functions

    Q_INVOKABLE bool tryLogIn();

public slots:
    void setFirst(const QString &value);
    void setSecond(const QString &value);

    void setError(const QString &value);

signals:
    void errorChanged();

private:
    bool m_registered = !AppContext::Get().firstRun;

    QString m_firstPassphrase;
    QString m_secondPassphrase;

    QString m_errorMessage = "";

    // Functions

    bool trySignUp();
};
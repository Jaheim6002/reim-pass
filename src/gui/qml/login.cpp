#include "login.hpp"

void LogInQml::setFirst (const QString &value) {
    if (value == m_firstPassphrase) {
        return;
    }
    m_firstPassphrase = value;
}

void LogInQml::setSecond (const QString &value) {
    if (value == m_secondPassphrase) {
        return;
    }
    m_secondPassphrase = value;
}

void LogInQml::setError (const QString &value) {
    if (value == m_errorMessage) {
        return;
    }
    m_errorMessage = value;
    emit errorChanged();
}
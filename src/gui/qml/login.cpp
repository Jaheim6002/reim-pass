#include "login.hpp"

bool LogInQml::tryLogIn () {
    if (AppContext::Get().firstRun) {
        return trySignUp();
    }

    Common comm;

    std::optional<std::string> enc = comm.selectPassword(AppContext::Get().data, comm.kVerifierId);
    if (!enc) {
        return false;
    }

    std::string received = m_firstPassphrase.toStdString();
    std::string plain;

    if (!comm.decryptPassword(*enc, received, plain)) {
        setError("Wrong passphrase, try again");
        return false;
    }

    setError("");
    AppContext::Get().passphrase = received;
    return true;
}

bool LogInQml::trySignUp () {
    if (m_firstPassphrase != m_secondPassphrase) {
        setError("Passphrases are not the same");
        return false;
    }
    std::string received = m_firstPassphrase.toStdString();

    if (received == "") {
        setError("This field cannot be empty");
        return false;
    }

    Common comm;
    std::string verifier = comm.encryptPassword("reimpass-verifier", received);

    if (!comm.insertSQL(AppContext::Get().data, "reimpass", "reimpass.user", verifier)) {
        setError("SQL Error");
        return false;
    }

    AppContext::Get().passphrase = received;
    return true;
}

// Setters

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
#include "login.hpp"

bool LogInQml::tryLogIn () {
    if (AppContext::Get().firstRun) {
        return trySignUp();
    }

    Common comm;

    std::optional<std::string> enc = comm.selectPassword(AppContext::Get().data, comm.kVerifierId);
    if (!enc) {
        qDebug() << "LogInQml::tryLogIn — Logging In: Failed: Verifier is empty, the database may be corrupted"; // idfk what would have to happen
        return false;
    }

    std::string received = m_firstPassphrase.toStdString();
    std::string plain;

    if (!comm.decryptPassword(*enc, received, plain)) {
        setError("Wrong passphrase, try again");
        return false;
    }

    qDebug() << "LogInQml::tryLogIn — Logging In: Success!";

    setError("");
    AppContext::Get().passphrase = received;
    return true;
}

bool LogInQml::trySignUp () {
    qDebug() << "LogInQml::trySignUp — Signing up: Started!";
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
        qWarning() << "LogInQml::trySignUp — Saving Passphrase: SQL Error: " << sqlite3_errmsg(AppContext::Get().data);
        return false;
    }

    qDebug() << "LogInQml::trySignUp — Saving Passphrase: Success!";

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
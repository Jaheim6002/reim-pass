#pragma once
#include <qobject.h>
#include <QtQml/qqmlregistration.h>
#include <qtmetamacros.h>
#include "loginList.hpp"
#include "../headers/common.hpp"

class BrowseData : public QObject {
    Q_OBJECT

    Q_PROPERTY(LoginList* list READ list NOTIFY listChanged)
    Q_PROPERTY(QString site READ site NOTIFY siteChanged)
    Q_PROPERTY(QString login READ login NOTIFY loginChanged)
    Q_PROPERTY(QString password READ password NOTIFY passwordChanged)

    QML_ELEMENT
public:
    BrowseData(QObject* = 0) {}

    // Getters
    LoginList* list() {
        return &m_list;
    }
    QString site() {
        return m_site;
    }
    QString login() {
        return m_login;
    }
    QString password() {
        return m_password;
    }

    // Setters
    void setSite(QString value);
    void setLogin(QString value);
    void setPassword(QString value);

    // Functions
    Q_INVOKABLE void SelectList ();
    Q_INVOKABLE void SelectPassword (int id);
    Q_INVOKABLE void InsertData (QString site, QString login, QString password);
    Q_INVOKABLE void DeleteData (int id);
    Q_INVOKABLE void UpdateData (int id, QString site, QString login, QString password);
signals:
    void listChanged();
    void siteChanged();
    void loginChanged();
    void passwordChanged();
private:
    QString m_filter = "";
    LoginList m_list;

    // Selected
    QString m_site = "default"; // If you see this value in QML, something does not work properly
    QString m_login = "default";
    QString m_password = "default";
};
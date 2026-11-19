#pragma once
#include <QAbstractListModel>
#include <QDate>
#include <QtQml/qqmlregistration.h>
#include <qabstractitemmodel.h>
#include <qobject.h>

class Entry {
public:
    Entry(const int& id, const QString& site, const QString& login, const QDate& date);
    
    int id() const{
        return m_id;
    }

    QString site() const{
        return m_site;
    }

    QString login() const{
        return m_login;
    }

    QDate date() const{
        return m_date;
    }

private:
    int m_id;
    QString m_site; 
    QString m_login;
    QDate m_date;
};

class LoginList : public QAbstractListModel {
    Q_OBJECT

    QML_ELEMENT
    QML_UNCREATABLE("LoginList is created in C++")

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        SiteRole,
        LoginRole,
        DateRole
    };

    QHash<int, QByteArray> roleNames() const {
        return {
            { IdRole, "selectedId"},
            { SiteRole, "site"},
            { LoginRole, "login"},
            { DateRole, "date"}
        };
    }

    LoginList(QObject* = 0) {}
    int rowCount(const QModelIndex& = QModelIndex()) const {
        return m_entries.size();
    }
    int columnCount(const QModelIndex& = QModelIndex()) const {
        return 3;
    }
    QVariant data(const QModelIndex &index, int role) const;
    void clear() {
        m_entries.clear();
    }
    void append(int id, QString site, QString login, QDate date);
 private:
    QList<Entry> m_entries;
};
#include "loginList.hpp"

Entry::Entry(const int& id, const QString& site, const QString& login, const QDate& date) {
    m_id = id;
    m_site = site;
    m_login = login;
    m_date = date;
}

QVariant LoginList::data (const QModelIndex &index, int role) const {
    if (!index.isValid()) {
       return {};
    }
    const auto &entry = m_entries.at(index.row());
    switch (role) {
        case IdRole:
            return entry.id();
        case SiteRole: 
            return entry.site();
        case LoginRole:
            return entry.login();
        case DateRole:
            return entry.date();
    }
    return {};
}

void LoginList::append (int id, QString site, QString login, QDate date) {
    beginResetModel();
    m_entries.append(Entry(id, site, login, date));
    endResetModel();
}

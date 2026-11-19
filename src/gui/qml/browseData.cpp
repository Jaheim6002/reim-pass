#include "browseData.hpp"
#include "core/headers/context.hpp"

void BrowseData::setSite(QString value) {
    m_site = value;
    emit siteChanged();
}


void BrowseData::setLogin(QString value) {
    m_login = value;
    emit loginChanged();
}


void BrowseData::setPassword(QString value) {
    m_password = value;
    emit passwordChanged();
}


void BrowseData::SelectList() {
    m_list.clear();

    const char* selectQuery =
        "SELECT id, site, login, last FROM login_data "
        "WHERE (site LIKE ? OR login LIKE ?) "
        "AND id != ? AND deleted != 1 "
        "ORDER BY site DESC, last ASC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(AppContext::Get().data, selectQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "BrowseData::SelectList — SQL Error: " << sqlite3_errmsg(AppContext::Get().data);
        return;
    }

    Common cm;
    std::string filter = m_filter.toStdString();

    std::string pattern = "%" + filter + "%";
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, cm.kVerifierId); // hide the verifier row

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        QString site = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        QString login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        QDate date = QDate::fromString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)), Qt::ISODate);

        m_list.append(id, site, login, date);
    }

    qDebug() << "BrowseData::SelectList — login data list SELECT: Success!";
    emit listChanged();
}


void BrowseData::SelectPassword(int id) {
    const char* selectQuery =
        "SELECT site, login, password, last FROM login_data "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(AppContext::Get().data, selectQuery, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "BrowseData::SelectPassword — SQL Error: " << sqlite3_errmsg(AppContext::Get().data);
        return;
    }

    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        setSite(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        setLogin(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));

        const void* blob = sqlite3_column_blob(stmt, 2);
        int len = sqlite3_column_bytes(stmt, 2);
        std::string enc = std::string(static_cast<const char*>(blob), len);

        Common cm;
        std::string plain;

        cm.decryptPassword(enc, AppContext::Get().passphrase, plain);
        setPassword(QString::fromStdString(plain));
    }

    qDebug() << "BrowseData::SelectPassword — login data " << id << " SELECT: Success!";
    sqlite3_finalize(stmt);
}


void BrowseData::InsertData(QString site, QString login, QString password) {
    std::string std_site = site.toStdString();
    std::string std_login = login.toStdString();
    std::string std_password = password.toStdString();

    Common cm;

    std_password = cm.encryptPassword(std_password, AppContext::Get().passphrase);

    bool ok = cm.insertSQL(AppContext::Get().data, std_site, std_login, std_password);

    if (ok) {
        qDebug() << "BrowseData::InsertData — login data INSERT: Success!";
    } else {
        qDebug() << "BrowseData::InsertData — login data INSERT: Failed: SQL Error";
    }
}


void BrowseData::UpdateData(int id, QString site, QString login, QString password) {
    std::string std_site = site.toStdString();
    std::string std_login = login.toStdString();
    std::string std_password = password.toStdString();

    Common cm;

    std_password = cm.encryptPassword(std_password, AppContext::Get().passphrase);

    bool ok = cm.updateSQL(AppContext::Get().data, id, std_site, std_login, std_password);

    if (ok) {
        qDebug() << "BrowseData::UpdateData — login data UPDATE: Success!";
    } else {
        qDebug() << "BrowseData::UpdateData — login data UPDATE: Failed: SQL Error";
    }
}


void BrowseData::DeleteData(int id) {
    Common cm;

    bool ok = cm.deleteSQL(AppContext::Get().data, id);

    if (ok) {
        qDebug() << "BrowseData::DeleteData — login data DELETE: Success!";
    } else {
        qDebug() << "BrowseData::DeleteData — login data DELETE: Failed: SQL Error";
    }
}
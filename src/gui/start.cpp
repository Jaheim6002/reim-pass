#include "headers/common.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QDebug>
#include <qnamespace.h>
#include <qqmlapplicationengine.h>

int main (int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    AppContext::Get().initApp();
    fs::path directory = AppContext::Get().directory;

    fs::path dbPath = directory / "logindata.db";
    AppContext::Get().firstRun = !fs::exists(dbPath);

    if (sqlite3_open(dbPath.string().c_str(), &AppContext::Get().data) != SQLITE_OK) {
        sqlite3_close(AppContext::Get().data);
        return 1;
    }

    if(AppContext::Get().firstRun) {
        const char* loginDataTable =
            "CREATE TABLE IF NOT EXISTS login_data ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "uid TEXT NOT NULL UNIQUE,"
                "site TEXT NOT NULL,"
                "login TEXT NOT NULL,"
                "password BLOB NOT NULL,"
                "last TEXT NOT NULL DEFAULT (strftime('%Y-%m-%d %H:%M','now')),"
                "deleted INTEGER NOT NULL DEFAULT 0"
            ");";

        char* error = nullptr;
        if (sqlite3_exec(AppContext::Get().data, loginDataTable, nullptr, nullptr, &error) != SQLITE_OK) {
            sqlite3_free(error);
            sqlite3_close(AppContext::Get().data);
            return 1;
        }
    }

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    engine.loadFromModule("ReimPass", "StartWindow");

    return app.exec();
}
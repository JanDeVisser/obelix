/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

//#include <QCompleter>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMutex>
#include <QProxyStyle>
#include <QStyleOption>
#include <QWaitCondition>

namespace Obelix::JV80::GUI {

class CommandDefinition;
class Command;
class CommandLineEdit;

typedef std::function<void(Command&)> CommandHandler;
typedef std::function<QVector<QString>(const QStringList&)> Completer;

class CommandDefinition {
private:
    QString m_command = "";
    int m_minArgs = 0;
    int m_maxArgs = 0;
    CommandHandler m_handler = nullptr;
    Completer m_completer = nullptr;

public:
    CommandDefinition() = default;
    CommandDefinition(CommandLineEdit*, QString&&, int, int, CommandHandler, Completer = nullptr);
    CommandDefinition(CommandLineEdit*, QString&, int, int, CommandHandler, Completer = nullptr);

    void changed(const QStringList&) const;
    void submit(Command&) const;
    [[nodiscard]] QVector<QString> complete(const QStringList&) const;
    [[nodiscard]] const QString& command() const { return m_command; }
    [[nodiscard]] int minArgs() const { return m_minArgs; }
    [[nodiscard]] int maxArgs() const { return m_maxArgs; }
};

class Command {
    CommandDefinition m_definition = CommandDefinition();
    QString m_line = "";
    QString m_command = "";
    QStringList m_args = QStringList();
    QString m_result = "";
    bool m_success = true;

public:
    explicit Command(CommandLineEdit&);
    void setError(QString const&);
    void setResult(QString const&);
    void setSuccess();

    const CommandDefinition& definition() const { return m_definition; }
    QString line() const { return m_line; }
    QString command() const { return m_command; }
    int numArgs() const { return m_args.size(); }
    QString arg(int ix) const { return m_args[ix]; }
    QString result() const { return m_result; }
    bool success() const { return m_success; }
};

class CommandLineEdit : public QLineEdit {
    Q_OBJECT

private:
    QHash<QString, CommandDefinition> m_commands;
    QVector<QString> m_completions;
    int m_currentCompletion = -1;
    QString m_typed = "";
    QStringList m_history;
    int m_historyIndex = -1;
    QString m_query = "";

public:
    explicit CommandLineEdit(QWidget* parent = nullptr);
    void addCommandDefinition(CommandLineEdit*, QString&&, int, int, CommandHandler, Completer = nullptr);
    void addCommandDefinition(CommandLineEdit*, QString&, int, int, CommandHandler, Completer = nullptr);
    void addCommandDefinition(CommandDefinition&& definition);
    void addCommandDefinition(CommandDefinition& definition);
    const CommandDefinition& findDefinition(QString&, bool& ok);
    QVector<QString> findCommands(QString&) const;
    QVector<QString> findCommands(QString&& cmd) const { return findCommands(cmd); }
    QVector<QString> findCompletions(QString&) const;
    QVector<QString> findCompletions(QString&& cmd) const { return findCompletions(cmd); }
    QString query(const QString&, const QString&);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    bool focusNextPrevChild(bool) override { return false; }

signals:
    void result(const QString&, bool, const QString&);
    void queryResult(const QString&);

private slots:
    void submitted();
    void edited(const QString&);
};

}

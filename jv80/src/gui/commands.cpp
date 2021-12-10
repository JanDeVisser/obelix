/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <QEventLoop>
#include <qlogging.h>

#include <gui/blockcursorstyle.h>
#include <gui/commands.h>

namespace Obelix::JV80::GUI {

Command::Command(CommandLineEdit& edit)
    : m_line(edit.text())
{
    m_args = m_line.split(" ", Qt::SkipEmptyParts);
    if (m_args.isEmpty()) {
        setError("Syntax error: no command");
    } else {
        m_command = m_args[0].toLower();
        bool ok;
        m_definition = edit.findDefinition(m_command, ok);
        if (!ok) {
            setError(QString("Syntax error: unknown command '%1'").arg(m_command));
            return;
        }
        m_args.removeAt(0);
        if (numArgs() < m_definition.minArgs()) {
            setError(QString("Syntax error: expected at least %1 arguments").arg(m_definition.minArgs()));
            return;
        }
        if ((m_definition.maxArgs() >= m_definition.minArgs()) && (numArgs() > m_definition.maxArgs())) {
            setError(QString("Syntax error: expected at most %1 arguments").arg(m_definition.minArgs()));
            return;
        }
    }
}

void Command::setError(QString&& err)
{
    m_result = err;
    m_success = false;
}

void Command::setError(QString& err)
{
    m_result = err;
    m_success = false;
}

void Command::setResult(QString&& res)
{
    m_result = res;
    m_success = true;
}

void Command::setResult(QString& res)
{
    m_result = res;
    m_success = true;
}

void Command::setSuccess()
{
    setResult("");
}

/* ----------------------------------------------------------------------- */

CommandDefinition::CommandDefinition(CommandLineEdit* edit, QString&& command,
    int minArgs, int maxArgs,
    CommandHandler handler, Completer completer)
    : CommandDefinition(edit, command, minArgs, maxArgs,
        std::move(handler), std::move(completer))
{
}

CommandDefinition::CommandDefinition(CommandLineEdit* edit, QString& command,
    int minArgs, int maxArgs,
    CommandHandler handler, Completer completer)
    : m_edit(edit)
    , m_command(command)
    , m_minArgs(minArgs)
    , m_maxArgs(maxArgs)
    , m_handler(std::move(handler))
    , m_completer(std::move(completer))
{
}

void CommandDefinition::changed(const QStringList& args) const
{
    //
}

void CommandDefinition::submit(Command& cmd) const
{
    m_handler(cmd);
}

QVector<QString> CommandDefinition::complete(const QStringList& args) const
{
    QVector<QString> ret;
    if (m_completer) {
        ret = m_completer(args);
    }
    return ret;
}

/* ----------------------------------------------------------------------- */

CommandLineEdit::CommandLineEdit(QWidget* parent)
    : QLineEdit(parent)
    , m_commands()
    , m_completions()
    , m_history()
{
    setStyle(new BlockCursorStyle(style()));
    connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(edited(const QString&)));
    //  connect(this, SIGNAL(returnPressed()), this, SLOT(submitted()));
}

void CommandLineEdit::addCommandDefinition(CommandLineEdit* edit, QString&& command,
    int minArgs, int maxArgs,
    CommandHandler handler,
    Completer completer)
{
    addCommandDefinition(CommandDefinition(edit, command,
        minArgs, maxArgs,
        std::move(handler), std::move(completer)));
}

void CommandLineEdit::addCommandDefinition(CommandLineEdit* edit, QString& command,
    int minArgs, int maxArgs,
    CommandHandler handler, Completer completer)
{
    addCommandDefinition(CommandDefinition(edit, command,
        minArgs, maxArgs,
        std::move(handler), std::move(completer)));
}

void CommandLineEdit::addCommandDefinition(CommandDefinition&& definition)
{
    addCommandDefinition(definition);
}

void CommandLineEdit::addCommandDefinition(CommandDefinition& definition)
{
    m_commands[definition.command()] = definition;
}

void CommandLineEdit::edited(const QString& text)
{
    auto args = text.split(" ", Qt::SkipEmptyParts);
    if (args.isEmpty()) {
        return;
    }
    bool ok;
    auto definition = findDefinition(args[0], ok);
    if (!ok) {
        return;
    }
    definition.changed(args);
}

void CommandLineEdit::submitted()
{
    QString cmd = text();
    m_historyIndex = -1;
    m_currentCompletion = -1;
    m_completions.clear();
    m_history.push_front(cmd);

    Command c(*this);
    if (c.success()) {
        c.definition().submit(c);
    }

    emit result(cmd, c.success(), c.result());
    setText("");
}

QString CommandLineEdit::query(const QString& prompt, const QString& options)
{
    setText(prompt + " ");
    m_query = options;

    QEventLoop loop;
    QObject::connect(this, &CommandLineEdit::queryResult,
        [this, &loop](const QString& chosen) {
            std::cout << "option " << chosen.toStdString() << " chosen" << std::endl;
            m_query = chosen;
            loop.quit();
        });
    loop.exec();
    return m_query;
}

const CommandDefinition& CommandLineEdit::findDefinition(QString& cmd, bool& ok)
{
    static CommandDefinition ret = CommandDefinition();
    QVector<QString> candidates = findCommands(cmd);
    if (candidates.size() == 1) {
        ok = true;
        return m_commands[candidates[0]];
    } else {
        ok = false;
        return ret;
    }
}

QVector<QString> CommandLineEdit::findCommands(QString& cmd) const
{
    QVector<QString> ret;
    for (auto& command : m_commands.keys()) {
        if (command.toLower().startsWith(cmd.toLower())) {
            ret.push_back(command);
        }
    }
    return ret;
}

QVector<QString> CommandLineEdit::findCompletions(QString& cmd) const
{
    QVector<QString> ret;
    auto args = cmd.split(" ", Qt::SkipEmptyParts);
    if (args.isEmpty()) {
        return ret;
    }
    ret = findCommands(args[0]);
    if ((ret.size() == 1) && (args.size() > 1)) {
        auto def = m_commands[ret[0]];
        ret = def.complete(args);
    }
    return ret;
}

void CommandLineEdit::keyPressEvent(QKeyEvent* e)
{
    if (!m_query.isEmpty()) {
        if (m_query.toLower().contains(e->text().toLower())) {
            emit queryResult(e->text().toLower());
        }
    }

    switch (e->key()) {
    case Qt::Key_Shift:
        if (m_currentCompletion >= 0) {
            return;
        }
        break;
    case Qt::Key_Backtab:
        m_historyIndex = -1;
        if (m_currentCompletion == -1) {
            m_typed = text();
            m_completions = findCompletions(m_typed);
        }
        if (m_currentCompletion > 0) {
            setText(m_completions[--m_currentCompletion]);
            return;
        } else if ((m_currentCompletion <= 0) && !m_completions.isEmpty()) {
            m_currentCompletion = m_completions.size() - 1;
            setText(m_completions[m_currentCompletion]);
            return;
        }
        break;
    case Qt::Key_Tab:
        m_historyIndex = -1;
        if (m_currentCompletion == -1) {
            m_typed = text();
            m_completions = findCompletions(m_typed);
        }
        if (m_currentCompletion + 1 < m_completions.size()) {
            setText(m_completions[++m_currentCompletion]);
            return;
        } else if (!m_completions.isEmpty()) {
            m_currentCompletion = 0;
            setText(m_completions[m_currentCompletion]);
            return;
        }
        break;
    case Qt::Key_Escape:
        m_historyIndex = -1;
        if (m_currentCompletion >= 0) {
            setText(m_typed);
            m_completions.clear();
            m_currentCompletion = -1;
        } else {
            clearFocus();
        }
        return;
    case Qt::Key_Up:
        if (m_historyIndex + 1 < m_history.size()) {
            if (m_currentCompletion >= 0) {
                m_completions.clear();
                m_currentCompletion = -1;
            }
            setText(m_history[++m_historyIndex]);
            return;
        }
        break;
    case Qt::Key_Down:
        if (m_historyIndex > 0) {
            setText(m_history[--m_historyIndex]);
            return;
        } else if (m_historyIndex == 0) {
            setText("");
            --m_historyIndex;
            return;
        }
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        submitted();
        return;
    default:
        if (m_currentCompletion >= 0) {
            m_typed = text(); // ? Maybe
            m_completions.clear();
            m_currentCompletion = -1;
        }
        m_historyIndex = -1;
        break;
    }
    QLineEdit::keyPressEvent(e);
}

/* ----------------------------------------------------------------------- */

}

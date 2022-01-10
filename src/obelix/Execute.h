/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

#define ENUMERATE_FLOWCONTROLS(S) \
    S(None)                       \
    S(Break)                      \
    S(Continue)                   \
    S(Return)                     \
    S(Goto)                       \
    S(Label)                      \
    S(Skipped)

enum class FlowControl {
#undef __ENUM_FLOWCONTROL
#define __ENUM_FLOWCONTROL(fc) fc,
    ENUMERATE_FLOWCONTROLS(__ENUM_FLOWCONTROL)
#undef __ENUM_FLOWCONTROL
};

constexpr std::string_view FlowControl_name(FlowControl fc)
{
    switch (fc) {
#undef __ENUM_FLOWCONTROL
#define __ENUM_FLOWCONTROL(fc) \
    case FlowControl::fc:      \
        return #fc;
        ENUMERATE_FLOWCONTROLS(__ENUM_FLOWCONTROL)
#undef __ENUM_FLOWCONTROL
    }
}

class StatementExecutionResult : public SyntaxNode {
public:
    explicit StatementExecutionResult(Obj result = Object::null(), FlowControl flow_control = FlowControl::None)
        : SyntaxNode()
        , m_flow_control(flow_control)
        , m_result(std::move(result))
    {
    }

    [[nodiscard]] std::string to_string(int) const override { return format("{} [{}]", result()->to_string(), FlowControl_name(flow_control())); }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::StatementExecutionResult; }

    [[nodiscard]] FlowControl flow_control() const { return m_flow_control; }
    [[nodiscard]] Obj const& result() const { return m_result; }

private:
    FlowControl m_flow_control;
    Obj m_result;
};

inline std::shared_ptr<StatementExecutionResult> execution_ok()
{
    static std::shared_ptr<StatementExecutionResult> s_ok = std::make_shared<StatementExecutionResult>();
    return s_ok;
}

inline std::shared_ptr<StatementExecutionResult> execution_evaluates_to(Obj return_value)
{
    return std::make_shared<StatementExecutionResult>(std::move(return_value));
}

inline std::shared_ptr<StatementExecutionResult> return_result(Obj return_value)
{
    return std::make_shared<StatementExecutionResult>(std::move(return_value), FlowControl::Return);
}

inline std::shared_ptr<StatementExecutionResult> break_loop()
{
    static std::shared_ptr<StatementExecutionResult> s_break = std::make_shared<StatementExecutionResult>(Object::null(), FlowControl::Break);
    return s_break;
}

inline std::shared_ptr<StatementExecutionResult> continue_loop()
{
    static std::shared_ptr<StatementExecutionResult> s_continue = std::make_shared<StatementExecutionResult>(Object::null(), FlowControl::Continue);
    return s_continue;
}

inline std::shared_ptr<StatementExecutionResult> skip_block()
{
    static std::shared_ptr<StatementExecutionResult> s_skipped = std::make_shared<StatementExecutionResult>(Object::null(), FlowControl::Skipped);
    return s_skipped;
}

inline std::shared_ptr<StatementExecutionResult> goto_label(int id)
{
    return std::make_shared<StatementExecutionResult>(make_obj<Integer>(id), FlowControl::Goto);
}

inline std::shared_ptr<StatementExecutionResult> mark_label(int id)
{
    return std::make_shared<StatementExecutionResult>(make_obj<Integer>(id), FlowControl::Label);
}

template<typename Context>
ErrorOrNode process_node(std::shared_ptr<SyntaxNode> const& tree, Context& ctx)
{
    ErrorOrNode ret_or_error = ctx.process(tree);
    if (!ret_or_error.is_error()) {
        auto ret = ret_or_error.value();
        if (auto processor_maybe = ctx.processor_for(SyntaxNodeType::Statement); processor_maybe.has_value()) {
            auto new_tree = TRY(ctx.add_if_error(processor_maybe.value()(ret, ctx)));
        }
        return ret;
    } else {
        debug(parser, "process_tree returns error: {}", ret_or_error.error().message());
        return ret_or_error.error();
    }
}

ErrorOrNode execute(std::shared_ptr<SyntaxNode> const&);
ErrorOrNode execute(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& root);

}

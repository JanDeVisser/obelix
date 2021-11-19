/*
 * Processor.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

template <class NodeClass, typename... Args>
std::shared_ptr<SyntaxNode> process_node(ProcessorMap const& map, Args&&... args)
{
    auto new_tree = std::make_shared<NodeClass>(std::forward<Args>(args)...);
    if (!new_tree)
        return new_tree;
    if (map.contains(new_tree->node_type())) {
        return map.at(new_tree->node_type())(new_tree);
    }
    return new_tree;
}

template <class BranchClass, typename... Args>
std::shared_ptr<SyntaxNode> process_branch(std::shared_ptr<SyntaxNode> const& tree, ProcessorMap const& map, Args&&... args)
{
    auto branch = std::dynamic_pointer_cast<Branch>(tree);
    auto condition = std::dynamic_pointer_cast<Expression>(process_tree(branch->condition(), map));
    auto statement = std::dynamic_pointer_cast<Statement>(process_tree(branch->statement(), map));
    return process_node<BranchClass>(map, condition, statement, std::forward<Args>(args)...);
}

std::shared_ptr<SyntaxNode> process_tree(std::shared_ptr<SyntaxNode> const& tree, ProcessorMap const& map)
{
    ProcessorMap forward_map;
    auto xform_block = [map](Statements const& block)
    {
        Statements ret;
        for (auto& stmt : block) {
            ret.push_back(std::dynamic_pointer_cast<Statement>(process_tree(stmt, map)));
        }
        return ret;
    };

    auto xform_expressions = [map](Expressions const& expressions)
    {
        Expressions ret;
        for (auto& expr : expressions) {
            ret.push_back(std::dynamic_pointer_cast<Expression>(process_tree(expr, map)));
        }
        return ret;
    };

    if (!tree)
        return tree;

    switch (tree->node_type()) {
    case SyntaxNodeType::Block: {
        auto block = std::dynamic_pointer_cast<Block>(tree);
        return process_node<Block>(map, xform_block(block->statements()));
    }
    case SyntaxNodeType::Module: {
        auto mod = std::dynamic_pointer_cast<Module>(tree);
        return process_node<Module>(map, mod->name(), xform_block(mod->statements()));
    }
    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto statement = std::dynamic_pointer_cast<Statement>(process_tree(func_def->statement(), map));
        return process_node<FunctionDef>(map, func_def->name(), func_def->parameters(), statement);
    }
    case SyntaxNodeType::ExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        auto expr = std::dynamic_pointer_cast<Expression>(process_tree(stmt->expression(), map));
        return process_node<ExpressionStatement>(map, expr);
    }
    case SyntaxNodeType::ListLiteral: {
        auto list_literal = std::dynamic_pointer_cast<ListLiteral>(tree);
        auto elements = xform_expressions(list_literal->elements());
        return process_node<ListLiteral>(map, elements);
    }
    case SyntaxNodeType::ListComprehension: {
        auto comprehension = std::dynamic_pointer_cast<ListComprehension>(tree);
        auto element_expr = std::dynamic_pointer_cast<Expression>(process_tree(comprehension->element(), map));
        auto generator = std::dynamic_pointer_cast<Expression>(process_tree(comprehension->generator(), map));
        auto condition = std::dynamic_pointer_cast<Expression>(process_tree(comprehension->condition(), map));
        return process_node<ListComprehension>(map, element_expr, comprehension->rangevar(), generator, condition);
    }
    case SyntaxNodeType::DictionaryLiteral: {
        auto dict_literal = std::dynamic_pointer_cast<DictionaryLiteral>(tree);
        DictionaryLiteralEntries entries;
        for (auto& entry : dict_literal->entries()) {
            entries.push_back({ entry.name, std::dynamic_pointer_cast<Expression>(process_tree(entry.value, map)) });
        }
        return process_node<DictionaryLiteral>(map, entries);
    }
    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto lhs = std::dynamic_pointer_cast<Expression>(process_tree(expr->lhs(), map));
        auto rhs = std::dynamic_pointer_cast<Expression>(process_tree(expr->rhs(), map));
        return process_node<BinaryExpression>(map, lhs, expr->op(), rhs);
    }
    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = std::dynamic_pointer_cast<Expression>(process_tree(expr->operand(), map));
        return process_node<UnaryExpression>(map, expr->op(), operand);
    }
    case SyntaxNodeType::FunctionCall: {
        auto func_call = std::dynamic_pointer_cast<FunctionCall>(tree);
        auto func = std::dynamic_pointer_cast<Expression>(process_tree(func_call->function(), map));
        auto arguments = xform_expressions(func_call->arguments());
        return process_node<FunctionCall>(map, func, arguments);
    }
    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto expr = std::dynamic_pointer_cast<Expression>(process_tree(var_decl->expression(), map));
        return process_node<VariableDeclaration>(map, var_decl->variable(), expr);
    }
    case SyntaxNodeType::Return: {
        auto ret = std::dynamic_pointer_cast<Return>(tree);
        auto expr = std::dynamic_pointer_cast<Expression>(process_tree(ret->expression(), map));
        return process_node<Return>(map, expr);
    }
    case SyntaxNodeType::Branch: {
        return process_branch<Branch>(tree, map);
    }
    case SyntaxNodeType::ElseStatement: {
        return process_branch<ElseStatement>(tree, map);
    }
    case SyntaxNodeType::ElifStatement: {
        return process_branch<ElifStatement>(tree, map);
    }
    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        ElifStatements elifs;
        for (auto& elif : if_stmt->elifs()) {
            elifs.push_back(std::dynamic_pointer_cast<ElifStatement>(process_tree(elif, map)));
        }
        auto else_stmt = std::dynamic_pointer_cast<ElseStatement>(process_tree(if_stmt->else_stmt(), map));
        return process_branch<IfStatement>(tree, map, elifs, else_stmt);
    }
    case SyntaxNodeType::WhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        auto condition = std::dynamic_pointer_cast<Expression>(process_tree(while_stmt->condition(), map));
        auto stmt = std::dynamic_pointer_cast<Statement>(process_tree(while_stmt->statement(), map));
        return process_node<WhileStatement>(map, condition, stmt);
    }
    case SyntaxNodeType::ForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        auto range = std::dynamic_pointer_cast<Expression>(process_tree(for_stmt->range(), map));
        auto stmt = std::dynamic_pointer_cast<Statement>(process_tree(for_stmt->statement(), map));
        return process_node<ForStatement>(map, for_stmt->variable(), range, stmt);
    }
    case SyntaxNodeType::CaseStatement: {
        return process_branch<CaseStatement>(tree, map);
    }
    case SyntaxNodeType::DefaultCase: {
        return process_branch<DefaultCase>(tree, map);
    }
    case SyntaxNodeType::SwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
        auto expr = std::dynamic_pointer_cast<Expression>(process_tree(switch_stmt->expression(), map));
        CaseStatements cases;
        for (auto& case_stmt : switch_stmt->cases()) {
            cases.push_back(std::dynamic_pointer_cast<CaseStatement>(process_tree(case_stmt, map)));
        }
        auto default_case = std::dynamic_pointer_cast<DefaultCase>(process_tree(switch_stmt->default_case(), map));
        return process_node<SwitchStatement>(map, expr, cases, default_case);
    }
    default:
        return tree;
    }
}

std::shared_ptr<SyntaxNode> fold_constants(std::shared_ptr<SyntaxNode> const& tree)
{
    ProcessorMap fold_constants_map;
    fold_constants_map[SyntaxNodeType::BinaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree) -> std::shared_ptr<SyntaxNode>
    {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        if ((expr->lhs()->node_type() == SyntaxNodeType::Literal) && (expr->rhs()->node_type() == SyntaxNodeType::Literal)) {
            auto scope = make_typed<Scope>();
            return std::make_shared<Literal>(expr->evaluate(scope));
        }
        return tree;
    };
    fold_constants_map[SyntaxNodeType::UnaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree) -> std::shared_ptr<SyntaxNode>
    {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        if (expr->operand()->node_type() == SyntaxNodeType::Literal) {
            auto scope = make_typed<Scope>();
            return std::make_shared<Literal>(expr->evaluate(scope));
        }
        return tree;
    };

    return process_tree(tree, fold_constants_map);
}




}
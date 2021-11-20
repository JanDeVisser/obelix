/*
 * BindStatements.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

std::shared_ptr<SyntaxNode> bind_types(std::shared_ptr<SyntaxNode> const& tree)
{
    ProcessorMap bind_types_map;
    bind_types_map[SyntaxNodeType::Literal] = [](std::shared_ptr<SyntaxNode> const& tree) -> std::shared_ptr<SyntaxNode>
    {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        return std::make_shared<TypedExpression>(literal, ObelixType_of(literal->literal()));
    };
    bind_types_map[SyntaxNodeType::ListLiteral] = [](std::shared_ptr<SyntaxNode> const& tree) -> std::shared_ptr<SyntaxNode>
    {
        auto expr = std::dynamic_pointer_cast<Expression>(tree);
        return std::make_shared<TypedExpression>(expr, ObelixType::List);
    };
    bind_types_map[SyntaxNodeType::ListComprehension] = bind_types_map[SyntaxNodeType::ListLiteral];
    bind_types_map[SyntaxNodeType::DictionaryLiteral] = [](std::shared_ptr<SyntaxNode> const& tree) -> std::shared_ptr<SyntaxNode>
    {
        auto expr = std::dynamic_pointer_cast<Expression>(tree);
        return std::make_shared<TypedExpression>(expr, ObelixType::Object);
    };
    return process_tree(tree, bind_types_map);
}

}
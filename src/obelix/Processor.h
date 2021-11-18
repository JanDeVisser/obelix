/*
 * Processor.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>

#include <obelix/Syntax.h>

namespace Obelix {

typedef std::unordered_map<SyntaxNodeType, std::function<std::shared_ptr<Obelix::SyntaxNode>(std::shared_ptr<Obelix::SyntaxNode> const&)>> ProcessorMap;

std::shared_ptr<SyntaxNode> process_tree(std::shared_ptr<SyntaxNode> const&, ProcessorMap const&);
std::shared_ptr<SyntaxNode> fold_constants(std::shared_ptr<SyntaxNode> const&);

}

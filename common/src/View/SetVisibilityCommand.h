/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Macros.h"
#include "View/UndoableCommand.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace TrenchBroom::Model
{
class Node;
enum class VisibilityState;
} // namespace TrenchBroom::Model

namespace TrenchBroom::View
{

class SetVisibilityCommand : public UndoableCommand
{
private:
  enum class Action;

  std::vector<Model::Node*> m_nodes;
  Action m_action;
  std::map<Model::Node*, Model::VisibilityState> m_oldState;

public:
  static std::unique_ptr<SetVisibilityCommand> show(std::vector<Model::Node*> nodes);
  static std::unique_ptr<SetVisibilityCommand> hide(std::vector<Model::Node*> nodes);
  static std::unique_ptr<SetVisibilityCommand> ensureVisible(
    std::vector<Model::Node*> nodes);
  static std::unique_ptr<SetVisibilityCommand> reset(std::vector<Model::Node*> nodes);

  SetVisibilityCommand(std::vector<Model::Node*> nodes, Action action);

private:
  static std::string makeName(Action action);

  std::unique_ptr<CommandResult> doPerformDo(MapDocumentCommandFacade& document) override;
  std::unique_ptr<CommandResult> doPerformUndo(
    MapDocumentCommandFacade& document) override;

  deleteCopyAndMove(SetVisibilityCommand);
};

} // namespace TrenchBroom::View

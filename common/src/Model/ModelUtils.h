/*
 Copyright (C) 2010-2017 Kristian Duske

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

#include "FloatType.h"
#include "Model/Brush.h"
#include "Model/BrushFaceHandle.h"
#include "Model/BrushNode.h"
#include "Model/EntityNode.h"
#include "Model/GroupNode.h"
#include "Model/HitType.h"
#include "Model/LayerNode.h"
#include "Model/Node.h"
#include "Model/PatchNode.h"
#include "Model/WorldNode.h"

#include <kdl/overload.h>

#include <vecmath/bbox.h>

#include <map>
#include <memory>
#include <vector>

namespace TrenchBroom::Model
{

class BrushFaceHandle;
class EditorContext;
class LayerNode;
class Node;

HitType::Type nodeHitType();

LayerNode* findContainingLayer(Node* node);

std::vector<LayerNode*> findContainingLayersUserSorted(const std::vector<Node*>& nodes);

GroupNode* findContainingGroup(Node* node);
const GroupNode* findContainingGroup(const Node* node);

GroupNode* findContainingLinkedGroup(Node& node);
const GroupNode* findContainingLinkedGroup(const Node& node);

/**
 * Searches the ancestor chain of `node` for the outermost closed group and returns
 * it if one is found, otherwise returns nullptr.
 */
GroupNode* findOutermostClosedGroup(Node* node);
const GroupNode* findOutermostClosedGroup(const Node* node);

std::vector<GroupNode*> findLinkedGroups(
  const std::vector<Node*>& nodes, const std::string& linkedGroupId);
std::vector<GroupNode*> findAllLinkedGroups(const std::vector<Node*>& nodes);

/**
 * Collect the linked group IDs of the given node and all of its ancestors.
 */
std::vector<std::string> collectParentLinkedGroupIds(const Node& parent);

std::vector<Node*> collectParents(const std::vector<Node*>& nodes);
std::vector<Node*> collectParents(const std::map<Node*, std::vector<Node*>>& nodes);
std::vector<Node*> collectParents(
  const std::vector<std::pair<Node*, std::vector<std::unique_ptr<Node>>>>& nodes);

std::vector<Node*> collectChildren(const std::map<Node*, std::vector<Node*>>& nodes);
std::vector<Node*> collectChildren(
  const std::vector<std::pair<Node*, std::vector<std::unique_ptr<Node>>>>& nodes);
std::vector<Node*> collectDescendants(const std::vector<Node*>& nodes);
std::map<Node*, std::vector<Node*>> parentChildrenMap(const std::vector<Node*>& nodes);

std::vector<Node*> collectNodes(const std::vector<Node*>& nodes);

template <typename T, typename Predicate>
std::vector<Node*> collectNodes(const std::vector<T*>& nodes, const Predicate& predicate)
{
  auto result = std::vector<Node*>{};

  for (auto* node : nodes)
  {
    node->accept(kdl::overload(
      [&](auto&& thisLambda, WorldNode* worldNode) {
        if (predicate(worldNode))
        {
          result.push_back(worldNode);
        }
        worldNode->visitChildren(thisLambda);
      },
      [&](auto&& thisLambda, LayerNode* layerNode) {
        if (predicate(layerNode))
        {
          result.push_back(layerNode);
        }
        layerNode->visitChildren(thisLambda);
      },
      [&](auto&& thisLambda, GroupNode* groupNode) {
        if (predicate(groupNode))
        {
          result.push_back(groupNode);
        }
        groupNode->visitChildren(thisLambda);
      },
      [&](auto&& thisLambda, EntityNode* entityNode) {
        if (predicate(entityNode))
        {
          result.push_back(entityNode);
        }
        entityNode->visitChildren(thisLambda);
      },
      [&](BrushNode* brushNode) {
        if (predicate(brushNode))
        {
          result.push_back(brushNode);
        }
      },
      [&](PatchNode* patchNode) {
        if (predicate(patchNode))
        {
          result.push_back(patchNode);
        }
      }));
  }

  return result;
}

std::vector<Node*> collectTouchingNodes(
  const std::vector<Node*>& nodes, const std::vector<BrushNode*>& brushes);
std::vector<Node*> collectContainedNodes(
  const std::vector<Node*>& nodes, const std::vector<BrushNode*>& brushes);

std::vector<Node*> collectSelectedNodes(const std::vector<Node*>& nodes);

std::vector<Node*> collectSelectableNodes(
  const std::vector<Node*>& nodes, const EditorContext& editorContext);

std::vector<BrushFaceHandle> collectBrushFaces(const std::vector<Node*>& nodes);
std::vector<BrushFaceHandle> collectSelectedBrushFaces(const std::vector<Node*>& nodes);
std::vector<BrushFaceHandle> collectSelectableBrushFaces(
  const std::vector<Node*>& nodes, const EditorContext& editorContext);

vm::bbox3 computeLogicalBounds(
  const std::vector<Node*>& nodes, const vm::bbox3& defaultBounds = vm::bbox3());
vm::bbox3 computePhysicalBounds(
  const std::vector<Node*>& nodes, const vm::bbox3& defaultBounds = vm::bbox3());

std::vector<BrushNode*> filterBrushNodes(const std::vector<Node*>& nodes);
std::vector<EntityNode*> filterEntityNodes(const std::vector<Node*>& nodes);

struct SelectionResult
{
  std::vector<Node*> nodesToSelect;
  std::vector<GroupNode*> groupsToLock;
};

/**
 * Given a list of `nodes` the user wants to select, returns the subset that we should
 * allow selection of, as well as a list of linked groups to lock.
 *
 * - Attempting to select nodes inside a linked group will propose locking all other
 * groups in that link set. This is intended to prevent users from making conflicting
 * commands as well as communicate which specific linked group they are modifying.
 *
 * - If `nodes` contains members of different groups in the same link set,
 *  only those in the first group will be allowed to be selected ("first" in the order of
 * `nodes`).
 *
 * Note: no changes are made, just the proposed selection and locking is returned.
 */
SelectionResult nodeSelectionWithLinkedGroupConstraints(
  WorldNode& world, const std::vector<Node*>& nodes);

struct FaceSelectionResult
{
  std::vector<BrushFaceHandle> facesToSelect;
  std::vector<GroupNode*> groupsToLock;
};

/**
 * Given a list of `faces` the user wants to select, returns the subset that we should
 * allow selection of, as well as a list of linked groups to lock.
 *
 * @see nodeSelectionWithLinkedGroupConstraints()
 */
FaceSelectionResult faceSelectionWithLinkedGroupConstraints(
  WorldNode& world, const std::vector<BrushFaceHandle>& faces);

} // namespace TrenchBroom::Model

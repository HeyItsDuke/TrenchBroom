/*
 Copyright (C) 2010-2012 Kristian Duske
 
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
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TrenchBroom_Grid_h
#define TrenchBroom_Grid_h

#include "Utility/VecMath.h"

using namespace TrenchBroom::Math;

namespace TrenchBroom {
    namespace Model {
        class Entity;
        class Face;
    }
    
    namespace Utility {
        class Grid {
        private:
            static const unsigned int MaxSize = 9;
            static const float SnapAngle;
            unsigned int m_size;
            bool m_snap;
            bool m_visible;
        public:
            Grid(unsigned int size) : m_size(size), m_snap(true), m_visible(true) {}
            unsigned int size() const;
            void setSize(unsigned int size);
            unsigned int actualSize() const;
            float angle() const {
                if (m_snap)
                    return 15.0f;
                return 1.0f;
            }

            void toggleVisible();
            bool visible() const;
            void toggleSnap();
            bool snap() const;
            
            float snap(float f) const;
            float snapAngle(float a) const;
            float snapUp(float f, bool skip = false) const;
            float snapDown(float f, bool skip = false) const;
            float offset(float f) const;
            Vec3f snap(const Vec3f& p) const;
            Vec3f snapUp(const Vec3f& p, bool skip = false) const;
            Vec3f snapDown(const Vec3f& p, bool skip = false) const;
            Vec3f snapTowards(const Vec3f& p, const Vec3f& d, bool skip = false) const;
            Vec3f offset(const Vec3f& p) const;
            Vec3f snap(const Vec3f& p, const Plane& onPlane) const;
            
            float intersectWithRay(const Ray& ray, unsigned int skip) const;

            Vec3f moveDeltaForEntity(const Vec3f& origin, const BBox& worldBounds, const Vec3f& delta) const;
            Vec3f moveDeltaForEntity(const Model::Face& face, const BBox& bounds, const BBox& worldBounds, const Ray& ray, const Vec3f& position) const;
            Vec3f moveDelta(const BBox& bounds, const BBox& worldBounds, const Vec3f& delta) const;
            Vec3f moveDelta(const Vec3f& point, const BBox& worldBounds, const Vec3f& delta) const;
            Vec3f moveDelta(const Vec3f& delta) const;
            Vec3f combineDeltas(const Vec3f& delta1, const Vec3f& delta2) const;
            // float moveDistance(const Model::Face& face, Vec3f& delta);
        };
    }
}

#endif

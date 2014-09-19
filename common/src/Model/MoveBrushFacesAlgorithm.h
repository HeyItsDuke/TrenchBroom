/*
 Copyright (C) 2010-2014 Kristian Duske
 
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

#ifndef __TrenchBroom__MoveBrushFacesAlgorithm__
#define __TrenchBroom__MoveBrushFacesAlgorithm__

#include "Model/MoveBrushVertexAlgorithm.h"

#include "TrenchBroom.h"
#include "VecMath.h"
#include "Model/BrushGeometry.h"

namespace TrenchBroom {
    namespace Model {
        class MoveBrushFacesAlgorithm : public MoveBrushVertexAlgorithm<MoveFacesResult> {
        private:
            const BBox3& m_worldBounds;
            const Polygon3::List& m_faces;
            const Vec3& m_delta;
        public:
            MoveBrushFacesAlgorithm(BrushGeometry& geometry, const BBox3& worldBounds, const Polygon3::List& faces, const Vec3& delta);
        private:
            bool doCanExecute(BrushGeometry& geometry);
            MoveFacesResult doExecute(BrushGeometry& geometry);
        };
    }
}

#endif /* defined(__TrenchBroom__MoveBrushFacesAlgorithm__) */

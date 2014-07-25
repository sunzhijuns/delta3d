/*
* Delta3D Open Source Game and Simulation Engine
* Copyright (C) 2008 MOVES Institute
*
* This library is free software; you can redistribute it and/or modify it under
* the terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; either version 2.1 of the License, or (at your option)
* any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
* details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
*/

#ifndef _OBJECT_VIEWER_DATA_
#define _OBJECT_VIEWER_DATA_

namespace dtCore
{
   class Transform;
   class Light;
}

struct LightInfo
{
   dtCore::Transform* transform;
   dtCore::Light* light;
};

const QString MAP_LABEL("Maps");
const QString STATIC_MESH_LABEL("Static Meshes");
const QString SKELETAL_MESH_LABEL("Skeletal Meshes");

#endif
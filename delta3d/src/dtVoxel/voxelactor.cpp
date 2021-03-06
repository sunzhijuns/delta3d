/* -*-c++-*-
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2015, Caper Holdings, LLC
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
 */

#include <dtVoxel/voxelactor.h>
#include <dtVoxel/voxelgriddebugdrawable.h>
#include <dtVoxel/aabbintersector.h>
#include <dtVoxel/voxelgeometry.h>
#include <dtVoxel/voxelmessagetype.h>

#include <dtABC/application.h>

#include <dtCore/transformable.h>
#include <dtCore/propertymacros.h>
#include <dtCore/camera.h>
#include <dtCore/project.h>
#include <dtCore/transform.h>
#include <dtCore/system.h>

#include <dtGame/basemessages.h>
#include <dtGame/gamemanager.h>
#include <dtGame/shaderactorcomponent.h>
#include <dtGame/drpublishingactcomp.h>

#include <openvdb/openvdb.h>
#include <openvdb/tools/Interpolation.h>

#include <dtPhysics/physicsactcomp.h>

#include <dtUtil/functor.h>

namespace dtVoxel
{

   VoxelActor::VoxelActor()
   : mResetCount(0)
   , mViewDistance(1000.0f)
   , mIsoLevel(0.12f)
   , mSimplify(false)
   , mSampleRatio(0.2f)
   , mMaxCellsToUpdatePerFrame(1)
   , mMinCellsToUpdatePerFrame(0)
   , mUpdateCellsOnBackgroundThread(true)
   , mNumLODs(0)
   , mPhysicsTesselationMode(&PhysicsTesselationMode::BOX_2_TRI_PER_SIDE)
   , mCreateRemotePhysics(false)
   , mPauseUpdate(false)
   , mTicksSinceVisualUpdate(0)
   {
   }

   VoxelActor::~VoxelActor()
   {
   }

   /////////////////////////////////////////////////////
   DT_IMPLEMENT_ACCESSOR_WITH_STATEMENT(VoxelActor, dtCore::ResourceDescriptor, Database, LoadGrid(value););
   DT_IMPLEMENT_ACCESSOR_WITH_STATEMENT(VoxelActor, int, ResetCount, if (value > mResetCount) {printf("ResetCount old %d, new %d\n", value, mResetCount); ResetGrid();});

   /////////////////////////////////////////////////////
   void VoxelActor::LoadGrid(const dtCore::ResourceDescriptor& rd)
   {
      if (rd != GetDatabase() && !rd.IsEmpty())
      {
         CleanupPhysics();
         mGrids.reset();
         try
         {
            if (!mLoader.valid())
            {
               mLoader = new ReadOVDBThreadPoolTask();
            }
            mLoader->ResetData();
            mLoader->SetFileToLoad(dtCore::Project::GetInstance().GetResourcePath(rd));
            mLoader->operator()();
            //dtUtil::ThreadPool::AddTask(*mLoader, dtUtil::ThreadPool::IO);
            if (IsInSTAGE())
               CompleteLoad();
         }
         catch (const dtUtil::FileNotFoundException& ex)
         {
            ex.LogException(dtUtil::Log::LOG_ERROR);
            throw ex;
         }
      }
      else if (rd.IsEmpty())
      {
         LOGN_DEBUG("voxelactor.cpp", "Unloading Grid");
         mGrids.reset();
      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::CompleteLoad()
   {
      if (!mLoader.valid())
         return;

      if (!mLoader->IsComplete())
      {
         mLoader->WaitUntilComplete();
      }

      mGrids = mLoader->GetLoadedGrids();

      mLoader = nullptr;

      InitializePhysics();

      if (mVisualGrid.valid())
      {
         mVisualGrid->ResetGrid();
      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::BuildPropertyMap()
   {
      BaseClass::BuildPropertyMap();

      typedef dtCore::PropertyRegHelper<VoxelActor> RegHelper;
      static dtUtil::RefString GROUP("VoxelActor");
      RegHelper regHelper(*this, this, GROUP);
      
      DT_REGISTER_PROPERTY_WITH_LABEL(ViewDistance, "View Distance", "The distance to at which voxels will be generated into groups of volumes and rendered.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(IsoLevel, "IsoLevel", "The value of the database which marks the surface, use the same number as the thickness if you compiled a database with the OpenVDBCompiler.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(Simplify, "Simplify", "Whether or not to simplify the result, this can take a long longer to generate.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(SampleRatio, "SampleRatio", "The percentage at which to sample in each dimension when simplifying, make sure to turn on the Simplify option to use this property.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(GridDimensions, "Grid Dimensions", "The size of the grid to allocate into blocks.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(BlockDimensions,"Block Dimensions", "The size of the blocks within the grid.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(CellDimensions, "Cell Dimensions", "The size of the cells within the blocks", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(StaticResolution, "Static Resolution", "The resolution to sample the VDB database for the static pregenerated data.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(DynamicResolution, "Dynamic Resolution", "The resolution to sample the VDB database for the dynamic deformable data.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(Offset, "Offset", "The offset of the database in world space.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY(MaxCellsToUpdatePerFrame, "The number of dirty cells to regenerate per frame", RegHelper, regHelper);
      DT_REGISTER_PROPERTY(MinCellsToUpdatePerFrame, "The minimum number to regenerate.  If this is less than max, it will decrease the number if simulation can't simulate as fast as configured in fixed time step. See system.h", RegHelper, regHelper);
      DT_REGISTER_PROPERTY(UpdateCellsOnBackgroundThread, "If this should push the update task to a thread pool BACKGROUND task.  If true, runs multithreaded but blocks the main thread until it completes.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY(NumLODs, "The number of LODs to generate, can be 0, 1, 2 or 3", RegHelper, regHelper);
      DT_REGISTER_PROPERTY_WITH_LABEL(CreateRemotePhysics, "Create Remote Physics", "Create the voxel geometry for the physics if this actor is remote.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY(ResetCount, "A database reset counter that will increment when reset is called so remotes will update.", RegHelper, regHelper);
      DT_REGISTER_PROPERTY(PhysicsTesselationMode, "The mode of tesselating the physics geometry.", RegHelper, regHelper);
      GetProperty("ResetCount")->SetSendInPartialUpdate(true);


      DT_REGISTER_RESOURCE_PROPERTY(dtCore::DataType::VOLUME, Database, "Database", "Voxel database file", RegHelper, regHelper);
   }

   /////////////////////////////////////////////////////
   openvdb::GridPtrVecPtr VoxelActor::GetGrids()
   {
      return mGrids;
   }

   /////////////////////////////////////////////////////
   openvdb::GridBase::Ptr VoxelActor::GetGrid(int i)
   {
      if (mGrids) return (*mGrids)[i];
      return NULL;
   }

   /////////////////////////////////////////////////////
   size_t VoxelActor::GetNumGrids() const
   {
      size_t result = 0;
      if (mGrids) result = mGrids->size();
      return result;
   }

   /////////////////////////////////////////////////////
   openvdb::GridBase::Ptr VoxelActor::CollideWithAABB(const osg::BoundingBox& bb, int gridIdx)
   {
      openvdb::GridBase::Ptr result(NULL);
      openvdb::BBoxd bbox(openvdb::Vec3d(bb.xMin(), bb.yMin(), bb.zMin()), openvdb::Vec3d(bb.xMax(), bb.yMax(), bb.zMax()));
      if (openvdb::BoolGrid::Ptr gridB = boost::dynamic_pointer_cast<openvdb::BoolGrid>(GetGrid(gridIdx)))
      {
         AABBIntersector<openvdb::BoolGrid> aabb(gridB);
         aabb.SetWorldBB(bbox);
         aabb.Intersect();
         result = aabb.GetHits();
      }
      else if (openvdb::FloatGrid::Ptr gridF = boost::dynamic_pointer_cast<openvdb::FloatGrid>(GetGrid(gridIdx)))
      {
         AABBIntersector<openvdb::FloatGrid> aabb(gridF);
         aabb.SetWorldBB(bbox);
         aabb.Intersect();
         result = aabb.GetHits();
      }
      return result;
   }

   ////////////////////////////////////////////////////
   bool VoxelActor::HasDataInAABB(const osg::BoundingBox& bb, int gridIdx)
   {
      bool result = false;
      openvdb::BBoxd bbox(openvdb::Vec3d(bb.xMin(), bb.yMin(), bb.zMin()), openvdb::Vec3d(bb.xMax(), bb.yMax(), bb.zMax()));
      if (openvdb::BoolGrid::Ptr gridB = boost::dynamic_pointer_cast<openvdb::BoolGrid>(GetGrid(gridIdx)))
      {
         AABBIntersector<openvdb::BoolGrid> aabb(gridB);
         aabb.SetWorldBB(bbox);
         result = aabb.HasDataInAABB();
       }
      else if (openvdb::FloatGrid::Ptr gridF = boost::dynamic_pointer_cast<openvdb::FloatGrid>(GetGrid(gridIdx)))
      {
         AABBIntersector<openvdb::FloatGrid> aabb(gridF);
         aabb.SetWorldBB(bbox);
         aabb.Intersect();
         result = aabb.HasDataInAABB();
      }
      return result;
   }

   /////////////////////////////////////////////////////
   void VoxelActor::MarkVisualDirty(const osg::BoundingBox& bb, int gridIdx)
   {
      mVisualGrid->MarkDirtyAABB(bb);
   }

   /////////////////////////////////////////////////////
   void VoxelActor::SharedTick(const dtGame::TickMessage& tickMessage, bool local)
   {
      if (mLoader.valid() && mLoader->IsComplete())
      {
         CompleteLoad();
      }

      if (mVisualGrid.valid())// && !mPauseUpdate)
      {
         dtGame::GameManager* gm = GetGameManager();

         if (gm != nullptr)
         {
            dtCore::Camera* cam = gm->GetApplication().GetCamera();

            osg::Vec3 pos;
            dtCore::Transform xform;
            cam->GetTransform(xform);
            xform.GetTranslation(pos);

            mVisualGrid->UpdateGrid(pos);

            if (local)
            {
               ModifyGrids.emit_signal(tickMessage);
            }

            std::vector<VolumeUpdateMessagePtr> workingMessages;
            workingMessages.swap(mUpdateMessages);
            //std::cout << "Number of update messages " << workingMessages.size() << std::endl;
            std::for_each(workingMessages.begin(), workingMessages.end(),
                  [this](VolumeUpdateMessagePtr& msg)
                  {
                     UpdateVolume(*msg, false);
                  });

            if (mMinCellsToUpdatePerFrame >= mMaxCellsToUpdatePerFrame)
            {
               mVisualGrid->BeginNewUpdates(pos, mMaxCellsToUpdatePerFrame, mUpdateCellsOnBackgroundThread);
            }
            else
            {
               double correctSimTime = dtCore::System::GetInstance().GetCorrectSimulationTime();
               double timeDiff = dtUtil::Max(correctSimTime - tickMessage.GetSimulationTime(), 0.0);
               if (timeDiff < tickMessage.GetDeltaSimTime())
               {
                  double fractionOfTickBehind = timeDiff/tickMessage.GetDeltaSimTime();
                  unsigned numToUpdate = unsigned(std::ceil((1.0 - fractionOfTickBehind) * double(mMaxCellsToUpdatePerFrame)));
                  dtUtil::Clamp(numToUpdate, mMinCellsToUpdatePerFrame, mMaxCellsToUpdatePerFrame);
                  mVisualGrid->BeginNewUpdates(pos, numToUpdate, mUpdateCellsOnBackgroundThread);
                  mTicksSinceVisualUpdate = 0;

                  LOGN_DEBUG("voxelactor.cpp", "Beginning New Updates on " + dtUtil::ToString(numToUpdate) + " tiles .");
               }
               else if (mMinCellsToUpdatePerFrame > 0 || mTicksSinceVisualUpdate > 2)
               {
                  LOGN_DEBUG("voxelactor.cpp", "Updating forced to 1 or minimum cells.");
                  mVisualGrid->BeginNewUpdates(pos, dtUtil::Max(mMinCellsToUpdatePerFrame, 1U), mUpdateCellsOnBackgroundThread);
                  mTicksSinceVisualUpdate = 0;
               }
               else
               {
                  ++mTicksSinceVisualUpdate;
                  LOGN_DEBUG("voxelactor.cpp", "Simulation time is behind by " + dtUtil::ToString(timeDiff) + "ms skipping voxel update to help catch up.");
               }
            }
         }
      }

   }

   /////////////////////////////////////////////////////
   void VoxelActor::OnTickLocal(const dtGame::TickMessage& tickMessage)
   {
      SharedTick(tickMessage, true);
   }

   /////////////////////////////////////////////////////
   void VoxelActor::OnTickRemote(const dtGame::TickMessage& tickMessage)
   {
      SharedTick(tickMessage, false);
   }

   /////////////////////////////////////////////////////
   void VoxelActor::CreateDrawable()
   {
      //commented out, this is a simple debug rendering
      //dtCore::RefPtr<VoxelGridDebugDrawable> dd = new VoxelGridDebugDrawable();
      //dd->CreateDebugDrawable(*mGrid);
      //SetDrawable(*dd);

      mVisualGrid = new VoxelGrid();
      SetDrawable(*mVisualGrid);

   }

   template<typename T>
   bool convertFromHalfFloat(const dtCore::NamedParameter* np, T& value)
   {
      return false;
   }

   template<>
   bool convertFromHalfFloat<float>(const dtCore::NamedParameter* np, float& value)
   {
      auto nsp = dynamic_cast<const dtCore::NamedUnsignedShortIntParameter*>(np);
      if (nsp != nullptr)
      {
         half h;
         h.setBits(nsp->GetValue());
         value = h;
         return true;
      }
      return false;
   }

   /////////////////////////////////////////////////////
   template<typename GridTypePtr>
   void VoxelActor::UpdateVolumeInternal(GridTypePtr grid, const dtCore::NamedArrayParameter* indices, const dtCore::NamedArrayParameter* values,
         bool updateVisualOnly)
   {
      typedef typename GridTypePtr::element_type GridType;
      typedef typename GridType::ValueType ValueType;
      typedef typename dtCore::TypeToActorProperty<ValueType>::named_parameter_type ParameterType;
      typedef typename GridType::Accessor AccessorType;

      AccessorType accessor = grid->getAccessor();
      bool first = true;
      float rangeToRecomputeBound = 0.5 * mCellDimensions.x() * mCellDimensions.y();

      openvdb::Vec3d lastVector;
      osg::BoundingBox bb;

      first = true;
      MarkVisualDirty(bb, 0);

      for (unsigned i = 0, iend = indices->GetSize(); i < iend; ++i)
      {
         const dtCore::NamedParameter* indexP = indices->GetParameter(i);
         if (indexP != nullptr && indexP->GetDataType() == dtCore::DataType::VEC3)
         {
            auto indexVp = static_cast<const dtCore::NamedVec3Parameter*>(indexP);
            osg::Vec3 idxVec = indexVp->GetValue();
            openvdb::Vec3d idxOVDBVec(idxVec.x(), idxVec.y(), idxVec.z());
            openvdb::Vec3d worldVec = grid->transform().indexToWorld(idxOVDBVec);
            if (first)
            {
               lastVector = worldVec;

               first = false;
            }
            else
            {
               openvdb::Vec3d diff = lastVector - worldVec;
               double lengthSqr = diff.lengthSqr();
               if (lengthSqr > rangeToRecomputeBound)
               {
                  //markDirtyCounter++;
                  MarkVisualDirty(bb, 0);
                  bb = osg::BoundingBox();
               }
            }

            bb.expandBy(osg::Vec3(worldVec.x(), worldVec.y(), worldVec.z()));
            if (!updateVisualOnly)
            {
               auto baseParam = values->GetParameter(i);
               auto valueParam = dynamic_cast<const ParameterType*>(baseParam);

               openvdb::Coord c(std::round(idxVec.x()), std::round(idxVec.y()), std::round(idxVec.z()));
               if (valueParam != nullptr)
               {
                  ValueType val = valueParam->GetValue();
                  accessor.setValueOn(c, val);
                  //std::cout << "Updating coord: "  << c << std::endl;
               }
               else if (baseParam != nullptr)
               {
                  ValueType val;
                  if (convertFromHalfFloat(baseParam, val))
                  {
                     accessor.setValueOn(c, val);
                  }
                  else
                  {
                     // It supposed to have a value, but it's not the right type.
                     accessor.setValueOn(c, grid->background());
                  }
               }
               else
               {
                  accessor.setValueOff(c, grid->background());
                  //std::cout << "turning off coord: "  << c << std::endl;
               }
            }

            lastVector = worldVec;
         }
         else
         {
            LOGN_ERROR("voxelactor.cpp", "Received a VolumeUpdateMessage, but the indices are not Vec3 parameters.");
         }
      }
      
      MarkVisualDirty(bb, 0);
   }

   /////////////////////////////////////////////////////
   void VoxelActor::UpdateVolume(const VolumeUpdateMessage& msg, bool updateVisualOnly)
   {
      if (GetNumGrids() > 0)
      {
         const dtCore::NamedArrayParameter* indices = msg.GetIndicesChanged();
         const dtCore::NamedArrayParameter* values = msg.GetValuesChanged();

         openvdb::FloatGrid::Ptr gridF = boost::dynamic_pointer_cast<openvdb::FloatGrid>(GetGrid(0));
         if (gridF)
         {
            UpdateVolumeInternal(gridF, indices, values, updateVisualOnly);
         }
         else
         {
            openvdb::BoolGrid::Ptr gridB = boost::dynamic_pointer_cast<openvdb::BoolGrid>(GetGrid(0));
            if (gridB)
               UpdateVolumeInternal(gridB, indices, values, updateVisualOnly);
         }
      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::OnVolumeUpdateMsg(const VolumeUpdateMessage& msg)
   {
      if ((IsRemote() || GetLocalActorUpdatePolicy() != dtGame::GameActorProxy::LocalActorUpdatePolicy::IGNORE_ALL)
                  && msg.GetSource() != GetGameManager()->GetMachineInfo())
      {
         mUpdateMessages.push_back(&msg);
      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::InitializePhysics()
   {
      if (IsInGM() && (!IsRemote() || GetCreateRemotePhysics()))
      {
         dtPhysics::PhysicsActCompPtr pac = GetComponent<dtPhysics::PhysicsActComp>();
         if (mGrids && pac.valid())
         {
            for (unsigned i = 0; i < mGrids->size(); ++i)
            {
               dtPhysics::PhysicsObject* po = pac->GetPhysicsObjectByIndex(0);
               if (po != nullptr && po->GetPrimitiveType() == dtPhysics::PrimitiveType::CUSTOM_CONCAVE_MESH)
               {
                  po->CleanUp();

                  dtPhysics::TransformType xform;
                  VoxelGeometryPtr geometry;
                  openvdb::FloatGrid::Ptr gridF = boost::dynamic_pointer_cast<openvdb::FloatGrid>(GetGrid(0));
                  if (gridF)
                  {
                     geometry = VoxelGeometry::CreateVoxelGeometry(xform, po->GetMass(), gridF, mPhysicsTesselationMode);
                  }
                  else
                  {
                     openvdb::BoolGrid::Ptr gridB = boost::dynamic_pointer_cast<openvdb::BoolGrid>(GetGrid(0));
                     if (gridB)
                        geometry = VoxelGeometry::CreateVoxelGeometry(xform, po->GetMass(), gridB, mPhysicsTesselationMode);
                  }
                  if (geometry.valid())
                     po->CreateFromGeometry(*geometry);
               }
            }
         }
      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::CleanupPhysics()
   {
      dtPhysics::PhysicsActCompPtr pac = GetComponent<dtPhysics::PhysicsActComp>();
      if (pac.valid())
      {
         dtPhysics::PhysicsActCompPtr pac = GetComponent<dtPhysics::PhysicsActComp>();
         if (mGrids && pac.valid())
         {
            for (unsigned i = 0; i < mGrids->size(); ++i)
            {
               dtPhysics::PhysicsObject* po = pac->GetPhysicsObjectByIndex(0);
               if (po != nullptr && po->GetPrimitiveType() == dtPhysics::PrimitiveType::CUSTOM_CONCAVE_MESH)
               {
                  po->CleanUp();
               }
            }
         }
      }
   }

   /////////////////////////////////////////////////////
   void VoxelActor::OnEnteredWorld()
   {
      if (IsRemote())
      {
         RegisterForMessages(dtGame::MessageType::TICK_REMOTE, dtGame::GameActorProxy::TICK_REMOTE_INVOKABLE);
      }
      else
      {
         RegisterForMessages(dtGame::MessageType::TICK_LOCAL, dtGame::GameActorProxy::TICK_LOCAL_INVOKABLE);
      }

      osg::Vec3i staticRes(int(mStaticResolution.x()), int(mStaticResolution.y()), int(mStaticResolution.z()));
      osg::Vec3i dynamicRes(int(mDynamicResolution.x()), int(mDynamicResolution.y()), int(mDynamicResolution.z()));

      mVisualGrid->Init(mOffset, mGridDimensions, mBlockDimensions, mCellDimensions, staticRes, dynamicRes);
      
      dtGame::GameManager* gm = GetGameManager();

      if (gm != nullptr)
      {
         dtCore::Camera* cam = gm->GetApplication().GetCamera();

         osg::Vec3 pos;
         dtCore::Transform xform;
         cam->GetTransform(xform);
         xform.GetTranslation(pos);

         mVisualGrid->SetViewDistance(mViewDistance);
         mVisualGrid->CreatePagedLODGrid(pos, *this);
      }
      else
      {
         LOG_ERROR("NULL GameManager!");
      }

      if (mLoader.valid() && mLoader->IsComplete())
      {
         CompleteLoad();
      }
      else
      {
         InitializePhysics();
      }

      RegisterForMessagesAboutSelf(VoxelMessageType::INFO_VOLUME_CHANGED, dtUtil::MakeFunctor(&VoxelActor::OnVolumeUpdateMsg, this));

      dtGame::ShaderActorComponent* shaderComp = nullptr;
      GetComponent(shaderComp);
      if (shaderComp != nullptr)
      {
         mVisualGrid->SetShaderGroup(shaderComp->GetCurrentShader().GetResourceIdentifier());
      }
   }


   void VoxelActor::ResetGrid()
   {
      dtCore::ResourceDescriptor temp = mDatabase;
      SetDatabase(dtCore::ResourceDescriptor::NULL_RESOURCE);
      SetDatabase(temp);

      if (!IsRemote())
      {
         ++mResetCount;
         if (!HasComponent(dtGame::DRPublishingActComp::TYPE))
         {
            NotifyPartialActorUpdate();
         }
         else
         {
            GetComponent<dtGame::DRPublishingActComp>()->ForceUpdateAtNextOpportunity();
         }
      }
   }

} /* namespace dtVoxel */

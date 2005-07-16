/* 
 * Delta3D Open Source Game and Simulation Engine 
 * Copyright (C) 2004 MOVES Institute 
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

#ifndef DELTA_SKYDOME
#define DELTA_SKYDOME

#include "dtCore/enveffect.h"
#include "dtUtil/deprecationmgr.h"
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Transform>
#include <osgUtil/CullVisitor>
#include "sg.h"

namespace dtCore
{
   ///A rendered dome that represents the sky
   class DT_EXPORT SkyDome :  public EnvEffect
   {
      DECLARE_MANAGEMENT_LAYER(SkyDome)
   public:
      SkyDome(std::string name="SkyDome");
      virtual ~SkyDome(void);
      //virtual osg::Group *GetNode(void) {return mNode;}

      void SetBaseColor(const osg::Vec3& color);
      //deprecated version
      void SetBaseColor(sgVec3 color)
      {
         DEPRECATE("void SetBaseColor(sgVec3 color)", "void SetBaseColor(const osg::Vec3& color)")
         SetBaseColor(osg::Vec3(color[0], color[1], color[2]));
      }
      
      void GetBaseColor(osg::Vec3& color) const{color.set(mBaseColor);}
      //deprecated version
      void GetBaseColor(sgVec3 color) 
      {
         DEPRECATE("void GetBaseColor(sgVec3 color)", "void GetBaseColor(osg::Vec3& color)const")
         osg::Vec3 tmp;
         GetBaseColor(tmp);
         color[0] = tmp[0]; color[1] = tmp[1]; color[2] = tmp[2];
      }


      virtual void Repaint(const osg::Vec3& sky_color, const osg::Vec3& fog_color, 
         double sun_angle, double sunAzimuth,
         double vis);

      //deprecated version
      virtual void Repaint(sgVec3 sky_color, sgVec3 fog_color, 
         double sun_angle, double sunAzimuth,
         double vis)
      {
         DEPRECATE("virtual void Repaint(sgVec3 sky_color, sgVec3 fog_color, double sun_angle, double sunAzimuth,double vis)", 
                   "virtual void Repaint(const osg::Vec3& sky_color, const osg::Vec3& fog_color, double sun_angle, double sunAzimuth, double vis)")

         Repaint(osg::Vec3(sky_color[0], sky_color[1], sky_color[2]),
                 osg::Vec3(fog_color[0], fog_color[1], fog_color[2]),
                 sun_angle, sunAzimuth, vis);      
      }


   private:
      // Build the sky dome
      void Config(void);

      //osg::Group *mNode;
      osg::Node* MakeDome(void);
      osg::Vec3 mBaseColor;
      osg::Geode *mGeode;

      class DT_EXPORT MoveEarthySkyWithEyePointTransform : public osg::Transform
      {
      public:
         float mAzimuth; ///<Rotates the Transform to line up with this amount(deg)

         /** Get the transformation matrix which moves from local coords to world coords.*/
         virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const 
         {
            osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
            if (cv)
            {
               osg::Vec3 eyePointLocal = cv->getEyeLocal();
               matrix.preMult(osg::Matrix::translate(eyePointLocal.x(),eyePointLocal.y(),eyePointLocal.z()));
               matrix.preMult(osg::Matrix::rotate(osg::DegreesToRadians(mAzimuth-90.0), 0.f, 0.f, 1.f));
            }
            return true;
         }

         /** Get the transformation matrix which moves from world coords to local coords.*/
         virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
         {    
            osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
            if (cv)
            {
               osg::Vec3 eyePointLocal = cv->getEyeLocal();
               matrix.postMult(osg::Matrix::translate(-eyePointLocal.x(),-eyePointLocal.y(),-eyePointLocal.z()));
            }
            return true;
         }
      };

      MoveEarthySkyWithEyePointTransform *mXform;
   };
}

#endif // DELTA_SKYDOME

/****************************************************************
 *
 * Copyright (c) 2010
 *
 * Fraunhofer Institute for Manufacturing Engineering
 * and Automation (IPA)
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Project name: care-o-bot
 * ROS stack name: cob_vision
 * ROS package name: cob_env_model
 * Description: Feature Map for storing and handling geometric features
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Author: Georg Arbeiter, email:georg.arbeiter@ipa.fhg.de
 * Supervised by: Georg Arbeiter, email:georg.arbeiter@ipa.fhg.de
 *
 * Date of creation: 08/2011
 * ToDo:
 *
 *
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Fraunhofer Institute for Manufacturing
 *       Engineering and Automation (IPA) nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License LGPL as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License LGPL for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License LGPL along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************/


//##################
//#### includes ####

// standard includes
//--
#include <sstream>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>


// external includes
#include <boost/timer.hpp>
#include <Eigen/Geometry>
#include <pcl/win32_macros.h>
#include <pcl/common/transform.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/common/centroid.h>
//#include <pcl/common/impl/transform.hpp>



#include "cob_3d_mapping_geometry_map/geometry_map.h"
//#include "cob_3d_mapping_geometry_map/vis/geometry_map_visualisation.h"



using namespace cob_3d_mapping;


//GeometryMap::addMapEntry(PolygonPtr p_ptr)



template <class T>
void
GeometryMap<T>::addMapEntry(boost::shared_ptr<T> p_ptr)

{

  T& p = *p_ptr;

cob_3d_mapping::merge_config  limits;
limits.d_thresh=d_;
limits.angle_thresh=cos_angle_;
//limits.weighting_method="AREA";
limits.weighting_method="COUNTER";
//limits.weighting_method="COMBINED";




//Debug Output------------
//std::cout<<"INPUT: "<<std::endl;
//std::cout<<"Polygon: "<<std::endl;
//std::cout<<"d= "<<p.d<<std::endl;
//std::cout<<"id= "<<p.id<<std::endl;
//std::cout<<"Map: "<<std::endl;
//std::cout<<"Size of map_ before merge "<<map_.size()<<std::endl;
//std::cout<<"old id "<<new_id_<<std::endl;
// -------------------


// find out polygons, to merge with
 std::vector<int> intersections;
 p.isMergeCandidate(map_,limits,intersections);
// std::cout<<"intersections size = "<<intersections.size()<<std::endl;


// if polygon has to be merged ...
if(intersections.size()>0)
	{
	  std::vector<boost::shared_ptr<T> > merge_candidates;

	  for(int i=0;i<(int)intersections.size();i++)
	  {

		  merge_candidates.push_back(map_[intersections[i]]);
	  }
// merge polygon with merge candidates
	  p.merge(merge_candidates);

//	  std::cout<<"size +- "<< 1 -merge_candidates.size()<<std::endl;
	}
//if polygon does not have to be merged , add new polygon
else
	{


	p.assignMembers();
	map_.push_back(p_ptr);
	new_id_++;

//	std::cout<<"size +1"<<std::endl;
	}


   if(save_to_file_) saveMap(file_path_);



   //debug
//   std::cout<<"size_map: "<<map_.size()<<std::endl;

}






template <class T>
void
GeometryMap<T>::printMapEntry(T& p)
{
  std::cout << "Polygon:\n";
  for(int i=0; i< (int)p.contours.size(); i++)
  {
    std::cout << i << std::endl;
    for(int j=0; j< (int)p.contours[i].size(); j++)
    {
      std::cout << "(" << p.contours[i][j](0) << ", " << p.contours[i][j](1) << ", " << p.contours[i][j](2) << ")\n";
    }
  }
  std::cout << "Normal: (" << p.normal(0) << ", " << p.normal(1) << ", " << p.normal(2) << ")\n";
  std::cout << "d: " << p.d << std::endl;
  std::cout << "Transformation:\n" << p.transform_from_world_to_plane.matrix() << "\n";
}

template <class T>
void
GeometryMap<T>::printMap()
{
	std::stringstream ss;

	ss << "/home/goa-tz/GM_test/map/outputfile_" << counter_output << ".txt";
    std::ofstream outputFile2;
    outputFile2.open(ss.str().c_str());


	  for(int i=0; i< (int)map_.size(); i++)
	  {

		  Polygon& p =*map_[i];

		     outputFile2 <<"ID: " << i << "trafo " << std::endl <<  p.transform_from_world_to_plane.matrix() <<std::endl;
			 outputFile2 << "normal:" << std::endl << p.normal << std::endl << "d: " << p.d << std::endl;
			 outputFile2 << "Polygon:\n";
			  for(int i=0; i< p.contours.size(); i++)
			  {
				  outputFile2 << i << std::endl;
			    for(int j=0; j< p.contours[i].size(); j++)
			    {
			    	outputFile2 << "(" << p.contours[i][j](0) << ", " << p.contours[i][j](1) << ", " << p.contours[i][j](2) << ")\n";
			    }
			  }
			 outputFile2 << "----------------------------";

	  }
	  outputFile2.close();
	  counter_output++;
}

template <class T>
void
GeometryMap<T>::saveMapEntry(std::string path, int ctr, T& p)
{
  std::stringstream ss;
  ss << path << "polygon_" << ctr << ".pl";
  std::ofstream plane_file;
  plane_file.open (ss.str().c_str());
  plane_file << p.normal(0) << " " << p.normal(1) << " " << p.normal(2) << " " << p.d;
  ss.str("");
  ss.clear();
  plane_file.close();
  for(int i=0; i< p.contours.size(); i++)
  {
    pcl::PointCloud<pcl::PointXYZ> pc;
    ss << path << "polygon_" << ctr << "_" << i << ".pcd";
    for(int j=0; j< p.contours[i].size(); j++)
    {
      pcl::PointXYZ pt;
      pt.x = p.contours[i][j](0);
      pt.y = p.contours[i][j](1);
      pt.z = p.contours[i][j](2);
      pc.points.push_back(pt);
    }
    //std::cout << ss.str() << std::endl;
    pcl::io::savePCDFileASCII (ss.str(), pc);
    ss.str("");
    ss.clear();
  }
}

template <class T>
void
GeometryMap<T>::saveMap(std::string path)
{
  static int ctr=0;
  std::stringstream ss;
  ss << path << "/" << ctr << "_";
  //std::cout << ctr << " Saving map with " << map_.size() << " entries..." << std::endl;
  for(size_t i=0; i< map_.size(); i++)
  {
      saveMapEntry(ss.str(), i, *map_[i]);
  }
  ctr++;
}


template <class T>
void
GeometryMap<T>::clearMap()
{
  map_.clear();
}

//void
//GeometryMap::printGpcStructure(gpc_polygon* p)
//{
//  std::cout << "GPC Structure: " << std::endl;
//  std::cout << "Num Contours: " << p->num_contours << std::endl;
//  for(int i=0; i< p->num_contours; i++)
//  {
//    std::cout << i << std::endl;
//    std::cout << "isHole: " << p->hole[i] << std::endl;
//    std::cout << "Num points: " << p->contour[i].num_vertices << std::endl;
//    for(int j=0; j< p->contour[i].num_vertices; j++)
//    {
//      std::cout << p->contour[i].vertex[j].x << " " << p->contour[i].vertex[j].y << "\n";
//    }
//  }
//}

//void
//GeometryMap::getCoordinateSystemOnPlane(const Eigen::Vector3f &normal,
//                                            Eigen::Vector3f &u, Eigen::Vector3f &v)
//{
//  v = normal.unitOrthogonal ();
//  u = normal.cross (v);
//}
//
//void
//GeometryMap::getTransformationFromPlaneToWorld(const Eigen::Vector3f &normal,
//                                                   const Eigen::Vector3f &origin, Eigen::Affine3f &transformation)
//{
//  Eigen::Vector3f u, v;
//
//
//  getCoordinateSystemOnPlane(normal, u, v);
//
////  std::cout << "u " << u <<  std::endl << " v " << v << std::endl;
//  pcl::getTransformationFromTwoUnitVectorsAndOrigin(v, normal,  origin, transformation);
//  transformation = transformation.inverse();
//}
//
//void
//GeometryMap::getPointOnPlane(const Eigen::Vector3f &normal,double d,Eigen::Vector3f &point)
//{
//	// outputFile << "in getPointOnPlane" << std::endl;
//	// outputFile << "normal 0 " << normal(0) << "normal 1 " << normal(1)<< "normal 2 " << normal(2)<< std::endl;
//
//	float value=fabs(normal(0));
//	int direction=0;
////	 outputFile << "abs normal 0 " << fabs(normal(0)) << "normal 1 " << fabs(normal(1))<< "normal 2 " << fabs(normal(2))<< std::endl;
//
//	if(fabs(normal(1))>value)
//	{
//
//		direction=1;
//		value=fabs(normal(1));
//	}
//
//
//	if(fabs(normal(2))>value)
//	{
//		direction=2;
//		value=fabs(normal(2));
//	}
//	point << 0,0,0;
//	point(direction)=-d/normal(direction);
////	 outputFile << "direction: " << direction << "  point " << std::endl << point << std::endl;
//
////	Eigen::Vector3f round_normal;
////	round_normal[0]=rounding(normal[0]);
////	round_normal[1]=rounding(normal[1]);
////	round_normal[2]=rounding(normal[2]);
////
////	int counter=0;
////	std::vector<int> no_zero_direction;
////	for(int i=0 ;i<3;i++)
////	{
////		if(round_normal[i]==0){
////			counter++;}
////		else
////		no_zero_direction.push_back(i);
////	}
//////	std::cout << " normal " <<std::endl<< normal << std::endl;
//////	std::cout << "counter " << counter;
////	if(counter==0)
////	{
////		point << 0,0,d/round_normal(2);
////	}
////	if(counter==1)
////	{
////		point << 0,0,0;
////		point[no_zero_direction[0]]=1;
////		point[no_zero_direction[1]]=-round_normal(no_zero_direction[0])/round_normal(no_zero_direction[1])+d/round_normal(no_zero_direction[1]);
////	}
////	if(counter==2)
////	{
////		point << 0,0,0;
////		point(no_zero_direction[0])=d/round_normal(no_zero_direction[0]);
////	}
//}


template <class T>
float
GeometryMap<T>::rounding(float x)

{
x *= 10000;
x += 0.5;
x = floor(x); 5;
x /= 10000;
return x;
}


template <class T>
void
GeometryMap<T>::colorizeMap()
{
  for(unsigned int i=0; i<map_.size(); i++)
  {
    if(fabs(map_[i]->normal[2]) < 0.1) //plane is vertical
    {
      map_[i]->color[0] = 0.5;
      map_[i]->color[1] = 0.5;
      map_[i]->color[2] = 0;
      map_[i]->color[3] = 1;
    }
    else if(fabs(map_[i]->normal[0]) < 0.12 && fabs(map_[i]->normal[1]) < 0.12 && fabs(map_[i]->normal[2]) > 0.9) //plane is horizontal
    {
      map_[i]->color[0] = 0;
      map_[i]->color[1] = 0.5;
      map_[i]->color[2] = 0;
      map_[i]->color[3] = 1;
    }
    else
    {
      map_[i]->color[0] = 1;
      map_[i]->color[1] = 1;
      map_[i]->color[2] = 1;
      map_[i]->color[3] = 1;
    }
  }
}

/*int main (int argc, char** argv)
 {

  GeometryMap fm;
  for(int j=0; j<46; j++)
  {
    GeometryMap::PolygonPtr m_p = GeometryMap::PolygonPtr(new GeometryMap::Polygon());
    std::stringstream ss;
    ss << "/home/goa/pcl_daten/kitchen_kinect/polygons/polygon_" << j << ".txt";
    std::ifstream myfile;
    myfile.open (ss.str().c_str());
    if (myfile.is_open())
    {
        myfile >> m_p->id;
        std::cout << m_p->id << std::endl;
        myfile >> m_p->normal(0);
        myfile >> m_p->normal(1);
        myfile >> m_p->normal(2);
        std::cout << "normal:" << m_p->normal << std::endl;
        std::vector<Eigen::Vector3f> vv;
        int num;
        myfile >> num;
        std::cout << num << std::endl;
        int i=0;
        while ( i<num )
        {
          Eigen::Vector3f v;
          myfile >> v(0);
          myfile >> v(1);
          myfile >> v(2);
          vv.push_back(v);
          i++;
      }
        m_p->contours.push_back(vv);
      myfile.close();
    }
    //fm.printMapEntry(*m_p);
    fm.addMapEntry(m_p);

  }
  fm.saveMap("/home/goa/pcl_daten/kitchen_kinect/map");
}*/

int main (int argc, char** argv)
 {
  GeometryMap<Polygon> gm;
  GeometryMapVisualisation gmv;
  PolygonPtr m_p = PolygonPtr(new Polygon());



  m_p->id = 0;
  m_p->normal << 0,0,1;
  m_p->d = -1;
  std::vector<Eigen::Vector3f> vv;
  Eigen::Vector3f v;
  v << 1,0,0;
  vv.push_back(v);
  v << 1,0.5,0;
  vv.push_back(v);
  v << 1,1,2;
  vv.push_back(v);
  v << 0,1,0;
  vv.push_back(v);
  m_p->contours.push_back(vv);
  m_p->holes.push_back(0);
  gm.addMapEntry(m_p);


  m_p->id = 1;
  m_p->normal << 0,0,1;
  m_p->d = -1;
  vv.clear();
  v << 0.5,3,0;
  vv.push_back(v);
  v << 1.5,3,0;
  vv.push_back(v);
  v << 1.5,4.5,0;
  vv.push_back(v);
  v << 0,1.5,0;
  vv.push_back(v);
  m_p->contours.push_back(vv);
  m_p->holes.push_back(0);
  gm.addMapEntry(m_p);

//  gmv.showPolygon(m_p,0);

  m_p = PolygonPtr(new Polygon());
  m_p->id = 2;
  m_p->normal << 0,0,1;
  m_p->d = 1;
  vv.clear();
  v << 1,0,-1;
  vv.push_back(v);
  v << 1,1,-1;
  vv.push_back(v);
  v << 0,1,-1;
  vv.push_back(v);
  v << 0,0,-1;
  vv.push_back(v);
  m_p->contours.push_back(vv);
  m_p->holes.push_back(0);
  gm.addMapEntry(m_p);
//
//  m_p = PolygonPtr(new Polygon());
//  m_p->id = 1;
//  m_p->normal << 0,0,-1;
//  m_p->d = -1;
//  vv.clear();
//  v << 2,2,1;
//  vv.push_back(v);
//  v << 2,3,1;
//  vv.push_back(v);
//  v << 3,3,1;
//  vv.push_back(v);
//  v << 3,2,1;
//  vv.push_back(v);
//  m_p->contours.push_back(vv);
//  m_p->holes.push_back(0);
//  gm.addMapEntry(m_p);
//  gm.saveMap("/home/goa-tz/GM_test/pcl_daten/merge_test");

/*  Eigen::Vector3f test;
  Eigen::Vector3f result;
  double d=1;
  test << 1,1,1;
  gm.getPointOnPlane(test,d,result);
  std::cout << result(0) <<"," << result(1) <<","<< result(2);*/
  std::cout<<"done"<<std::endl;
  return 1;
 }



template class GeometryMap<cob_3d_mapping::Polygon>;
template class GeometryMap<cob_3d_mapping::Cylinder>;


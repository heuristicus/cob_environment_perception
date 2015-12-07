#pragma once

#include "image.h"

//boost polygon, geometry
#include <boost/polygon/polygon.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
namespace bg = boost::geometry;	//nedded due error in geometry/extensions
#include <boost/geometry/extensions/algorithms/dissolve.hpp>

#include <libpolypartition/polypartition.h>

//ros
#include <eigen_conversions/eigen_msg.h>

//ros message types
#include <cob_3d_mapping_msgs/Plane.h>


namespace cob_3d_geometry_map {
	
class VolumeCut {
public:
	virtual bool cut(const nuklei::kernel::r3 &pt) const = 0;
	virtual nuklei::kernel::r3 cutting_point(const nuklei::kernel::r3 &pt) const = 0;
};
	
class Intersection_Volume {
public:
	virtual ObjectVolume::TBB bb() const = 0;
	
	virtual bool intersects(const Object *obj) const = 0;
	virtual bool contains(const Object *obj) const = 0;
};
	
class Plane_Point {
public:

	struct CustomSharedPtr : public boost::shared_ptr<Plane_Point>
	{
		CustomSharedPtr() : boost::shared_ptr<Plane_Point>(new Plane_Point) {}
		CustomSharedPtr(Plane_Point *ptr) : boost::shared_ptr<Plane_Point>(ptr) {}
	};
	
    typedef float coordinate_type;
    typedef Eigen::Matrix<coordinate_type, 2,1> Vector2;	//TODO: use definition from object.h
	typedef CustomSharedPtr Ptr;
    
    Vector2 pos, tex;
    float var;
    
    Plane_Point() : var(0) {}
    Plane_Point(const double x, const double y) : pos(x,y), var(0) {}
    Plane_Point(const Vector2 &p) : pos(p), var(0.01) {}	//TODO: get correct value from init.
    Plane_Point(const cob_3d_mapping_msgs::Point2D &);
    
private:
	
};

struct Plane_Ring : std::deque<Plane_Point::Ptr>
{
	Plane_Ring() {}
	
	Plane_Ring(const cob_3d_mapping_msgs::Polygon &poly) {
		from(poly);
	}
	
	Plane_Ring &from(const cob_3d_mapping_msgs::Polygon &poly) {
		resize(poly.points.size());
		for(size_t i=0; i<poly.points.size(); i++)
			(*this)[i].reset(new Plane_Point(poly.points[i]));
		if(size()>1)
			push_back(new Plane_Point(poly.points[0]));
		return *this;
	}
	
	void clone(const Plane_Ring &o, const Projector &proj, const nuklei::kernel::se3 &pose) {
		resize(o.size());
		for(size_t i=0; i<size(); i++) {
			(*this)[i].reset(new Plane_Point(*o[i]));
			nuklei_wmf::Vector3<double> loc(o[i]->pos(0), o[i]->pos(1), 0);
			(*this)[i]->pos = proj(nuklei::la::transform(pose.loc_, pose.ori_, loc));
		}
	}
	
	float max_var() const {
		float m=0;
		for(size_t i=0; i<size(); i++) m=std::max(m, (*this)[i]->var);
		return m;
	}
	
	void simplify_by_area(const double min_area2);
};

/*
 * a polygon consists of:
 *  - points
 *  - texture ids
 */
class Plane_Polygon {
public:
	typedef std::vector<Plane_Ring> THoles;
	typedef boost::shared_ptr<Plane_Polygon> Ptr;
	
private:
	Plane_Ring boundary_;
	THoles holes_;
	Eigen::Vector3f color_;
	std::vector<Image::Ptr> imgs_;
	
	static void internal_add_triangulation(const Plane_Ring &inp, std::list<TPPLPoly> &polys, const bool hole);
	bool cut(const VolumeCut &cutter, const nuklei::kernel::se3 &pose, Plane_Ring &inp);
public:
	Plane_Polygon() {}
	Plane_Polygon(const std::vector<cob_3d_mapping_msgs::Polygon> &, const Eigen::Vector3f &color);
	Plane_Polygon(const Plane_Polygon &o, const Projector &proj, const nuklei::kernel::se3 &pose) {
		color_ = o.color_;
		imgs_ = o.imgs_;
		clone(o, proj, pose);
	}

	const Plane_Ring &boundary() const {return boundary_;}
	Plane_Ring &boundary() {return boundary_;}
	
	const THoles &holes() const {return holes_;}
	THoles &holes() {return holes_;}
	
	float max_var() const {
		float m=boundary_.max_var();
		for(size_t i=0; i<holes_.size(); i++) m=std::max(m, holes_[i].max_var());
		return m;
	}
	
	void visualization(const std::string &name, std::vector<boost::shared_ptr<Visualization::Object> > &, const Eigen::Affine3f &);
	
	
	void buildBB(ObjectVolume::TBB &bb);
	bool cut(const VolumeCut &cutter, const nuklei::kernel::se3 &pose);
	
	inline static nuklei_wmf::Vector3<double> to3D(const Vector2 &pt, const nuklei::kernel::se3 &pose) {
		nuklei_wmf::Vector3<double> r;
		r.X() = pt(0);
		r.Y() = pt(1);
		r.Z() = 0;
		return nuklei::la::transform(pose.loc_, pose.ori_, r);
	}
	
	inline static nuklei::kernel::r3 to3Dvar(const Plane_Point &pt, const nuklei::kernel::se3 &pose) {
		nuklei::kernel::r3 r;
		r.loc_.X() = pt.pos(0);
		r.loc_.Y() = pt.pos(1);
		r.loc_.Z() = 0;
		r = r.transformedWith(pose);
		r.loc_h_ = pt.var;
		return r;
	}
	inline static nuklei::kernel::r3 to3Dvar(const Plane_Point::Ptr &pt, const nuklei::kernel::se3 &pose) {
		return to3Dvar(*pt, pose);
	}
	
	inline static Plane_Point::Vector2 to2D(const nuklei::kernel::r3 &pt, const nuklei::kernel::se3 &pose) {
		const nuklei::kernel::se3 pose_inv = pose.inverseTransformation();
		nuklei_wmf::Vector3<double> t = nuklei::la::transform(pose_inv.loc_, pose_inv.ori_, pt.loc_);
		return Plane_Point::Vector2(t.X(), t.Y());
	}
	
	inline static Plane_Point::Vector2 to2D(const nuklei_wmf::Vector3<double> &pt, const nuklei::kernel::se3 &pose) {
		const nuklei::kernel::se3 pose_inv = pose.inverseTransformation();
		nuklei_wmf::Vector3<double> t = nuklei::la::transform(pose_inv.loc_, pose_inv.ori_, pt);
		return Plane_Point::Vector2(t.X(), t.Y());
	}
	
	bool simplify_by_area(const double min_area);
	double polygon_distance(const Plane_Point &pt) const;
	
	void clone(const Plane_Polygon &o, const Projector &proj, const nuklei::kernel::se3 &pose);
	
	//helpers
	void save_as_svg(const std::string &fn, const bool check=true) const;
	void save_as_svg(boost::geometry::svg_mapper<Plane_Point::Ptr> &mapper, const int color_b=0, const int color_g=0) const;
	
	std::vector<Plane_Polygon> operator-(const Plane_Polygon &o) const ;
	std::vector<Plane_Polygon> operator+(const Plane_Polygon &o) const ;
	std::vector<Plane_Polygon> operator&(const Plane_Polygon &o) const ;
};

/*
 * a plane consists of:
 *  - polygons
 *  - a pose to captured view point
 *  - plane parameters (normal + offset)
 */
class Plane : public ObjectVolume {
	typedef double Real;
		
	std::vector<Plane_Polygon::Ptr> polygons_;
	Eigen::Vector3f offset_, normal_;
	
	void buildBB();
	
public:
	typedef ObjectVolume Parent;
	typedef boost::shared_ptr<Plane> Ptr;

	Plane(const ContextPtr &ctxt, const cob_3d_mapping_msgs::Plane &);
	Plane(const ContextPtr &ctxt, const Plane_Polygon::Ptr &inp, const nuklei::kernel::se3 &pose);
	
	virtual void visualization(std::vector<boost::shared_ptr<Visualization::Object> > &);
	
	inline Eigen::Vector3f offset_eigen() const {return cast(pose_.loc_);}
	inline Eigen::Vector3f normal_eigen() const {return cast(pose_.ori_)*Eigen::Vector3f::UnitZ();}
	
	inline nuklei_wmf::Plane3<Real> plane() const {
		return nuklei_wmf::Plane3<Real>(pose_.ori_.Rotate(nuklei_wmf::Vector3<double>::UNIT_Z), pose_.loc_);
	}
	
	static inline nuklei::kernel::r3xs2 plane_params(const nuklei::kernel::se3 &pose_) {
		nuklei::kernel::r3xs2 params;
		params.loc_ = pose_.loc_;
		params.dir_ = pose_.ori_.Rotate(nuklei_wmf::Vector3<double>::UNIT_Z);
		params.loc_h_ = pose_.loc_h_;
		params.dir_h_ = pose_.ori_h_;
		return params;
	}
	
	inline nuklei::kernel::r3xs2 plane_params() const {
		return plane_params(pose_);
	}
	
	bool snap(const Plane &other);
	bool cut(const VolumeCut &other);
	
	const std::vector<Plane_Polygon::Ptr> &polygons() const {return polygons_;}
	
	double polygon_distance(const Plane_Point &pt) const;
	
	void project(const Projector &proj, std::vector<Plane_Polygon::Ptr> &res) const;
	
	virtual bool can_merge_fast(const Object &o) const;
	virtual bool can_merge(const Object &o) const;
	virtual bool merge(const Object &o, const double relation);
	
	bool simplify_by_area(const double min_area);
	
	void insert(const Plane* origin, const std::vector<Plane_Polygon> &polys) {
		//polygons_.clear();
		for(size_t i=0; i<polys.size(); i++) {
			if(!boost::geometry::is_valid(polys[i])) continue;
			
			//check overlap
			bool ignore=false;
			for(size_t j=0; j<i; j++) {
				if(boost::geometry::overlaps(polys[i], polys[j])) {
					ignore=true;
					insert(origin, polys[i]-polys[j]);
					break;
				}
			}
			if(ignore) continue;
			
			polygons_.push_back(Plane_Polygon::Ptr(new Plane_Polygon(polys[i])));
			boost::geometry::correct(*polygons_.back());
			//boost::geometry::simplify(*polygons_[polygons_.size()-polys.size()+i], *polygons_[polygons_.size()-polys.size()+i], 0.05);
		}
	}
	
	static void complex_projection(Plane* plane_out, const Plane* plane_in1, const Plane* plane_in2, const size_t ind1, const size_t ind2);
	static void simple_projection(Plane* plane_out, const Plane* plane_in1, const Plane* plane_in2, const size_t ind1, const size_t ind2);
	
	void save_as_svg(const std::string &fn) const;
	void save_as_svg(boost::geometry::svg_mapper<Plane_Point::Ptr> &mapper, const int color_g=0) const;
	
	
	inline nuklei_wmf::Vector3<double> to3D(const Vector2 &pt) const {
		nuklei_wmf::Vector3<double> r;
		r.X() = pt(0);
		r.Y() = pt(1);
		r.Z() = 0;
		return nuklei::la::transform(pose_.loc_, pose_.ori_, r);
	}
};

class Projector_Plane : public Projector {
	Plane* plane_;
	
public:
	Projector_Plane(Plane* plane) : plane_(plane) {
	}
	
	virtual Plane_Point::Vector2 operator()(const nuklei_wmf::Vector3<double> &pt3) const {
		return Plane_Polygon::to2D(pt3, plane_->pose());
	}
	
	virtual nuklei_wmf::Vector3<double> operator()(const Vector2 &pt2) const {
		return Plane_Polygon::to3D(pt2, plane_->pose());
	}
	
	virtual Plane_Polygon operator()(const Plane &plane_other, const Plane_Polygon &poly) const {
		return Plane_Polygon(poly, *this, plane_other.pose());
	}
	
	virtual std::vector<Plane_Polygon> operator()(const Plane &plane_other, const std::vector<Plane_Polygon::Ptr> &poly) const {
		std::vector<Plane_Polygon> r(poly.size());
		for(size_t i=0; i<poly.size();  i++)
			r[i] = Plane_Polygon(*poly[i], *this, plane_other.pose());
		return r;
	}
};

struct CmpSmallestAxis {
	const Vector2 dir_;
	CmpSmallestAxis(const Vector2 &d) : dir_(d) {}
	
	bool operator()(const Plane_Point::Ptr &a, const Plane_Point::Ptr &b) const {
		return a->pos.dot(dir_)<b->pos.dot(dir_);
	}
	
	bool operator()(const Plane_Point::Ptr &a, const Vector2 &b) const {
		return a->pos.dot(dir_)<b.dot(dir_);
	}
};

}

#include "impl/plane_boost_traits.h"
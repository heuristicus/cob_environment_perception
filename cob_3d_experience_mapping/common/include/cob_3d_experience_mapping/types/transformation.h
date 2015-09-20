#pragma once

#include <Eigen/Core>
#include <boost/math/distributions/normal.hpp>

namespace cob_3d_experience_mapping {
	
	template<class TTransformation>
	class ActionSearchResult {
		typename TTransformation::TPtr trans_;
		bool found_;
	public:
	
		ActionSearchResult(typename TTransformation::TPtr trans): found_(true), trans_(trans)
		{}
		
		ActionSearchResult(const bool found): found_(found)
		{}
		
		static ActionSearchResult None() {return ActionSearchResult(false);}
		static ActionSearchResult Reached() {return ActionSearchResult(true);}
		
		bool found() {return found_;}
		bool reached() {return found_ && !trans_;}
	};
	
	template<typename _TType, int NUM_TRANS, int NUM_ROT>
	class TransformationLink {
	public:
		typedef _TType TType;
		typedef Eigen::Matrix<TType, NUM_TRANS+NUM_ROT, 1> TLink;
		typedef Eigen::Matrix<TType, 2, 1> TDist;
		typedef boost::shared_ptr<TransformationLink> TPtr;
		
	protected:
		TLink link_;
		_TType _DEVIATION;

		template<int TInd>
		inline static void helper_hystersis(TDist &d, const TDist &thr) {
			if(d(TInd)<thr(TInd)) d(TInd) = 1-d(TInd)/thr(TInd);
			else d(TInd) = 0;
		}
		
	public:
		
		TransformationLink() : link_(TLink::Zero()), _DEVIATION(0.000001f)
		{}
		
		inline _TType &deviation() {return _DEVIATION;}
		inline _TType deviation() const {return _DEVIATION;}

		TransformationLink scale(const TDist &thr) const {
			TLink tmp = link_;
			TDist d = distance(TransformationLink());
			for(int i=0; d(0) && i<NUM_TRANS; i++)
				tmp(i) *= thr(0)/d(0);
			for(int i=NUM_TRANS; d(1) && i<NUM_TRANS+NUM_ROT; i++)
				tmp(i) *= thr(1)/d(1);
			return TransformationLink(tmp);
		}

		TDist distance(const TransformationLink &o) const {
			TLink tmp = link_-o.link_;
			TDist r;
			
			r(0) = tmp.template head<NUM_TRANS>().norm();
			r(1) = tmp.template tail<NUM_ROT>  ().norm();
			
			return r;
		}
		
		void integrate(const TransformationLink &movement) {
			link_ += movement.link_;
			_DEVIATION += movement._DEVIATION;
		}
		
		inline Eigen::Matrix<TType, NUM_TRANS, 1> translation() const {return link_.template head<NUM_TRANS>();}
		inline Eigen::Matrix<TType, NUM_ROT, 1> rotation() const {return link_.template tail<NUM_ROT>();}
		
		TType dist(const TDist &thr) const {
			TDist r;
			
			r(0) = link_.template head<NUM_TRANS>().norm()/thr(0);
			r(1) = link_.template tail<NUM_ROT>  ().norm()/thr(1);
			
			return r.norm();
		}
		
		TType dist_uncertain(const TDist &thr) const {
			return dist(thr) + _DEVIATION;
		}
		
		inline static Eigen::Matrix<TType, NUM_ROT, 1> dist_rad(Eigen::Matrix<TType, NUM_ROT, 1> v) {
			for(int i=0; i<NUM_ROT; i++) {
				while(v(i)<0) v(i)+=2*M_PI;
				while(v(i)>2*M_PI) v(i)-=2*M_PI;
				if(v(i)>M_PI) v(i)=2*M_PI-v(i);
			}
			return v;
		}
		
		TType transition_factor(const TransformationLink &o, const TDist &thr) const {
			TDist r;
			TLink tmp;
			
			if(link_.squaredNorm())
				tmp = o.link_.norm()/link_.norm()*link_ + o.link_;
			else
				tmp = o.link_;
			
			r(0) = tmp.template head<NUM_TRANS>().norm()/thr(0);
			r(1) = tmp.template tail<NUM_ROT>  ().norm()/thr(1);
			
			dbg();
			o.dbg();
			std::cout<<"t "<<tmp.transpose()<<std::endl;
			std::cout<<"r "<<r.transpose()<<std::endl;
			std::cout<<"--> "<<o.dist(thr)<<"  "<<r.norm()<<std::endl;
			
			return o.dist(thr)-r.norm();
		}
		
		TType transition_factor_dbg(const TransformationLink &o, const TDist &thr) const {
			TType r = transition_factor(o,thr);
			
			dbg();
			o.dbg();
				
			return r;
		}
		
		void dbg() const {
			DBG_PRINTF("action: ");
			for(int i=0; i<NUM_TRANS+NUM_ROT; i++)
				DBG_PRINTF("%f ", link_(i));
			DBG_PRINTF("\n");
		}
		
		typename Eigen::Transform<TType,3,Eigen::Affine> affine() const {
			BOOST_STATIC_ASSERT(NUM_TRANS<=3);
			BOOST_STATIC_ASSERT(NUM_ROT==1);
			
			Eigen::Matrix<TType, 3, 1> tr = Eigen::Matrix<TType, 3, 1>::Zero();
			for(int i=0; i<NUM_TRANS; i++)
				tr(i) = link_(i);
			return Eigen::Translation<TType,3>(tr)
					* Eigen::AngleAxis<TType>(link_.template tail<NUM_ROT>()(0), Eigen::Matrix<TType, 3, 1>::UnitZ());
		}
		
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
		    ROS_ASSERT(version==0); //TODO: version handling
		    
		    for(int i=0; i<NUM_TRANS+NUM_ROT; i++) {
				char buf[16];
				sprintf(buf, "link_%d", i);
		    	ar & boost::serialization::make_nvp(buf, link_(i));
			}
		}
	};
	
	
	template<class _TransformationLink, class TStatePtr>
	class Transformation : public _TransformationLink {
	public:
		typedef boost::shared_ptr<Transformation> TPtr;
		typedef typename _TransformationLink::TLink TLink;
		typedef typename _TransformationLink::TDist TDist;
		
	protected:
		TStatePtr src_;
		
		using typename _TransformationLink::link_;
		
	public:
		
		Transformation()
		{}
		
		Transformation(const TStatePtr &state) :
			src_(state)
		{}

		Transformation(const TLink &link, const TStatePtr &state) :
			src_(state)
		{
			this->link_ = link;
		}
		
		inline const TStatePtr &src() const {return src_;}
		inline TStatePtr &src() {return src_;}

		Transformation directed(const TStatePtr &state) const {
			if(state==src_) return *this;
			return Transformation(-1*this->link_, state);
		}
	};
}

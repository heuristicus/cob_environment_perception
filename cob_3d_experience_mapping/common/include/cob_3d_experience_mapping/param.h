#pragma once

#include <std_msgs/ColorRGBA.h>
#include <dynamic_reconfigure/server.h>
#include <cob_3d_experience_mapping/SettingsConfig.h>
#include <cob_3d_experience_mapping/Vis_SettingsConfig.h>

namespace cob_3d_experience_mapping {
	
	template<class TEnergyFactor, class TDist>
	struct Parameter {
		//typedef float TScalar;
		
		TEnergyFactor algo1_energy_weight_;
		TEnergyFactor algo2_inhibition_constant_;
		
		TDist prox_thr_;
		typename TDist::Scalar energy_max_;
		
		int est_occ_;
		int min_age_;
		int max_active_cells_;

		//visualization
		std_msgs::ColorRGBA vis_color_cell_;

		dynamic_reconfigure::Server<cob_3d_experience_mapping::SettingsConfig> server_settings_;
		dynamic_reconfigure::Server<cob_3d_experience_mapping::Vis_SettingsConfig> server_vis_settings_;
		
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
		    ROS_ASSERT(version==0); //TODO: version handling
		    
		    for(int i=0; i<prox_thr_.rows(); i++)
				for(int j=0; j<prox_thr_.cols(); j++) {
					char buf[16];
					sprintf(buf, "prox_thr_%d_%d", i,j);
					ar & boost::serialization::make_nvp(buf, prox_thr_(i,j));
				}
		    ar & BOOST_SERIALIZATION_NVP(est_occ_);
		    ar & BOOST_SERIALIZATION_NVP(min_age_);
		    ar & BOOST_SERIALIZATION_NVP(max_active_cells_);
		    ar & BOOST_SERIALIZATION_NVP(energy_max_);
		}

		Parameter() {
			//TODO: read from parameter server

			server_settings_.setCallback(boost::bind(&Parameter<TEnergyFactor, TDist>::cb_settings, this, _1, _2));
			server_vis_settings_.setCallback(boost::bind(&Parameter<TEnergyFactor, TDist>::cb_vis_settings, this, _1, _2));
		}

		void cb_settings(cob_3d_experience_mapping::SettingsConfig &config, uint32_t level) {
			prox_thr_(0) = config.translation;
			prox_thr_(1) = config.rotation;
			energy_max_ = config.energy_max;
			est_occ_ = config.est_occ;
			min_age_ = config.min_age;
			max_active_cells_ = config.max_active_cells;

			ROS_INFO("updated settings");
		}

		void cb_vis_settings(cob_3d_experience_mapping::Vis_SettingsConfig &config, uint32_t level) {
			vis_color_cell_.r = config.groups.cell_color.r;
			vis_color_cell_.g = config.groups.cell_color.g;
			vis_color_cell_.b = config.groups.cell_color.b;
			vis_color_cell_.a = config.groups.cell_color.a;

			ROS_INFO("updated vis. settings");
		}
		
	};
	
}

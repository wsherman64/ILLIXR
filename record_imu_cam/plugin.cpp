#include "common/plugin.hpp"
#include "common/switchboard.hpp"
#include "common/data_format.hpp"

#include <fstream>
#include <boost/filesystem.hpp>
#include <iomanip>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
using namespace ILLIXR;

class record_imu_cam : public plugin {
public:
	record_imu_cam(std::string name_, phonebook* pb_)
		: plugin{name_, pb_}
		, sb{pb->lookup_impl<switchboard>()}
		, record_data{get_record_data_path()}
		, cam0_data_dir{record_data / "cam0" / "data"}
		, cam1_data_dir{record_data / "cam1" / "data"}
	{
		// check folder exist, if exist delete it
		boost::filesystem::remove_all(record_data);

		// create imu0 directory
		boost::filesystem::path imu_dir = record_data / "imu0";
		boost::filesystem::create_directories(imu_dir);
		std::string imu_file = imu_dir.string() + "/data.csv";
		imu_wt_file.open(imu_file, std::ofstream::out);
		imu_wt_file << "#timestamp [ns],w_x [rad s^-1],w_y [rad s^-1],w_z [rad s^-1],a_x [m s^-2],a_y [m s^-2],a_z [m s^-2]" << std::endl;

		// create cam0 directory
		boost::filesystem::create_directories(cam0_data_dir);
		std::string cam0_file = (record_data / "cam0" / "data.csv").string();
		cam0_wt_file.open(cam0_file, std::ofstream::out);
		cam0_wt_file << "#timestamp [ns],filename" << std::endl;

		// create cam1 directory
		boost::filesystem::create_directories(cam1_data_dir);
		std::string cam1_file = (record_data / "cam1" / "data.csv").string();
		cam1_wt_file.open(cam1_file, std::ofstream::out);
		cam1_wt_file << "#timestamp [ns],filename" << std::endl;

		sb->schedule<imu_cam_type>(id, "imu_cam", [this](switchboard::ptr<const imu_cam_type> datum, std::size_t){
			this->dump_data(datum);
		});
	}

	void dump_data(switchboard::ptr<const imu_cam_type> datum) {
		ullong timestamp  = datum->dataset_time;
		Eigen::Vector3f angular_v = datum->angular_v;
		Eigen::Vector3f linear_a = datum->linear_a;

		// write imu0
		imu_wt_file << datum->dataset_time << "," << std::setprecision(17) << angular_v[0] << "," << angular_v[1] << "," << angular_v[2] << "," << linear_a[0] << "," << linear_a[1] << "," << linear_a[2] << std::endl;

		// write cam0
		std::optional<cv::Mat> cam0_data = datum->img0;
		std::string cam0_img = cam0_data_dir.string() + "/" + std::to_string(timestamp) + ".png";
		if (cam0_data != std::nullopt) {
			cam0_wt_file << timestamp << "," << timestamp <<".png "<< std::endl;
			cv::imwrite(cam0_img, cam0_data.value());
		}

		// write cam1 
		std::optional<cv::Mat> cam1_data = datum->img1;
        	std::string cam1_img = cam1_data_dir.string() + "/" +std::to_string(timestamp) + ".png";
		if (cam1_data != std::nullopt) {
			cam1_wt_file << timestamp << "," << timestamp << ".png " << std::endl;
			cv::imwrite(cam1_img, cam1_data.value());
		}
	}

	virtual ~record_imu_cam() override { 
		imu_wt_file.close();
		cam0_wt_file.close();
		cam1_wt_file.close();
	}

private:
	std::ofstream imu_wt_file;
	std::ofstream cam0_wt_file;
	std::ofstream cam1_wt_file;
	const std::shared_ptr<switchboard> sb;

	const boost::filesystem::path record_data; 
	const boost::filesystem::path cam0_data_dir;
	const boost::filesystem::path cam1_data_dir;

	// TODO: This should come from a yaml file
	boost::filesystem::path get_record_data_path() {
		boost::filesystem::path ILLIXR_DIR = boost::filesystem::current_path();
		return ILLIXR_DIR / "data_record";
	}
};
// This line makes the plugin importable by Spindle
PLUGIN_MAIN(record_imu_cam);

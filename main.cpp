#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "iostream"

// my code
#include "Smooth.h"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
	po::options_description desc("Smooth given vessel surface");

	float smoothFactor;
	int imageSize;

	desc.add_options()
		("help,h", "Displays this help.")
		("input,i",po::value<std::string>(),"File to process.")
		("centerline,c", po::value<std::string>(),"Centerline file to process.")
		("output,o", po::value<std::string>(),"Output file path.")
		("smooth,s", po::value<float>(&smoothFactor)->default_value(0.6), "Voronoi diagram smooth factor between 0 and 1, where 0 indicates minimal smoothing (default = 0.6)")
		("imageSize,a", po::value<int>(&imageSize)->default_value(90), "PolyBall image size used for marching cube surface generation, higher is more detail (default = 90)")

		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	notify(vm);

	if (vm.count("help")) 
	{
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("input")) 
	{
		// check existence of the input file
		if (!boost::filesystem::exists(vm["input"].as<std::string>()))
		{
			std::cout << "Input file not exists" << std::endl;
			return 1;
		}
	}
	else
	{
		std::cout << "Input file not given" << std::endl;
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("centerline"))
	{
		// check existence of the centerline file
		if (!boost::filesystem::exists(vm["centerline"].as<std::string>()))
		{
			std::cout << "Centerline file not exists" << std::endl;
			return 1;
		}
	}
	else
	{
		std::cout << "Centerline file not given" << std::endl;
		std::cout << desc << "\n";
		return 1;
	}

	if (!vm.count("output"))
	{
		std::cout << "Output file not given" << std::endl;
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("smooth"))
	{
		// check smooth factor between 0 and 1
		if (vm["smooth"].as<float>() < 0 || vm["smooth"].as<float>()>1)
		{
			std::cout << "Smooth factor should between 0 and 1." << std::endl;
			return 1;
		}
	}

	if (vm.count("imageSize"))
	{
		// check smooth factor between 0 and 1
		if (vm["imageSize"].as<int>() <= 0)
		{
			std::cout << "PolyBall image size should be integer > 0." << std::endl;
			return 1;
		}
	}

	Smooth smooth;
	smooth.SetInputPath(vm["input"].as<std::string>());
	smooth.SetCenterlinePath(vm["centerline"].as<std::string>());
	smooth.SetOutputPath(vm["output"].as<std::string>());
	smooth.SetSmoothFactor(vm["smooth"].as<float>());
	smooth.SetPolyBallImageSize(vm["imageSize"].as<int>());
	smooth.Run();
}
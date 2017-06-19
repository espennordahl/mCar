#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <memory>


#include "openMVG/sfm/sfm.hpp"
#include "openMVG/exif/exif_IO_EasyExif.hpp"
#include "openMVG/image/image.hpp"
#include "openMVG/stl/split.hpp"

#include "openMVG/matching_image_collection/Matcher_Regions_AllInMemory.hpp"
#include "openMVG/matching_image_collection/GeometricFilter.hpp"
#include "openMVG/matching_image_collection/F_ACRobust.hpp"
#include "openMVG/matching_image_collection/Pair_Builder.hpp"
#include "openMVG/cameras/Cameras_Common_command_line_helper.hpp"
#include "openMVG/sfm/sfm.hpp"


class ImageList {
	public:
	ImageList(const std::string &foldername);
	void generateFeatures();
	void computeMatches();
	void sequentialReconstruct();

	private:
  	std::string _directory;
  	std::string _matches_full;
  	std::string _matches_filtered;
  	std::vector<std::string> _fnames;
  	openMVG::sfm::SfM_Data _sfm_data;
  	unique_ptr<openMVG::features::Regions> _regionsType;

	void loadImage(const std::string &filename);
};

ImageList::ImageList(const std::string &foldername){
	std::cout << "ImageList constructor" << std::endl;
	vector<string> file_list;
  	_directory = foldername;
  	_sfm_data.s_root_path = foldername;
  	file_list = stlplus::folder_files(_directory);

  	sort(file_list.begin(), file_list.end());

	// List images in directory

  	for (std::vector<std::string>::const_iterator it = file_list.begin(); it != file_list.end(); it++)  {
    	std::string which = *it;
    	std::string imagename = stlplus::create_filespec(_directory, which);

		std::cout << "adding image to list: " << imagename << std::endl;
    	_fnames.push_back(imagename);
  	}
  	_matches_full = string(_directory + "/matches.putative.txt");
  	_matches_filtered = string(_directory + "/matches.f.txt");

	// load images
	for ( vector<string>::const_iterator iter_image = _fnames.begin(); iter_image != _fnames.end(); iter_image++ ) {
		string which = *iter_image;
		std::cout << "Reading image: " << which << std::endl;
		loadImage(which);
  	}
	stdd:cout << "Loaded all images" << std::endl;

}

void ImageList::loadImage(const std::string &filename) {
	double width, height, image_focal;
	double focal, ppx, ppy;

	openMVG::sfm::Views& views = _sfm_data.views;
	openMVG::sfm::Intrinsics& intrinsics = _sfm_data.intrinsics;
	shared_ptr<openMVG::cameras::IntrinsicBase> intrinsic (NULL);

  	if (openMVG::image::GetFormat(filename.c_str()) == openMVG::image::Unknown){
		std::cout << "Unknown image format. Skipping." << std::endl;
    	return;
	}

	unique_ptr<openMVG::exif::Exif_IO_EasyExif> exifReader(new openMVG::exif::Exif_IO_EasyExif());
  	if (!exifReader->open(filename)){
		std::cout << "Unable to open file. Skipping." << std::endl;
    	return;
	}

	image_focal = static_cast<double>(exifReader->getFocal());
  	width = static_cast<double>(exifReader->getWidth());
  	height = static_cast<double>(exifReader->getHeight());

	//const double ccdw = 5.75; // Cheap Samsung S890
  	//const double ccdw = 4.62;// Also Cheap Canon SX410-IS
  	//const double ccdw = 7.39; // Nicer Canon G9
  	//const double ccdw = 35.9; // Very Nice Sony DSLR/A850
	const double ccdw = 4.89; // iPhone 6
  	focal = std::max (width, height) * image_focal / ccdw;

	std::cout << "width : " << width << std::endl;
  	std::cout << "height : " << height << std::endl;
  	std::cout << "focal : " << image_focal << std::endl;
  	std::cout << "brand : " << exifReader->getBrand() << std::endl;
  	std::cout << "model : " << exifReader->getModel() << std::endl;
 
	// TODO: Use database for focal in pixels
 	focal = std::max (width, height) * image_focal / ccdw;

  	openMVG::sfm::View v(filename, views.size(), views.size(), views.size(), width, height);

  	intrinsic = make_shared<openMVG::cameras::Pinhole_Intrinsic_Radial_K3> (width, height, focal,
                                                        ppx, ppy, 0.0, 0.0, 0.0);
  	intrinsics[v.id_intrinsic] = intrinsic;
  	views[v.id_view] = std::make_shared<openMVG::sfm::View>(v);
}


void ImageList::generateFeatures(){
	std::cout << "generateFeatures()" << std::endl;

	unique_ptr<openMVG::features::AKAZE_Image_describer> image_describer(new openMVG::features::AKAZE_Image_describer(openMVG::features::AKAZE::Params(), openMVG::features::AKAZE_MSURF));

  	image_describer->Allocate(_regionsType);
  	image_describer->Set_configuration_preset(openMVG::features::NORMAL_PRESET);
	
	// TODO: Multi thread
  	for(openMVG::sfm::Views::const_iterator iterViews = _sfm_data.views.begin(); iterViews != _sfm_data.views.end(); ++iterViews) {
  		const openMVG::sfm::View * view = iterViews->second.get();
  		const string sFeat = stlplus::create_filespec(_directory, stlplus::basename_part(view->s_Img_path), "feat");
  		const string sDesc = stlplus::create_filespec(_directory, stlplus::basename_part(view->s_Img_path), "desc");

    	if (!stlplus::file_exists(sFeat)) {
      		openMVG::image::Image<unsigned char> imageGray;
      		printf("Creating %s\n", sFeat.c_str());
      		ReadImage(view->s_Img_path.c_str(), &imageGray);
      		unique_ptr<openMVG::features::Regions> regions;
      		image_describer->Describe(imageGray, regions);
      		image_describer->Save(regions.get(), sFeat, sDesc);
    	}
    	else {
      		std::cout << "Using existing features from " << sFeat.c_str() << std::endl;
    	}
  	}
}

void ImageList::computeMatches(){
	std::cout << "computeMatches()" << std::endl;

	float fDistRatio = 2.f; // Higher is stricter (???)
	openMVG::matching::PairWiseMatches map_PutativesMatches;
	std::vector<std::string> vec_fileNames;
	std::vector<std::pair<size_t, size_t> > vec_imagesSize;

  	for (openMVG::sfm::Views::const_iterator iter = _sfm_data.GetViews().begin(); iter != _sfm_data.GetViews().end(); ++iter) {
    	const openMVG::sfm::View * v = iter->second.get();
    	vec_fileNames.push_back(stlplus::create_filespec(_sfm_data.s_root_path, v->s_Img_path));
    	vec_imagesSize.push_back(make_pair( v->ui_width, v->ui_height) );
  	}

	std::shared_ptr<openMVG::sfm::Regions_Provider> regions_provider;
	regions_provider = std::make_shared<openMVG::sfm::Regions_Provider>();

  	if (!regions_provider->load(_sfm_data, _directory, _regionsType)) {
    	std::cerr << std::endl << "Invalid regions." << std::endl;
    	return;
  	}

  	std::unique_ptr<openMVG::matching_image_collection::Matcher_Regions_AllInMemory> collectionMatcher;
  	collectionMatcher.reset(new openMVG::matching_image_collection::Matcher_Regions_AllInMemory(fDistRatio, openMVG::ANN_L2));

	std::cout << "Building pairs" << std::endl;

    openMVG::Pair_Set pairs;
    pairs = openMVG::exhaustivePairs(_sfm_data.GetViews().size());
    collectionMatcher->Match(_sfm_data, regions_provider, pairs, map_PutativesMatches);

    if (!Save(map_PutativesMatches, std::string(_directory + "/matches.putative.txt"))){
		 std::cerr << "Cannot save computed matches in: " << std::string(_directory + "/matches.putative.txt");
	return;
	}

  	std::shared_ptr<openMVG::sfm::Features_Provider> feats_provider = make_shared<openMVG::sfm::Features_Provider>();

  	if (!feats_provider->load(_sfm_data, _directory, _regionsType))
    	return;

  	openMVG::PairWiseMatches map_GeometricMatches;

  	std::unique_ptr<openMVG::matching_image_collection::ImageCollectionGeometricFilter> filter_ptr(new openMVG::matching_image_collection::ImageCollectionGeometricFilter(&_sfm_data, regions_provider));

  	const double maxResidualError = 10.0; // dflt 1.0; // Lower is stricter

  	filter_ptr->Robust_model_estimation(
            openMVG::matching_image_collection::GeometricFilter_FMatrix_AC(maxResidualError, 1024),
            map_PutativesMatches, false);
	map_GeometricMatches = filter_ptr->Get_geometric_matches();

  	ofstream file (_matches_filtered);

    if (!Save(map_GeometricMatches,
      std::string(_directory + "/matches.f.txt"))){
      	std::cerr
          << "Cannot save computed matches in: "
          << std::string(_directory + "/matches.f.txt");
      	return;
    }
}

void ImageList::sequentialReconstruct(){
	std::cout << "sequentialReconstruct()" << std::endl;

	std::string output_directory = "sequential";
	std::string sfm_data = stlplus::create_filespec(output_directory, "sfm_data", ".json");
	std::string cloud_data = stlplus::create_filespec(output_directory, "cloud_and_poses", ".ply");
	std::string report_name = stlplus::create_filespec(output_directory, "Reconstruction_Report", ".html");

  	if (!stlplus::folder_exists(output_directory)){
		std::cout << "Creating directory" << std::endl;
    	stlplus::folder_create(output_directory);
	}

  	openMVG::sfm::SequentialSfMReconstructionEngine sfmEngine(_sfm_data, output_directory, report_name);
  	std::shared_ptr<openMVG::sfm::Features_Provider> feats_provider = std::make_shared<openMVG::sfm::Features_Provider>();
  	std::shared_ptr<openMVG::sfm::Matches_Provider> matches_provider = std::make_shared<openMVG::sfm::Matches_Provider>();

  	feats_provider->load(_sfm_data, _directory, _regionsType);
  	matches_provider->load(_sfm_data, _matches_filtered);

  	sfmEngine.SetFeaturesProvider(feats_provider.get());
  	sfmEngine.SetMatchesProvider(matches_provider.get());

	openMVG::cameras::Intrinsic_Parameter_Type intrinsic_refinement_options =
    openMVG::cameras::StringTo_Intrinsic_Parameter_Type("ADJUST_ALL");
  	sfmEngine.Set_Intrinsics_Refinement_Type(intrinsic_refinement_options);
	
  	sfmEngine.SetUnknownCameraType(openMVG::cameras::PINHOLE_CAMERA_RADIAL3);
  	sfmEngine.Set_Use_Motion_Prior(false);


  	openMVG::Pair initialPairIndex;
  	openMVG::sfm::Views::const_iterator it;

  	it = _sfm_data.GetViews().begin();
  	const openMVG::sfm::View *v1 = it->second.get();
  	it++;
  	const openMVG::sfm::View *v2 = it->second.get();


  	initialPairIndex.first = v1->id_view;
  	initialPairIndex.second = v2->id_view;

//  	sfmEngine.setInitialPair(initialPairIndex);

  	if(sfmEngine.Process()){
		std::cout << "generating SfM report" << std::endl;
		Generate_SfM_Report(sfmEngine.Get_SfM_Data(), create_filespec);
  		Save(sfmEngine.Get_SfM_Data(), sfm_data, openMVG::sfm::ESfM_Data(openMVG::sfm::ALL));
  		Save(sfmEngine.Get_SfM_Data(), cloud_data, openMVG::sfm::ESfM_Data(openMVG::sfm::ALL));
	} else {
		std::cout << "sfmEngine.Process() failed." << std::endl;
	}
}




int main (int argc, char *argv[])
{
	std::cout << "HERRO!" << std::endl;

	ImageList iml = ImageList(std::string("/Users/espennordahl/Desktop/openMVGtest"));
	iml.generateFeatures();
	iml.computeMatches() ;
	iml.sequentialReconstruct();

	return 0;
}

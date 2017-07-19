#!/usr/bin/python
#! -*- encoding: utf-8 -*-


#TODO: dmap files from mvs end up where script is ran from
#TODO: jpeg files from gopro end up where the script is ran from

import os
import subprocess
import sys
import argparse
import datetime

from goprocam import GoProCamera
from goprocam import constants


OPENMVG_SFM_BIN = "/Users/espennordahl/openMVG/build/Darwin-x86_64-RELEASE/"
OPENMVS_BIN = "/Users/espennordahl/openMVS_build/bin/"
CAMERA_SENSOR_WIDTH_DIRECTORY = "/Users/espennordahl/openMVG/openMVG/src/software/SfM" + "/../../openMVG/exif/sensor_width_database"



def take_photos(output_dir, numphotos):
	gpCam = GoProCamera.GoPro()
	for i in range(0, numphotos):
		filename = gpCam.take_photo(0)
		print("take_photo done. Filename: {0} Downloading.".format(filename))
		gpCam.downloadLastMedia(filename)

def run_openmvs(input_dir, output_dir):
	print ("Using input dir  : ", input_dir)
	print ("      output dir : ", output_dir)

	if not os.path.exists(output_dir):
	  os.mkdir(output_dir)


	print("1. Converting scene from mvg to mvs")
	pConvert = subprocess.Popen([os.path.join(OPENMVG_SFM_BIN, "openMVG_main_openMVG2openMVS"), 
									"-i", os.path.join(input_dir, "reconstruction_global", "sfm_data.bin"),
									"-d", output_dir,
									"-o", os.path.join(output_dir, "scene.mvs")])
	pConvert.wait()

	print("2. Refining pointcloud")
	pRefine = subprocess.Popen([os.path.join(OPENMVS_BIN, "DensifyPointCloud"), os.path.join(output_dir, "scene.mvs")])
	pRefine.wait()

def run_openmvg(input_dir, output_dir):

	matches_dir = os.path.join(output_dir, "matches")
	reconstruction_dir = os.path.join(output_dir, "reconstruction_global")
	camera_file_params = os.path.join(CAMERA_SENSOR_WIDTH_DIRECTORY, "sensor_width_camera_database.txt")

	print ("Using input dir  : ", input_dir)
	print ("      output dir : ", output_dir)

	# Create the ouput/matches folder if not present
	if not os.path.exists(output_dir):
	  os.mkdir(output_dir)
	if not os.path.exists(matches_dir):
	  os.mkdir(matches_dir)

	print ("1. Intrinsics analysis")
	pIntrisics = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_SfMInit_ImageListing"),  "-i", input_dir, "-o", matches_dir, "-d", camera_file_params, "-c", "5"])
	pIntrisics.wait()

	print ("2. Compute features")
	pFeatures = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_ComputeFeatures"),  "-i", matches_dir+"/sfm_data.json", "-o", matches_dir, "-m", "SIFT", "-p", "HIGH"] )
	pFeatures.wait()

	print ("3. Compute matches")
	pMatches = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_ComputeMatches"),  "-i", matches_dir+"/sfm_data.json", "-o", matches_dir, "-g", "f", "-r", "0.8", "-n", "ANNL2"] )
	pMatches.wait()

	# Create the reconstruction if not present
	if not os.path.exists(reconstruction_dir):
		os.mkdir(reconstruction_dir)

#	print ("4. Do Global reconstruction")
#	pRecons = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_GlobalSfM"),  "-i", matches_dir+"/sfm_data.json", "-m", matches_dir, "-o", reconstruction_dir] )
#	pRecons.wait()


	print ("4b. Do Incremental reconstruction")
	pRecons = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_IncrementalSfM"),  "-i", matches_dir+"/sfm_data.json", "-m", matches_dir, "-o", reconstruction_dir, "-c", "5" ] )
	pRecons.wait()



	print ("5. Colorize Structure")
	pRecons = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_ComputeSfM_DataColor"),  "-i", reconstruction_dir+"/sfm_data.bin", "-o", os.path.join(reconstruction_dir,"colorized.ply")] )
	pRecons.wait()

	# optional, compute final valid structure from the known camera poses
	print ("6. Structure from Known Poses (robust triangulation)")
	pRecons = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_ComputeStructureFromKnownPoses"),  "-i", reconstruction_dir+"/sfm_data.bin", "-m", matches_dir, "-f", os.path.join(matches_dir, "matches.e.bin"), "-o", os.path.join(reconstruction_dir,"robust.bin")] )
	pRecons.wait()

	pRecons = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_ComputeSfM_DataColor"),  "-i", reconstruction_dir+"/robust.bin", "-o", os.path.join(reconstruction_dir,"robust_colorized.ply")] )
	pRecons.wait()

if __name__ == "__main__":
	argparser = argparse.ArgumentParser()
	argparser.add_argument("-g", "--gopro", type=bool, help="Take photos with the gopro", default=False)
	argparser.add_argument("-b", "--build", type=bool, help="Build pointcloud using openMVG", default=False)
	argparser.add_argument("-r", "--refine", type=bool, help="Refine pointcloud using openMVS", default=False)
	argparser.add_argument("-n", "--numphotos", type=int, help="Number of photos to take.", default=10)
	args = argparser.parse_args()

	basepath = "/Users/espennordahl/Desktop/goproTest/"

	if args.gopro:
		print ("Taking pictures with GoPro.")
		timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
		basepath = os.path.join(basepath, timestamp)
		goproDir = os.path.join(basepath, "gopro")
		take_photos(goproDir, args.numphotos)
		print ("Done taking pictures.")
	else:
		print("Skipping go pro step.")
		basefolders = os.listdir(basepath)
		folders = []
		for folder in basefolders:
			if folder.startswith("."):
				continue
			if "gopro" in os.listdir(os.path.join(basepath, folder)):
				folders.append(folder)
		if not folders:
			raise Exception("couldn't find folders containing gopro photos.")
		folders.sort()
		basepath = os.path.join(basepath, folders[-1])
		goproDir = os.path.join(basepath, "gopro")
		print("Using these previously generated gopro photos: " + goproDir)

	if args.build:
		print ("Running openMVG pipeline.")
		mvgDir = os.path.join(basepath, "mvg")
		run_openmvg(goproDir, mvgDir)
		print ("MVG process completed.")
	else:
		print("Skipping openMVG.")
		mvgDir = os.path.join(basepath, "mvg")

	if args.refine:
		print ("Running openMVS pipeline.")
		mvsDir = os.path.join(basepath, "mvs")
		run_openmvs(mvgDir, mvsDir)
		print ("MVS pipeline completed. Final pointclouds are located here: ", mvsDir)
	else:
		print("Skipping openMVS.")

	print ("gg wp :)")



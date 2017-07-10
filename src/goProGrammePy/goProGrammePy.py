#!/usr/bin/python
#! -*- encoding: utf-8 -*-


import os
import subprocess
import sys

from goprocam import GoProCamera
from goprocam import constants

def take_photos(output_dir, numphotos):
	gpCam = GoProCamera.GoPro()
	for i in range(0, numphotos):
		filename = gpCam.take_photo(0)
		print("take_photo done. Filename: {0} Downloading.".format(filename))
		gpCam.downloadLastMedia(filename)

def run_openmvs(input_dir, output_dir):
	print("lol jk")

def run_openmvg(input_dir, output_dir):
	OPENMVG_SFM_BIN = "/Users/espennordahl/openMVG/build/Darwin-x86_64-RELEASE/"
	CAMERA_SENSOR_WIDTH_DIRECTORY = "/Users/espennordahl/openMVG/openMVG/src/software/SfM" + "/../../openMVG/exif/sensor_width_database"


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
	pMatches = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_ComputeMatches"),  "-i", matches_dir+"/sfm_data.json", "-o", matches_dir, "-g", "e", "-r", "2.0", "-n", "ANNL2"] )
	pMatches.wait()

	# Create the reconstruction if not present
	if not os.path.exists(reconstruction_dir):
		os.mkdir(reconstruction_dir)

	print ("4. Do Global reconstruction")
	pRecons = subprocess.Popen( [os.path.join(OPENMVG_SFM_BIN, "openMVG_main_GlobalSfM"),  "-i", matches_dir+"/sfm_data.json", "-m", matches_dir, "-o", reconstruction_dir] )
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

	basepath = "/Users/espennordahl/Desktop/goproTest"
	goproDir = basepath + "/gopro/"
	mvgDir = basepath + "/mvg/"
	mvsDir = basepath + "/mvs"

	print ("Taking pictures with GoPro.")
	#take_photos(goproDir, 30)
	print ("Done taking pictures.")

	print ("Running openMVG pipeline.")
	run_openmvg(goproDir, mvgDir)
	print ("MVG process completed.")

	print ("Running openMVS pipeline.")
	run_openmvs(mvgDir, mvsDir)
	print ("MVS pipeline completed. Final pointclouds are located here: ", mvsDir)

	print ("gg wp :)")



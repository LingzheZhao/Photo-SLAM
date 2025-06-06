/**
* This file modified from Photo-SLAM.
*
 * Copyright (C) 2023-2024 Longwei Li and Hui Cheng, Sun Yat-sen University.
 * Copyright (C) 2023-2024 Huajian Huang and Sai-Kit Yeung, Hong Kong University of Science and Technology.
 *
 * Photo-SLAM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Photo-SLAM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Photo-SLAM.
 * If not, see <http://www.gnu.org/licenses/>.
*/

#include <torch/torch.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sys/wait.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>


#include<opencv2/core/core.hpp>
#include<opencv2/imgcodecs/legacy/constants_c.h>

#include "ORB-SLAM3/include/System.h"
#include "include/gaussian_mapper.h"
#include "viewer/imgui_viewer.h"

namespace fs = std::filesystem;

void LoadImages(const fs::path &file, vector<fs::path> &vImageFilenames);
void saveTrackingTime(std::vector<float> &vTimesTrack, const fs::path &savePath);
void saveGpuPeakMemoryUsage(const fs::path &pathSave);

int main(int argc, char **argv)
{
    if (argc != 6 && argc != 7)
    {
        std::cerr << std::endl
                  << "Usage: " << argv[0]
                  << " path_to_vocabulary"                   /*1*/
                  << " path_to_ORB_SLAM3_settings"           /*2*/
                  << " path_to_gaussian_mapping_settings"    /*3*/
                  << " path_to_sequence"                     /*4*/
                  << " path_to_trajectory_output_directory/" /*5*/
                  << " (optional)no_viewer"                  /*6*/
                  << std::endl;
        return 1;
    }
    bool use_viewer = true;
    if (argc == 7)
        use_viewer = (std::string(argv[6]) == "no_viewer" ? false : true);

    std::string output_directory = std::string(argv[5]);
    if (output_directory.back() != '/')
        output_directory += "/";
    std::filesystem::path output_dir(output_directory);

    // Retrieve paths to images
    vector<fs::path> vImageFilenames;
    vector<fs::path> vDepthFilenames;
    const fs::path sequence_dir(argv[4]);
    const fs::path color_dir = sequence_dir / "color";
    assert(fs::exists(color_dir));
    LoadImages(color_dir, vImageFilenames);
    const fs::path depth_dir = sequence_dir / "depth";
    assert(fs::exists(depth_dir));
    LoadImages(depth_dir, vDepthFilenames);

//    for (auto s : vstrImageFilenames) cout << s << endl;
//    for (auto s : vstrDepthFilenames) cout << s << endl;

    // Check consistency in the number of images
    int nImages = vImageFilenames.size();
    if (vImageFilenames.empty())
    {
        std::cerr << std::endl << "No images found in provided path." << std::endl;
        return 1;
    }
    else if (vDepthFilenames.size() != vImageFilenames.size())
    {
        std::cerr << std::endl << "Different number of images for rgb and depth." << std::endl;
        return 1;
    }

    // Device
    torch::DeviceType device_type;
    if (torch::cuda::is_available())
    {
        std::cout << "CUDA available! Training on GPU." << std::endl;
        device_type = torch::kCUDA;
    }
    else
    {
        std::cout << "Training on CPU." << std::endl;
        device_type = torch::kCPU;
    }

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    std::shared_ptr<ORB_SLAM3::System> pSLAM =
        std::make_shared<ORB_SLAM3::System>(
            argv[1], argv[2], ORB_SLAM3::System::RGBD);
    // float imageScale = pSLAM->GetImageScale();

    // Create GaussianMapper
    std::filesystem::path gaussian_cfg_path(argv[3]);
    std::shared_ptr<GaussianMapper> pGausMapper =
        std::make_shared<GaussianMapper>(
            pSLAM, gaussian_cfg_path, output_dir, 0, device_type);
    std::thread training_thd(&GaussianMapper::run, pGausMapper.get());

    // Create Gaussian Viewer
    std::thread viewer_thd;
    std::shared_ptr<ImGuiViewer> pViewer;
    if (use_viewer)
    {
        pViewer = std::make_shared<ImGuiViewer>(pSLAM, pGausMapper);
        viewer_thd = std::thread(&ImGuiViewer::run, pViewer.get());
    }

    // Vector for tracking time statistics
    std::vector<float> vTimesTrack;
    vTimesTrack.resize(nImages);

    std::cout << std::endl << "-------" << std::endl;
    std::cout << "Start processing sequence ..." << std::endl;
    std::cout << "Images in the sequence: " << nImages << std::endl << std::endl;

    // Main loop
    cv::Mat imRGB, imD;
    for (int ni = 0; ni < nImages; ni++)
    {
        if (pSLAM->isShutDown())
            break;
        // Read image and depthmap from file
        assert(vImageFilenames[0].string().length()>0);
        imRGB = cv::imread(color_dir / vImageFilenames[ni], cv::IMREAD_UNCHANGED);
        cv::cvtColor(imRGB, imRGB, CV_BGR2RGB);
        assert(vDepthFilenames[0].string().length()>0);
        imD = cv::imread(depth_dir / vDepthFilenames[ni], cv::IMREAD_UNCHANGED);
        double tframe = ni;

        if (imRGB.empty())
        {
            std::cerr << std::endl << "Failed to load image at: "
                      << vImageFilenames[ni] << std::endl;
            return 1;
        }
        if (imD.empty())
        {
            std::cerr << std::endl << "Failed to load image at: "
                      << vDepthFilenames[ni] << std::endl;
            return 1;
        }

        cv::Size targetImSize = pSLAM->getSettings()->originalImSize();
        int actualWidth = imRGB.cols;
        int actualHeight = imRGB.rows;

        float targetWidthScale = float(targetImSize.width) / float(actualWidth);
        float targetHeightScale = float(targetImSize.height) / float(actualHeight);
        if (targetWidthScale != 1.f || targetHeightScale != 1.f)
        {
            int width = imRGB.cols * targetWidthScale;
            int height = imRGB.rows * targetHeightScale;
            cv::resize(imRGB, imRGB, cv::Size(width, height));
            cv::resize(imD, imD, cv::Size(width, height));
        }

        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        // Pass the image to the SLAM system
        pSLAM->TrackRGBD(imRGB, imD, tframe, std::vector<ORB_SLAM3::IMU::Point>(), vImageFilenames[ni]);

        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

        double ttrack = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
        vTimesTrack[ni] = ttrack;
    }

    // Stop all threads
    pSLAM->Shutdown();
    training_thd.join();
    if (use_viewer)
        viewer_thd.join();

    // GPU peak usage
    saveGpuPeakMemoryUsage(output_dir / "GpuPeakUsageMB.txt");

    // Tracking time statistics
    saveTrackingTime(vTimesTrack, (output_dir / "TrackingTime.txt").string());

    // Save camera trajectory
    pSLAM->SaveTrajectoryTUM((output_dir / "CameraTrajectory_TUM.txt").string());
    pSLAM->SaveKeyFrameTrajectoryTUM((output_dir / "KeyFrameTrajectory_TUM.txt").string());
    pSLAM->SaveTrajectoryEuRoC((output_dir / "CameraTrajectory_EuRoC.txt").string());
    pSLAM->SaveKeyFrameTrajectoryEuRoC((output_dir / "KeyFrameTrajectory_EuRoC.txt").string());
    pSLAM->SaveTrajectoryKITTI((output_dir / "CameraTrajectory_KITTI.txt").string());

    return 0;
}

void ls(const fs::path &path, vector<fs::path> &vImageFilenames) {
    DIR *mydir;
    struct dirent *myfile;
    struct stat mystat;

    mydir = opendir(path.c_str());
    if (!mydir) {
        cerr << "Unable to open " << path << endl;
        exit(1);
    }
    while((myfile = readdir(mydir)) != NULL)
    {
        stat(myfile->d_name, &mystat);
        auto filename = fs::path(myfile->d_name);
        if (filename != "." && filename != "..")
            vImageFilenames.push_back(filename);
    }
    closedir(mydir);

    sort(vImageFilenames.begin(),
        vImageFilenames.end(),
        [](const fs::path& a, const fs::path& b) {
            return stoi(a.stem().string()) < stoi(b.stem().string());
    });
}

void LoadImages(const fs::path &file, vector<fs::path> &vImageFilenames)
{
    ls(file, vImageFilenames);
}

void saveTrackingTime(std::vector<float> &vTimesTrack, const fs::path &savePath)
{
    std::ofstream out;
    out.open(savePath);
    std::size_t nImages = vTimesTrack.size();
    float totaltime = 0;
    for (int ni = 0; ni < nImages; ni++)
    {
        out << std::fixed << std::setprecision(4)
            << vTimesTrack[ni] << std::endl;
        totaltime += vTimesTrack[ni];
    }

    // std::sort(vTimesTrack.begin(), vTimesTrack.end());
    // out << "-------" << std::endl;
    // out << std::fixed << std::setprecision(4)
    //     << "median tracking time: " << vTimesTrack[nImages / 2] << std::endl;
    // out << std::fixed << std::setprecision(4)
    //     << "mean tracking time: " << totaltime / nImages << std::endl;

    out.close();
}

void saveGpuPeakMemoryUsage(const fs::path &pathSave)
{
    namespace c10Alloc = c10::cuda::CUDACachingAllocator;
    c10Alloc::DeviceStats mem_stats = c10Alloc::getDeviceStats(0);

    c10Alloc::Stat reserved_bytes = mem_stats.reserved_bytes[static_cast<int>(c10Alloc::StatType::AGGREGATE)];
    float max_reserved_MB = reserved_bytes.peak / (1024.0 * 1024.0);

    c10Alloc::Stat alloc_bytes = mem_stats.allocated_bytes[static_cast<int>(c10Alloc::StatType::AGGREGATE)];
    float max_alloc_MB = alloc_bytes.peak / (1024.0 * 1024.0);

    std::ofstream out(pathSave);
    out << "Peak reserved (MB): " << max_reserved_MB << std::endl;
    out << "Peak allocated (MB): " << max_alloc_MB << std::endl;
    out.close();
}

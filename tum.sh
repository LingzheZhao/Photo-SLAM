./bin/tum_rgbd \
    ./ORB-SLAM3/Vocabulary/ORBvoc.txt \
    ./cfg/ORB_SLAM3/RGB-D/TUM/tum_freiburg1_desk.yaml \
    ./cfg/gaussian_mapper/RGB-D/TUM/tum_freiburg1_desk.yaml \
    ./data/TUM/rgbd_dataset_freiburg1_desk/ \
    ./cfg/ORB_SLAM3/RGB-D/TUM/associations/tum_freiburg1_desk.txt \
    ./outputs/TUM/rgbd_dataset_freiburg1_desk \
    no_viewer

./bin/tum_rgbd \
    ./ORB-SLAM3/Vocabulary/ORBvoc.txt \
    ./cfg/ORB_SLAM3/RGB-D/TUM/tum_freiburg2_xyz.yaml \
    ./cfg/gaussian_mapper/RGB-D/TUM/tum_freiburg2_xyz.yaml \
    ./data/TUM/rgbd_dataset_freiburg2_xyz/ \
    ./cfg/ORB_SLAM3/RGB-D/TUM/associations/tum_freiburg2_xyz.txt \
    ./outputs/TUM/rgbd_dataset_freiburg2_xyz \
    no_viewer

./bin/tum_rgbd \
    ./ORB-SLAM3/Vocabulary/ORBvoc.txt \
    ./cfg/ORB_SLAM3/RGB-D/TUM/tum_freiburg3_long_office_household.yaml \
    ./cfg/gaussian_mapper/RGB-D/TUM/tum_freiburg3_long_office_household.yaml \
    ./data/TUM/rgbd_dataset_freiburg3_long_office_household/ \
    ./cfg/ORB_SLAM3/RGB-D/TUM/associations/tum_freiburg3_long_office_household.txt \
    ./outputs/TUM/rgbd_dataset_freiburg3_long_office_household \
    no_viewer

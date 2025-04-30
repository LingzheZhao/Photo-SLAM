export OPENCV_IO_ENABLE_OPENEXR=1
./bin/replica_rgbd \
    ./ORB-SLAM3/Vocabulary/ORBvoc.txt \
    ./cfg/ORB_SLAM3/RGB-D/MBA-VO/common.yaml \
    ./cfg/gaussian_mapper/RGB-D/MBA-VO/common.yaml \
    ./data/MBA-VO/archviz1 \
    ./outputs/MBA-VO/archviz1 \
    no_viewer

./bin/replica_rgbd \
    ./ORB-SLAM3/Vocabulary/ORBvoc.txt \
    ./cfg/ORB_SLAM3/RGB-D/MBA-VO/common.yaml \
    ./cfg/gaussian_mapper/RGB-D/MBA-VO/common.yaml \
    ./data/MBA-VO/archviz2 \
    ./outputs/MBA-VO/archviz2 \
    no_viewer

./bin/replica_rgbd \
    ./ORB-SLAM3/Vocabulary/ORBvoc.txt \
    ./cfg/ORB_SLAM3/RGB-D/MBA-VO/common.yaml \
    ./cfg/gaussian_mapper/RGB-D/MBA-VO/common.yaml \
    ./data/MBA-VO/archviz3_020_first_sharp \
    ./outputs/MBA-VO/archviz3_020_first_sharp \
    no_viewer

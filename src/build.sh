for dir in {nvxio,libdnn_trt,libCarEye,careyevw,careye_safe}
do
    echo "Building " $dir
    cd $dir
    qmake
    make -j4
    sudo make install
    cd ..
done


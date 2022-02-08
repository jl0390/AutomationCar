for dir in {libdnn_trt,libCarEye,careyevw,careye_safe}
do
    cd $dir
    make clean
    rm Makefile*
    cd ..
done

cd nvxio;rm -rf obj;cd ..
rm -rf libs

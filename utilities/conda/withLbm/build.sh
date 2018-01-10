mkdir build
cd build
export BOOST_ROOT=$PREFIX

cmake .. -DWALBERLA_BUILD_WITH_PYTHON=1 -DWALBERLA_BUILD_WITH_PYTHON_MODULE=1 -DWALBERLA_BUILD_WITH_PYTHON_LBM=1 -DWALBERLA_BUILD_WITH_OPENMESH=0

make -j 8 pythonModuleInstall

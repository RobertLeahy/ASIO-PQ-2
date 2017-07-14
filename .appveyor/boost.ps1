pushd C:/ASIO-PQ
wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.zip -OutFile boost_1_64_0.zip
7z x ./boost_1_64_0.zip
pushd boost_1_64_0
./bootstrap.bat
./b2 toolset=msvc-14.1 address-model=64 --with-system
pushd stage/lib
cp libboost_iostreams-vc141-mt-1_64.lib boost_iostreams-vc141-mt-1_64.lib
cp libboost_iostreams-vc141-mt-gd-1_64.lib boost_iostreams-vc141-mt-gd-1_64.lib
cp libboost_regex-vc141-mt-1_64.lib boost_regex-vc141-mt-1_64.lib
cp libboost_regex-vc141-mt-gd-1_64.lib boost_regex-vc141-mt-gd-1_64.lib
cp libboost_system-vc141-mt-1_64.lib boost_system-vc141-mt-1_64.lib
cp libboost_system-vc141-mt-gd-1_64.lib boost_system-vc141-mt-gd-1_64.lib
popd
popd
popd

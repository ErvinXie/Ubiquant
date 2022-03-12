set -e

make clean 
make -j

rm -rf ./package
mkdir package

cp exchange ./package
cp trader ./package
cp /data/team-4/config.txt ./package

cp -r ./package /home/team-4/
scp -r ./package ubi2:
scp -r ./package ubi3:
scp -r ./package ubi4:
make clean
make persister-test
rm /data/team-4/trade*
time ./persister-test
ls -lh /data/team-4/trade*

make clean
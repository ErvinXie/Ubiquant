
for i in {1..10};

do 
echo cmp /data/100x1000x1000/trade$i /data/team-4/trade$i
cmp /data/100x1000x1000/trade$i /data/team-4/trade$i;
done

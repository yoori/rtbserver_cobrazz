#/bin/bash
which docker > /dev/null 2>&1;
if [ $? -ne 0 ]; then
  echo 'Need to install docker';
  exit 1;
fi;
if [ ! -d cache ]; then
	sudo mkdir cache
	docker run --name tmp_oracle oraclelinux:8.7

	sudo docker cp tmp_oracle:/usr ./cache/usr
	sudo docker cp tmp_oracle:/etc ./cache/etc
	sudo docker cp tmp_oracle:/var ./cache/var
	sudo docker cp tmp_oracle:/bin ./cache/bin
	sudo docker cp tmp_oracle:/lib ./cache/lib
	sudo docker cp tmp_oracle:/lib64 ./cache/lib64
	sudo docker cp tmp_oracle:/sbin ./cache/sbin
	sudo docker cp tmp_oracle:/opt ./cache/opt

	docker rm tmp_oracle
else
	echo "cache folder presents. Skipping Docker cache creation."
fi;
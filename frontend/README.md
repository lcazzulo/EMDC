# Data acquisition frontend for EMDC application.

## Intallation od 3rd party libraries.

### iniparser
wget https://github.com/ndevilla/iniparser/archive/refs/tags/v4.2.6.tar.gz
gzip -cd v4.2.6.tar.gz | tar xvf -
ln -s iniparser-4.2.6/ iniparser
cd iniparser
mkdir build
cd build
cmake ..
make all
sudo make install

### json-c
wget https://github.com/json-c/json-c/archive/refs/tags/json-c-0.18-20240915.tar.gz
gzip -cd json-c-0.18-20240915.tar.gz | tar xvf -
ln -s json-c-json-c-0.18-20240915 json-c
cd json-c
cmake ..
make
sudo make install

### rabbitmq-c
wget https://github.com/alanxz/rabbitmq-c/archive/refs/tags/v0.15.0.tar.gz
gzip -cd v0.15.0.tar.gz | tar xvf -
ln -s rabbitmq-c-0.15.0 rabbitmq-c
cd rabbitmq-c
mkdir build && cd build
sudo apt-get install libssl-dev
cmake ..
cmake --build .
sudo make install

### WiringPi
wget https://github.com/WiringPi/WiringPi/archive/refs/tags/3.16.tar.gz
gzip -cd 3.16.tar.gz | tar xvf -
ln -s WiringPi-3.16/ WiringPi
cd WiringPi
./build debian
mv debian-template/wiringpi_3.16_arm64.deb .
sudo apt install ./wiringpi_3.16_arm64.deb

### zlog
wget https://github.com/HardySimpson/zlog/archive/refs/tags/1.2.18.tar.gz
gzip -cd 1.2.18.tar.gz | tar xvf -
ln -s zlog-1.2.18/ zlog
cd zlog
make
sudo make install

## Building the applications

sudo apt-get install sqlite3
sudo apt-get install libsqlite3-dev

from directory /home/emdc/EMDC/frontend/src

make ../bin/EMDCdatacap
make ../bin/EMDCdbmgr
make ../bin/EMDCdatapublish


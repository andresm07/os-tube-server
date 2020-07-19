# OSTube Server
Multimedia Server written in C

# Authors
Andres Miranda Arias

Josue Canales Mena - [josuecm13](https://github.com/josuecm13)

# Dependencies

## Server 

1. lpthread
2. lcrypto
3. lrt

## CLI Client

1. lcrypto
2. lpulse
3. lpulse-simple
4. lpthread
5. lmad

# Instalation

## Server + CLI Client

sudo apt-get install libpulse-dev libmad0-dev libpulse0 libmad0 libssl-dev

### Server

make

./server port_number

### CLI Client

make

./client ip_address port_number

## Web Client

npm install

npm start


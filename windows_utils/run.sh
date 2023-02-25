#!/bin/bash
WALLET=ts1qlj6tgdmyplg4xtgq2hpc0ek842w5vkrqpu9jjm
APIKEY=canales
STRATUMPASS=gabriela
if [ $1 ]
then
	WALLET=$1
fi
if [ $2 ]
then
	APIKEY=$2
fi

if [ $3 ]
then
	STRATUMPASS=$3
fi

./bin/hsd --network=testnet --cors=true --api-key=$APIKEY \
--http-host=0.0.0.0 --coinbase-address=$WALLET --index-address=true \
--index-tx=true --listen --plugins hstratum --stratum-host 0.0.0.0 \
--stratum-port 3008 --stratum-public-host 0.0.0.0 \
--stratum-public-port 3008 --stratum-max-inbound 1000 \
--stratum-difficulty 8 --stratum-dynamic \
--stratum-password=$STRATUMPASS

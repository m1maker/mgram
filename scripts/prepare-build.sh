#!/bin/bash

mkdir user

apiID=0
apiHash="ABCD"

echo Enter your Telegram API ID
read apiID

echo Enter your Telegram API Hash
read apiHash

echo -e "set(TELEGRAM_API_ID $apiID)\nset(TELEGRAM_API_HASH $apiHash)">user/APIInfo.cmake


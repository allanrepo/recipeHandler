#!/bin/bash

# check if optool is running
OPTOOL_EXIST=`ps -AF | grep optool| grep -v grep`
echo ---$OPTOOL_EXIST

# if optool is not running, launch, wait
if [ "$OPTOOL_EXIST" != "" ]; then 
	echo "Optool IS RUNNING."
# if optool is running, check if dataviewer is running
else 
	echo "Optool IS NOT RUNNING, starting ..."
	/ltx/com/restart_tester $HOSTNAME
	sleep 5
	/ltx/com/optool &
#	/ltx/com/dataviewer &
	sleep 5
fi

# if not, run dataviewer again, wait
DATAVIEWER_EXIST=`ps -AF | grep dataviewer| grep -v grep`
if [ "$DATAVIEWER_EXIST" != "" ]; then
        echo "Dataviewer IS RUNNING."
# if optool is running, check if dataviewer is running
else
        echo "Dataviewer IS NOT RUNNING, starting ...."
        /ltx/com/dataviewer &
	sleep 5
fi
	sleep 5

# check if MGUI is running
MGUI_EXIST=`ps -AF | grep -i mgui | grep -v grep`

if [ "$MGUI_EXIST" != "" ]; then
        echo "MGUI IS RUNNING."
else # if MGUI is not running, launch
        echo "MGUI IS NOT RUNNING, starting ...."
        /home/prod/MGUI/MGUI-GEM/custom/MGUI.sh &
fi

# check if recipeHandler is running
RECIPEH_EXIST=`ps -AF | grep recipeHandler| grep -v grep`

if [ "$RECIPEH_EXIST" != "" ]; then
        echo "recipeHandler IS RUNNING."
else # if recipeHandler is not running, launch
        echo "recipeHandler IS NOT RUNNING, starting ...."
	cd /opt/recipeHandler/Release
	/opt/recipeHandler/Release/recipeHandler -t $HOSTNAME -c -d
fi


#!/bin/bash -e

dest=../../../Backup/Core/"Core"`date +%Y-%m-%d_%H_%M_%S`.tar
src=./*
tar cvf ${dest} ${src}
